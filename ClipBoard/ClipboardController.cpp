#include "include/ClipboardController.h"
#include "include/ClipboardCloudClient.h"
#include "include/ClipboardItem/CliText.h"
#include "include/ClipboardItem/CliImage.h"
#include "include/ClipboardItemFactory.h"
#include "include/ClipboardView.h"
#include "../Setting/include/SettingManager.h"
#include "../manager/include/FileLocationHelper.h"
#include <QClipboard>
#include <algorithm>
#include <QDateTime>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

namespace {
// 与服务端 CLIPBOARD_MAX_IMAGE_BYTES 默认值保持一致
constexpr int kMaxCloudImageBytes = 16 * 1024 * 1024 - 1;

QDateTime parseCloudTimestamp(const QString &isoTime)
{
    const QDateTime parsed = QDateTime::fromString(isoTime, Qt::ISODateWithMs);
    return parsed.isValid() ? parsed : QDateTime::fromString(isoTime, Qt::ISODate);
}
} // namespace

ClipboardController::ClipboardController(QObject *parent)
    : QObject(parent),
      m_clipboardMonitor(new ClipboardMonitor(this)),
      m_view(nullptr),
      m_cloudClient(new ClipboardCloudClient(this))
{
    connect(m_clipboardMonitor, &ClipboardMonitor::newItemCaptured,
            this, &ClipboardController::handleNewClipboardItem);

    connect(m_cloudClient, &ClipboardCloudClient::uploadSucceeded, this, [this]() {
        // emit infoMessageRequested(QStringLiteral("已同步到云端"));
        if (m_quickSyncPending) {
            m_quickSyncPending = false;
            emit quickActionMessage(QStringLiteral("已同步到云端"), false);
        }
        refreshCloudItems();
    });
    connect(m_cloudClient, &ClipboardCloudClient::uploadFailed, this, [this](const QString &message) {
        if (m_quickSyncPending) {
            m_quickSyncPending = false;
            emit quickActionMessage(message, true);
            return;
        }
        emit errorMessageRequested(message);
    });
    connect(m_cloudClient, &ClipboardCloudClient::deleteSucceeded, this, [this]() {
        // emit infoMessageRequested(QStringLiteral("已取消云端同步"));
        refreshCloudItems();
    });
    connect(m_cloudClient, &ClipboardCloudClient::deleteFailed,
            this, &ClipboardController::errorMessageRequested);
    connect(m_cloudClient, &ClipboardCloudClient::clearSucceeded, this, [this]() {
        m_historyManager.removeCloudItems();
        rebuildModelFromHistory();
    });
    connect(m_cloudClient, &ClipboardCloudClient::clearFailed,
            this, &ClipboardController::errorMessageRequested);
    connect(m_cloudClient, &ClipboardCloudClient::itemsFetched,
            this, &ClipboardController::handleCloudItemsFetched);
    connect(m_cloudClient, &ClipboardCloudClient::imagesFetched,
            this, &ClipboardController::handleCloudImagesFetched);
    connect(m_cloudClient, &ClipboardCloudClient::imageDataReceived,
            this, &ClipboardController::handleImageDataReceived);
    connect(m_cloudClient, &ClipboardCloudClient::fetchFailed,
            this, [this](const QString &message) {
                // 任一请求失败都结束本轮刷新，用已到的数据重建，避免列表卡在旧状态
                if (m_refreshInFlight) {
                    m_textFetchDone = true;
                    m_imageFetchDone = true;
                    m_pendingDownloadIds.clear();
                    tryRebuildCloudModel();
                }
                emit errorMessageRequested(message);
            });
    connect(m_cloudClient, &ClipboardCloudClient::cloudItemsChanged,
            this, &ClipboardController::refreshCloudItems);

    m_cloudClient->startEventStream();
}

ClipboardController::~ClipboardController() = default;

void ClipboardController::setView(ClipboardView* view)
{
    m_view = view;
}

ClipboardHistoryManager* ClipboardController::getHistoryManager()
{
    return &m_historyManager;
}

