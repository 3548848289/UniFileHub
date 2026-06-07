#ifndef CLIPBOARDHISTORYMANAGER_H
#define CLIPBOARDHISTORYMANAGER_H

#include <memory>
#include <QString>
#include <vector>
#include "./ClipboardItem/ClipboardItem.h"
#include "../../manager/include/dbService.h"

class ClipboardHistoryManager {
public:
    ClipboardHistoryManager();

    void loadHistory(int hours);
    bool addItem(std::unique_ptr<ClipboardItem> item);
    void removeItem(ClipboardItem* item);
    void clear();
    int saveIncremental();
    void removeCloudItems();
    bool addCloudTextItem(const QString &content, int cloudItemId);

    const std::vector<std::unique_ptr<ClipboardItem>>& items() const { return m_items; }

    void moveToPinnedFront(ClipboardItem *item);
    void updatePinnedStatus(ClipboardItem *item);

private:
    dbService& m_dbService;
    std::vector<std::unique_ptr<ClipboardItem>> m_items;
    size_t m_initialItemCount;
};

#endif // CLIPBOARDHISTORYMANAGER_H
