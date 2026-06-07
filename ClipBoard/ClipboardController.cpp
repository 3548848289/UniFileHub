#include "include/ClipboardController.h"
#include "include/ClipboardCloudClient.h"
#include "include/ClipboardItem/CliText.h"
#include "include/ClipboardView.h"
#include "../Setting/include/SettingManager.h"
#include "../manager/include/FileLocationHelper.h"
#include <QClipboard>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

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
        refreshCloudItems();
    });
    connect(m_cloudClient, &ClipboardCloudClient::uploadFailed,
            this, &ClipboardController::errorMessageRequested);
    connect(m_cloudClient, &ClipboardCloudClient::deleteSucceeded, this, [this]() {
        // emit infoMessageRequested(QStringLiteral("已取消云端同步"));
        refreshCloudItems();
    });
    connect(m_cloudClient, &ClipboardCloudClient::deleteFailed,
            this, &ClipboardController::errorMessageRequested);
    connect(m_cloudClient, &ClipboardCloudClient::itemsFetched,
            this, &ClipboardController::handleCloudItemsFetched);
    connect(m_cloudClient, &ClipboardCloudClient::fetchFailed,
            this, &ClipboardController::errorMessageRequested);
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
    }
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

    for (auto it = cloudItems.rbegin(); it != cloudItems.rend(); ++it) {
        emit itemAddedToModel(*it);
    }
    for (auto it = pinnedItems.rbegin(); it != pinnedItems.rend(); ++it) {
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
    if (item->type() != ClipboardItemType::Text) {
        emit errorMessageRequested(QStringLiteral("当前仅支持文本同步"));
        return;
    }
    if (SettingManager::Instance().getToken().trimmed().isEmpty()) {
        emit errorMessageRequested(QStringLiteral("请先登录账号"));
        return;
    }

    m_cloudClient->uploadTextItem(item->serialize());
}

void ClipboardController::unsyncItemFromCloud(ClipboardItem *item)
{
    if (!item || !item->isCloudItem()) {
        emit errorMessageRequested(QStringLiteral("未找到云端记录"));
        return;
    }

    m_cloudClient->deleteItem(item->cloudItemId());
}

void ClipboardController::refreshCloudItems()
{
    if (SettingManager::Instance().getToken().trimmed().isEmpty()) {
        m_historyManager.removeCloudItems();
        emit modelCleared();
        const auto& items = m_historyManager.items();
        for (auto it = items.begin(); it != items.end(); ++it) {
            emit itemAddedToModel(it->get());
        }
        return;
    }

    m_cloudClient->fetchItems();
}

void ClipboardController::handleCloudItemsFetched(const QJsonArray &items)
{
    m_historyManager.removeCloudItems();
    for (const QJsonValue &value : items) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject obj = value.toObject();
        m_historyManager.addCloudTextItem(obj.value("content").toString(), obj.value("id").toInt(-1));
    }

    emit modelCleared();
    const auto& allItems = m_historyManager.items();
    for (auto it = allItems.begin(); it != allItems.end(); ++it) {
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