void ClipboardController::handleNewClipboardItem(ClipboardItem* rawItem)
{
    if (!rawItem) {
        return;
    }

    std::unique_ptr<ClipboardItem> item(rawItem);
    if (m_historyManager.addItem(std::move(item))) {
        ClipboardItem* addedItem = m_historyManager.items().back().get();
        emit itemAddedToModel(addedItem);
        m_historyManager.saveIncremental();

        // "复制即同步"开启时，自动上传到云端（文本与图片）
        const ClipboardItemType type = addedItem->type();
        if (SettingManager::Instance().clip_board_copy_auto_sync()
            && (type == ClipboardItemType::Text || type == ClipboardItemType::Image)
            && !SettingManager::Instance().getToken().trimmed().isEmpty()) {
            if (!isItemAlreadyInCloud(addedItem)) {
                if (type == ClipboardItemType::Text) {
                    m_cloudClient->uploadTextItem(addedItem->serialize());
                } else {
                    auto* imageItem = static_cast<CliImage*>(addedItem);
                    if (imageItem->data().size() > kMaxCloudImageBytes) {
                        emit errorMessageRequested(QStringLiteral("图片过大，未自动同步（上限 16MB）"));
                    } else {
                        m_cloudClient->uploadImageItem(imageItem->data());
                    }
                }
            }
        }
    }
}

bool ClipboardController::isItemAlreadyInCloud(ClipboardItem* item) const
{
    if (!item) {
        return true;
    }

    const QString serialized = item->serialize();
    const auto& items = m_historyManager.items();
    for (const auto& existing : items) {
        if (existing && existing->isCloudItem() && existing->serialize() == serialized) {
            return true;
        }
    }

    // 图片复制时会重新编码 PNG，字节可能不同，再按像素内容比对一次
    if (item->type() == ClipboardItemType::Image) {
        auto* imageItem = static_cast<const CliImage*>(item);
        const QImage newImage = imageItem->pixmap().toImage();
        for (const auto& existing : items) {
            if (!existing || !existing->isCloudItem()
                || existing->type() != ClipboardItemType::Image) {
                continue;
            }
            auto* cloudImage = static_cast<const CliImage*>(existing.get());
            const QPixmap& cloudPixmap = cloudImage->pixmap();
            if (cloudPixmap.width() != newImage.width()
                || cloudPixmap.height() != newImage.height()) {
                continue;
            }
            if (cloudPixmap.toImage() == newImage) {
                return true;
            }
        }
    }

    return false;
}

void ClipboardController::copyItemToClipboard(ClipboardItem* item)
{
    if (!item) {
        return;
    }

    QClipboard* clipboard = QGuiApplication::clipboard();
    item->copyToClipboard(clipboard);
}

void ClipboardController::deleteItem(ClipboardItem* item)
{
    if (!item) {
        return;
    }

    m_historyManager.removeItem(item);
    emit itemRemovedFromModel(item);
    m_historyManager.saveIncremental();
}

void ClipboardController::clearAllItems()
{
    m_historyManager.clear();
    emit modelCleared();
}

void ClipboardController::pinItem(ClipboardItem* item)
{
    if (!item || item->isCloudItem()) {
        return;
    }

    item->setPinned(!item->isPinned());
    m_historyManager.updatePinnedStatus(item);
    emit itemPinnedChanged(item);
    m_historyManager.saveIncremental();
}

void ClipboardController::saveHistory()
{
    m_historyManager.saveIncremental();
}

void ClipboardController::openFileLocation(ClipboardItem* item)
{
    if (!item || item->type() != ClipboardItemType::File) {
        return;
    }

    const QString serialized = item->serialize();
    if (serialized.startsWith("FILE_DATA:")) {
        const QStringList filePaths = serialized.mid(10).split(";");
        if (!filePaths.isEmpty()) {
            FileLocationHelper::openFileLocation(filePaths.first());
        }
    }
}

