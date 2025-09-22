#include "include/ClipboardHistoryManager.h"
#include "include/ClipboardItemFactory.h"
#include "../Setting/include/SettingManager.h"
#include <algorithm>

ClipboardHistoryManager::ClipboardHistoryManager()
    : m_dbService(dbService::instance("./SmartDesk.db")),
    m_initialItemCount(0) {}

void ClipboardHistoryManager::loadHistory(int hours) {
    QList<QString> serializedItems = m_dbService.dbClip().loadRecentHistory(hours);

    for (const QString& serialized : serializedItems) {
        std::unique_ptr<ClipboardItem> item =
            ClipboardItemFactory::createFromSerializedString(serialized);
        if (item) {
            m_items.push_back(std::move(item));
        }
    }
    m_initialItemCount = m_items.size();
}

bool ClipboardHistoryManager::addItem(std::unique_ptr<ClipboardItem> item) {
    if (!item) return false;

    QString serialized = item->serialize();
    for (const auto& existing : m_items) {
        if (existing->serialize() == serialized) {
            return false; // 已存在
        }
    }
    m_items.push_back(std::move(item));
    return true;
}

void ClipboardHistoryManager::removeItem(ClipboardItem* item) {
    auto it = std::find_if(m_items.begin(), m_items.end(),
                           [item](const std::unique_ptr<ClipboardItem>& ptr) {
                               return ptr.get() == item;
                           });
    if (it != m_items.end()) {
        m_items.erase(it);
    }
}

void ClipboardHistoryManager::clear() {
    m_items.clear();
    m_initialItemCount = 0;
}

int ClipboardHistoryManager::saveIncremental() {
    int successCount = 0;
    for (size_t i = m_initialItemCount; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        if (m_dbService.dbClip().setHistory(item->serialize())) {
            successCount++;
        }
    }
    m_initialItemCount = m_items.size();
    return successCount;
}
