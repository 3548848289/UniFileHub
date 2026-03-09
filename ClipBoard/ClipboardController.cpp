#include "include/ClipboardController.h"
#include "include/ClipboardView.h"
#include "include/ClipboardItemFactory.h"
#include "include/FileTypeDetector.h"
#include "include/ClipboardItem/CliText.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QRegularExpression>

ClipboardController::ClipboardController(QObject *parent)
    : QObject(parent), m_view(nullptr)
{
    m_clipboardMonitor = new ClipboardMonitor(this);
    
    // 连接剪贴板监控信号
    connect(m_clipboardMonitor, &ClipboardMonitor::newItemCaptured,
            this, &ClipboardController::handleNewClipboardItem);
}

ClipboardController::~ClipboardController()
{
    // 清理资源
}

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
    if (!rawItem) return;
    
    std::unique_ptr<ClipboardItem> item(rawItem);
    if (m_historyManager.addItem(std::move(item))) {
        ClipboardItem* addedItem = m_historyManager.items().back().get();
        emit itemAddedToModel(addedItem);
        
        // 自动保存到数据库，防止异常退出时数据丢失
        m_historyManager.saveIncremental();
    }
}

void ClipboardController::copyItemToClipboard(ClipboardItem* item)
{
    if (!item) return;
    
    QClipboard* clipboard = QGuiApplication::clipboard();
    item->copyToClipboard(clipboard);
}

void ClipboardController::deleteItem(ClipboardItem* item)
{
    if (!item) return;
    
    m_historyManager.removeItem(item);
    emit itemRemovedFromModel(item);
    
    // 自动保存到数据库，防止异常退出时数据丢失
    m_historyManager.saveIncremental();
}

void ClipboardController::clearAllItems()
{
    m_historyManager.clear();
    emit modelCleared();
}

void ClipboardController::pinItem(ClipboardItem* item)
{
    if (!item) return;
    
    item->setPinned(!item->isPinned());
    m_historyManager.updatePinnedStatus(item);
    emit itemPinnedChanged(item);
    
    // 自动保存到数据库，防止异常退出时数据丢失
    m_historyManager.saveIncremental();
}

void ClipboardController::saveHistory()
{
    m_historyManager.saveIncremental();
}

void ClipboardController::openFileLocation(ClipboardItem* item)
{
    if (!item || item->type() != ClipboardItemType::File) return;
    
    // 由于我们不能直接包含CliFile.h（避免循环依赖），这里使用动态转换
    // 在实际实现中，我们可能需要在ClipboardItem中添加获取文件路径的接口
    // 暂时通过序列化方式获取文件路径
    QString serialized = item->serialize();
    if (serialized.startsWith("FILE_DATA:")) {
        QStringList filePaths = serialized.mid(10).split(";");
        if (!filePaths.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePaths.first()));
        }
    }
}

void ClipboardController::loadHistory(int hours)
{
    m_historyManager.loadHistory(hours);

    // 通知View加载完成，可以更新UI
    // 正向遍历（从最旧到最新），因为View的insertNewItem会将每个新项目插入到头部
    // 这样最终顺序会是：最新的在前面，最旧的在后面
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

void ClipboardController::searchItems(const QString& query)
{
    // 先清空现有内容
    emit modelCleared();

    QString searchText = query.trimmed();

    // 收集符合条件的置顶项和普通项
    std::vector<ClipboardItem*> pinnedItems;
    std::vector<ClipboardItem*> normalItems;

    // 从最新到最旧收集项目，确保新项排在前面（反向迭代）
    const auto& items = m_historyManager.items();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        ClipboardItem* item = it->get();
        bool matchFound = false;

        // 根据不同类型的剪贴板项目进行搜索
        if (item->type() == ClipboardItemType::Text) {
            // 文本类型的项目可以直接搜索内容
            CliText* textItem = dynamic_cast<CliText*>(item);
            if (searchText.isEmpty() || (textItem && textItem->text().contains(searchText, Qt::CaseInsensitive))) {
                matchFound = true;
            }
        } else if (item->type() == ClipboardItemType::File) {
            // 文件类型的项目可以搜索文件名
            if (searchText.isEmpty()) {
                matchFound = true;
            } else {
                QString serialized = item->serialize();
                if (serialized.startsWith("FILE_DATA:")) {
                    QStringList filePaths = serialized.mid(10).split(";").filter(QRegularExpression(".+"));
                    for (const QString& path : filePaths) {
                        if (QFileInfo(path).fileName().contains(searchText, Qt::CaseInsensitive)) {
                            matchFound = true;
                            break;
                        }
                    }
                }
            }
        } else if (searchText.isEmpty()) {
            // 空搜索时显示所有类型
            matchFound = true;
        }

        // 将匹配的项目添加到相应的列表
        if (matchFound) {
            if (item->isPinned()) {
                pinnedItems.push_back(item);
            } else {
                normalItems.push_back(item);
            }
        }
    }

    // 按优先级排序显示：先显示置顶项（从旧到新顺序发射，因为View使用insertNewItem会在头部插入）
    // 使用反向迭代器，从最旧的置顶项开始发射，这样最新的会在最前面
    for (auto it = pinnedItems.rbegin(); it != pinnedItems.rend(); ++it) {
        emit itemAddedToModel(*it);
    }
    // 普通项同样从最旧的开始发射
    for (auto it = normalItems.rbegin(); it != normalItems.rend(); ++it) {
        emit itemAddedToModel(*it);
    }
}