void ClipboardController::loadHistory(int hours)
{
    emit modelCleared();
    m_historyManager.loadHistory(hours);

    const auto& items = m_historyManager.items();
    for (auto it = items.begin(); it != items.end(); ++it) {
        emit itemAddedToModel(it->get());
    }

    refreshCloudItems();
}

void ClipboardController::searchItems(const QString& query)
{
    emit modelCleared();

    const QString searchText = query.trimmed();
    std::vector<ClipboardItem*> cloudItems;
    std::vector<ClipboardItem*> pinnedItems;
    std::vector<ClipboardItem*> normalItems;

    const auto& items = m_historyManager.items();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        ClipboardItem* item = it->get();
        bool matchFound = false;

        if (item->type() == ClipboardItemType::Text) {
            CliText* textItem = dynamic_cast<CliText*>(item);
            if (searchText.isEmpty() || (textItem && textItem->text().contains(searchText, Qt::CaseInsensitive))) {
                matchFound = true;
            }
        } else if (item->type() == ClipboardItemType::File) {
            if (searchText.isEmpty()) {
                matchFound = true;
            } else {
                const QString serialized = item->serialize();
                if (serialized.startsWith("FILE_DATA:")) {
                    const QStringList filePaths = serialized.mid(10).split(";").filter(QRegularExpression(".+"));
                    for (const QString& path : filePaths) {
                        if (QFileInfo(path).fileName().contains(searchText, Qt::CaseInsensitive)) {
                            matchFound = true;
                            break;
                        }
                    }
                }
            }
        } else if (searchText.isEmpty()) {
            matchFound = true;
        }

        if (matchFound) {
            if (item->isCloudItem()) {
                cloudItems.push_back(item);
            } else if (item->isPinned()) {
                pinnedItems.push_back(item);
            } else {
                normalItems.push_back(item);
            }
        }
    }

    for (auto it = pinnedItems.rbegin(); it != pinnedItems.rend(); ++it) {
        emit itemAddedToModel(*it);
    }
    for (auto it = cloudItems.rbegin(); it != cloudItems.rend(); ++it) {
        emit itemAddedToModel(*it);
    }
    for (auto it = normalItems.rbegin(); it != normalItems.rend(); ++it) {
        emit itemAddedToModel(*it);
    }
}

void ClipboardController::syncItemToCloud(ClipboardItem *item)
{
    if (!item) {
        emit errorMessageRequested(QStringLiteral("未找到要同步的内容"));
        return;
    }
    if (item->isCloudItem()) {
        emit infoMessageRequested(QStringLiteral("该记录已在云端"));
        return;
    }
    if (SettingManager::Instance().getToken().trimmed().isEmpty()) {
        emit errorMessageRequested(QStringLiteral("请先登录账号"));
        return;
    }

    if (item->type() == ClipboardItemType::Text) {
        m_cloudClient->uploadTextItem(item->serialize());
    } else if (item->type() == ClipboardItemType::Image) {
        auto* imageItem = static_cast<CliImage*>(item);
        if (imageItem->data().size() > kMaxCloudImageBytes) {
            emit errorMessageRequested(QStringLiteral("图片过大，无法同步（上限 16MB）"));
            return;
        }
        m_cloudClient->uploadImageItem(imageItem->data());
    } else {
        emit errorMessageRequested(QStringLiteral("当前仅支持文本与图片同步"));
    }
}

void ClipboardController::unsyncItemFromCloud(ClipboardItem *item)
{
    if (!item || !item->isCloudItem()) {
        emit errorMessageRequested(QStringLiteral("未找到云端记录"));
        return;
    }

    if (item->type() == ClipboardItemType::Image) {
        m_cloudClient->deleteImage(item->cloudItemId());
    } else {
        m_cloudClient->deleteItem(item->cloudItemId());
    }
}

void ClipboardController::clearCloudItems()
{
    if (SettingManager::Instance().getToken().trimmed().isEmpty()) {
        emit errorMessageRequested(QStringLiteral("请先登录账号"));
        return;
    }

    m_cloudClient->clearItems();
}

