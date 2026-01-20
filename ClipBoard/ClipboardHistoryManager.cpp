#include "include/ClipboardHistoryManager.h"
#include "include/ClipboardItemFactory.h"
#include "../Setting/include/SettingManager.h"
#include <algorithm>

ClipboardHistoryManager::ClipboardHistoryManager()
    : m_dbService(dbService::instance("./SmartDesk.db")),
    m_initialItemCount(0) {}


void ClipboardHistoryManager::loadHistory(int hours) {
    m_items.clear();

    // 加载置顶项
    for (const auto& rec : m_dbService.dbClip().loadPinnedHistory()) {
        std::unique_ptr<ClipboardItem> item =
            ClipboardItemFactory::createFromSerializedString(rec.content);
        if (item) {
            item->setId(rec.id);
            item->setPinned(rec.isPinned);
            m_items.push_back(std::move(item));
        }
    }

    // 加载普通项
    for (const auto& rec : m_dbService.dbClip().loadRecentNormalHistory(hours)) {
        std::unique_ptr<ClipboardItem> item =
            ClipboardItemFactory::createFromSerializedString(rec.content);
        if (item) {
            item->setId(rec.id);
            item->setPinned(rec.isPinned);
            m_items.push_back(std::move(item));
        }
    }

    // UI 显示时按置顶排序（已保证置顶在前）
    std::stable_sort(m_items.begin(), m_items.end(),
                     [](const auto& a, const auto& b){ return a->isPinned() && !b->isPinned(); });

    m_initialItemCount = m_items.size();
}

bool ClipboardHistoryManager::addItem(std::unique_ptr<ClipboardItem> item) {
    if (!item) return false;

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
        auto& item = m_items[i];
        int newId = m_dbService.dbClip().setHistory(item->serialize(), item->isPinned());
        if (newId != -1) {
            // 设置新插入项的ID
            item->setId(newId);
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

    // 优先使用ID更新，如果ID有效
    if (item->id() != -1) {
        m_dbService.dbClip().updatePinnedStatusById(item->id(), item->isPinned());
    } else {
        // 否则回退到使用内容更新（用于新创建尚未保存的项）
        m_dbService.dbClip().updatePinnedStatus(item->serialize(), item->isPinned());
    }
}
