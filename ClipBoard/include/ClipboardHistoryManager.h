#ifndef CLIPBOARDHISTORYMANAGER_H
#define CLIPBOARDHISTORYMANAGER_H

#include <vector>
#include <memory>
#include <QString>
#include "./ClipboardItem/ClipboardItem.h"
#include "../../manager/include/dbService.h"
class ClipboardHistoryManager {
public:
    ClipboardHistoryManager();

    // 加载历史记录
    void loadHistory(int hours);

    // 添加新项（自动去重）
    bool addItem(std::unique_ptr<ClipboardItem> item);

    // 删除项
    void removeItem(ClipboardItem* item);

    // 清空所有项
    void clear();

    // 保存增量数据
    int saveIncremental();

    // 获取当前项
    const std::vector<std::unique_ptr<ClipboardItem>>& items() const { return m_items; }

private:
    dbService& m_dbService;
    std::vector<std::unique_ptr<ClipboardItem>> m_items;
    size_t m_initialItemCount; // 初始加载项数
};

#endif // CLIPBOARDHISTORYMANAGER_H