void ClipboardController::quickSyncCurrentClipboard()
{
    if (SettingManager::Instance().getToken().trimmed().isEmpty()) {
        emit quickActionMessage(QStringLiteral("请先登录账号"), true);
        return;
    }

    const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();
    std::unique_ptr<ClipboardItem> item(ClipboardItemFactory::createFromMimeData(mimeData));
    if (!item) {
        emit quickActionMessage(QStringLiteral("剪贴板为空或内容不支持同步"), true);
        return;
    }
    if (item->type() == ClipboardItemType::File) {
        emit quickActionMessage(QStringLiteral("文件内容暂不支持同步到云端"), true);
        return;
    }

    // 上传所需数据先取出：条目交给 handleNewClipboardItem 后所有权即转移
    const bool isText = item->type() == ClipboardItemType::Text;
    const QString textContent = isText ? item->serialize() : QString();
    QByteArray imageData;
    if (!isText) {
        auto *imageItem = static_cast<CliImage *>(item.get());
        imageData = imageItem->data();
        if (imageData.size() > kMaxCloudImageBytes) {
            emit quickActionMessage(QStringLiteral("图片过大，无法同步（上限 16MB）"), true);
            return;
        }
    }

    const bool alreadyInCloud = isItemAlreadyInCloud(item.get());
    // 收录进本地历史；"复制即同步"开启时这里也会自动上传，云端按内容去重
    handleNewClipboardItem(item.release());

    if (alreadyInCloud) {
        emit quickActionMessage(QStringLiteral("内容已在云端"), false);
        return;
    }

    // 无论"复制即同步"是否开启，快速同步都主动上传，重复内容由云端去重
    if (isText) {
        m_cloudClient->uploadTextItem(textContent);
    } else {
        m_cloudClient->uploadImageItem(imageData);
    }
    m_quickSyncPending = true;
    emit quickActionMessage(QStringLiteral("正在同步到云端…"), false);
}

void ClipboardController::quickCopyLatestCloudItem()
{
    ClipboardItem *latest = nullptr;
    const auto &items = m_historyManager.items();
    for (const auto &item : items) {
        if (item && item->isCloudItem()
            && (!latest || item->copyTime() > latest->copyTime())) {
            latest = item.get();
        }
    }

    if (!latest) {
        emit quickActionMessage(QStringLiteral("云端暂无记录，请先刷新"), true);
        return;
    }

    copyItemToClipboard(latest);
    const QString typeHint = latest->type() == ClipboardItemType::Image
                                 ? QStringLiteral("图片")
                                 : QStringLiteral("文本");
    emit quickActionMessage(QStringLiteral("已复制云端最新%1到剪贴板").arg(typeHint), false);
}

void ClipboardController::refreshCloudItems()
{
    if (SettingManager::Instance().getToken().trimmed().isEmpty()) {
        m_refreshInFlight = false;
        m_refreshQueued = false;
        m_pendingDownloadIds.clear();
        m_historyManager.removeCloudItems();
        rebuildModelFromHistory();
        return;
    }

    // 合并进行中的刷新，结束时按需补一轮，避免多个触发源叠加请求
    if (m_refreshInFlight) {
        m_refreshQueued = true;
        return;
    }

    m_refreshInFlight = true;
    m_refreshQueued = false;
    m_textFetchDone = false;
    m_imageFetchDone = false;
    m_pendingTextItems = QJsonArray();
    m_pendingImageMeta = QJsonArray();
    m_pendingDownloadIds.clear();

    m_cloudClient->fetchItems();
    m_cloudClient->fetchImages();
}

void ClipboardController::handleCloudItemsFetched(const QJsonArray &items)
{
    m_pendingTextItems = items;
    m_textFetchDone = true;
    tryRebuildCloudModel();
}

void ClipboardController::handleCloudImagesFetched(const QJsonArray &images)
{
    m_pendingImageMeta = images;
    m_imageFetchDone = true;
    startMissingImageDownloads();
    tryRebuildCloudModel();
}

