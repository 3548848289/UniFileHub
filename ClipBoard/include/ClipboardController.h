#ifndef CLIPBOARDCONTROLLER_H
#define CLIPBOARDCONTROLLER_H

#include <QObject>
#include <QListWidgetItem>
#include "ClipboardItem/ClipboardItem.h"
#include "ClipboardHistoryManager.h"
#include "ClipboardMonitor.h"

class ClipboardView;

class ClipboardController : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardController(QObject *parent = nullptr);
    ~ClipboardController() override;

    void setView(ClipboardView* view);
    ClipboardHistoryManager* getHistoryManager();

public slots:
    // 处理新的剪贴板项
    void handleNewClipboardItem(ClipboardItem* item);
    
    // 用户操作相关
    void copyItemToClipboard(ClipboardItem* item);
    void deleteItem(ClipboardItem* item);
    void clearAllItems();
    void pinItem(ClipboardItem* item);
    void saveHistory();
    
    // 文件操作
    void openFileLocation(ClipboardItem* item);
    
    // 加载历史记录
    void loadHistory(int hours);

private slots:
    void onItemAdded(ClipboardItem* item);
    void onItemRemoved(ClipboardItem* item);
    void onItemsCleared();
    void onItemPinnedStatusChanged(ClipboardItem* item);

signals:
    // 通知View更新的信号
    void itemAddedToModel(ClipboardItem* item);
    void itemRemovedFromModel(ClipboardItem* item);
    void modelCleared();
    void itemPinnedChanged(ClipboardItem* item);

private:
    ClipboardHistoryManager m_historyManager;
    ClipboardMonitor* m_clipboardMonitor;
    ClipboardView* m_view;
};

#endif // CLIPBOARDCONTROLLER_H