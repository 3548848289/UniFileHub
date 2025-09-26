#include "include/ClipboardHistoryManager.h"
#include "include/ClipboardItemFactory.h"
#include "../Setting/include/SettingManager.h"
#include <algorithm>

ClipboardHistoryManager::ClipboardHistoryManager()
    : m_dbService(dbService::instance("./SmartDesk.db")),
    m_initialItemCount(0) {}


void ClipboardHistoryManager::loadHistory(int hours) {
    m_items.clear();
    QSet<QString> loadedContents; // 用于避免重复

    // 1️⃣ 先加载置顶项（无时间限制）
    for (const auto& rec : m_dbService.dbClip().loadPinnedHistory()) {
        std::unique_ptr<ClipboardItem> item =
            ClipboardItemFactory::createFromSerializedString(rec.content);
        if (item) {
            item->setPinned(true);
            m_items.push_back(std::move(item));
            loadedContents.insert(rec.content);
        }
    }

    // 2️⃣ 再加载最近 X 小时的普通项
    for (const auto& rec : m_dbService.dbClip().loadRecentNormalHistory(hours)) {
        if (loadedContents.contains(rec.content)) continue; // 避免重复
        std::unique_ptr<ClipboardItem> item =
            ClipboardItemFactory::createFromSerializedString(rec.content);
        if (item) {
            item->setPinned(false);
            m_items.push_back(std::move(item));
            loadedContents.insert(rec.content);
        }
    }

    // 3️⃣ UI 显示时按置顶排序（已保证置顶在前）
    std::stable_sort(m_items.begin(), m_items.end(),
                     [](const auto& a, const auto& b){ return a->isPinned() && !b->isPinned(); });

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
        if (m_dbService.dbClip().setHistory(item->serialize(), item->isPinned())) {
            successCount++;
        }
    }

    m_initialItemCount = m_items.size();
    return successCount;
}

void ClipboardHistoryManager::moveToPinnedFront(ClipboardItem* item) {
    if (!item) return;

    auto it = std::find_if(m_items.begin(), m_items.end(),
                           [item](const std::unique_ptr<ClipboardItem>& ptr) {
                               return ptr.get() == item;
                           });

    if (it == m_items.end()) return;

    // 如果已经在最前面则无需移动
    if (it == m_items.begin()) return;

    // 将 unique_ptr 移到最前面（保持所有权）
    std::unique_ptr<ClipboardItem> tmp = std::move(*it);
    m_items.erase(it);
    m_items.insert(m_items.begin(), std::move(tmp));
}

void ClipboardHistoryManager::updatePinnedStatus(ClipboardItem* item) {
    if (!item) return;

    // 调用数据库接口更新 pinned
    m_dbService.dbClip().updatePinnedStatus(item->serialize(), item->isPinned());
}
