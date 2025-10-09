#include "include/ClipboardController.h"
#include "include/ClipboardView.h"
#include "include/ClipboardItemFactory.h"
#include "include/FileTypeDetector.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

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
    for (const auto& item : m_historyManager.items()) {
        emit itemAddedToModel(item.get());
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