void ClipboardController::startMissingImageDownloads()
{
    for (const QJsonValue &value : m_pendingImageMeta) {
        if (!value.isObject()) {
            continue;
        }
        const int imageId = value.toObject().value("id").toInt(-1);
        if (imageId > 0 && !m_imageDataCache.contains(imageId)) {
            m_pendingDownloadIds.insert(imageId);
        }
    }

    for (int imageId : m_pendingDownloadIds) {
        m_cloudClient->downloadImageData(imageId);
    }
}

void ClipboardController::handleImageDataReceived(int cloudImageId, const QByteArray &data)
{
    if (!data.isEmpty()) {
        m_imageDataCache.insert(cloudImageId, data);
    }
    m_pendingDownloadIds.remove(cloudImageId);
    tryRebuildCloudModel();
}

void ClipboardController::tryRebuildCloudModel()
{
    if (!m_refreshInFlight
        || !m_textFetchDone
        || !m_imageFetchDone
        || !m_pendingDownloadIds.isEmpty()) {
        return;
    }

    m_refreshInFlight = false;
    if (m_refreshQueued) {
        m_refreshQueued = false;
        refreshCloudItems();
        return;
    }

    // 清理已不在云端列表里的图片缓存
    QSet<int> liveImageIds;
    for (const QJsonValue &value : m_pendingImageMeta) {
        if (value.isObject()) {
            liveImageIds.insert(value.toObject().value("id").toInt(-1));
        }
    }
    for (auto it = m_imageDataCache.begin(); it != m_imageDataCache.end();) {
        if (!liveImageIds.contains(it.key())) {
            it = m_imageDataCache.erase(it);
        } else {
            ++it;
        }
    }

    m_historyManager.removeCloudItems();

    // 文本与图片按服务端时间戳合并排序后加入，保持云端区新旧一致
    struct CloudEntry {
        QDateTime createdAt;
        bool isImage;
        QJsonObject obj;
    };
    std::vector<CloudEntry> entries;
    for (const QJsonValue &value : m_pendingTextItems) {
        if (value.isObject()) {
            entries.push_back({parseCloudTimestamp(value.toObject().value("created_at").toString()),
                               false, value.toObject()});
        }
    }
    for (const QJsonValue &value : m_pendingImageMeta) {
        if (value.isObject()) {
            entries.push_back({parseCloudTimestamp(value.toObject().value("created_at").toString()),
                               true, value.toObject()});
        }
    }
    std::stable_sort(entries.begin(), entries.end(),
                     [](const CloudEntry &a, const CloudEntry &b) {
                         return a.createdAt > b.createdAt;
                     });

    for (const CloudEntry &entry : entries) {
        const int cloudId = entry.obj.value("id").toInt(-1);
        if (cloudId <= 0) {
            continue;
        }
        if (entry.isImage) {
            const QByteArray pngData = m_imageDataCache.value(cloudId);
            if (pngData.isEmpty()) {
                continue; // 下载失败的图片等下一轮刷新再补
            }
            m_historyManager.addCloudImageItem(cloudId, pngData,
                                               parseCloudTimestamp(entry.obj.value("created_at").toString()));
        } else {
            m_historyManager.addCloudTextItem(entry.obj.value("content").toString(), cloudId,
                                              parseCloudTimestamp(entry.obj.value("created_at").toString()));
        }
    }

    rebuildModelFromHistory();
}

void ClipboardController::rebuildModelFromHistory()
{
    emit modelCleared();
    const auto& items = m_historyManager.items();
    for (auto it = items.begin(); it != items.end(); ++it) {
        emit itemAddedToModel(it->get());
    }
}

void ClipboardController::onItemAdded(ClipboardItem* item)
{
    emit itemAddedToModel(item);
}

void ClipboardController::onItemRemoved(ClipboardItem* item)
{
    emit itemRemovedFromModel(item);
}

void ClipboardController::onItemsCleared()
{
    emit modelCleared();
}

void ClipboardController::onItemPinnedStatusChanged(ClipboardItem* item)
{
    emit itemPinnedChanged(item);
}
