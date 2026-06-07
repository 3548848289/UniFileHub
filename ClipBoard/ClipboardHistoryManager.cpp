#include "include/ClipboardHistoryManager.h"
#include "include/ClipboardItem/CliText.h"
#include "include/ClipboardItemFactory.h"
#include <algorithm>

ClipboardHistoryManager::ClipboardHistoryManager()
    : m_dbService(dbService::instance("./SmartDesk.db")),
      m_initialItemCount(0) {}

void ClipboardHistoryManager::loadHistory(int hours) {
    m_items.clear();

    for (const auto& rec : m_dbService.dbClip().loadPinnedHistory()) {
        std::unique_ptr<ClipboardItem> item =
            ClipboardItemFactory::createFromSerializedString(rec.content);
        if (item) {
            item->setId(rec.id);
            item->setPinned(rec.isPinned);
            m_items.push_back(std::move(item));
        }
    }

    for (const auto& rec : m_dbService.dbClip().loadRecentNormalHistory(hours)) {
        std::unique_ptr<ClipboardItem> item =
            ClipboardItemFactory::createFromSerializedString(rec.content);
        if (item) {
            item->setId(rec.id);
            item->setPinned(rec.isPinned);
            m_items.push_back(std::move(item));
        }
    }

    std::stable_sort(m_items.begin(), m_items.end(),
                     [](const auto& a, const auto& b){ return a->isPinned() && !b->isPinned(); });

    m_initialItemCount = m_items.size();
}

bool ClipboardHistoryManager::addItem(std::unique_ptr<ClipboardItem> item) {
    if (!item) {
        return false;
    }

    const int checkRecentCount = 2;
    const QString newItemSerialized = item->serialize();

    int checkCount = 0;
    for (auto it = m_items.rbegin(); it != m_items.rend() && checkCount < checkRecentCount; ++it, ++checkCount) {
        if ((*it)->serialize() == newItemSerialized) {
            return false;
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
    m_dbService.dbClip().clearAllHistory();
    m_items.clear();
    m_initialItemCount = 0;
}

int ClipboardHistoryManager::saveIncremental() {
    int successCount = 0;

    for (size_t i = m_initialItemCount; i < m_items.size(); ++i) {
        auto& item = m_items[i];
        if (item->isCloudItem()) {
            continue;
        }

        const int newId = m_dbService.dbClip().setHistory(item->serialize(), item->isPinned());
        if (newId != -1) {
            item->setId(newId);
            successCount++;
        }
    }

    m_initialItemCount = m_items.size();
    return successCount;
}

void ClipboardHistoryManager::removeCloudItems()
{
    auto newEnd = std::remove_if(m_items.begin(), m_items.end(),
                                 [](const std::unique_ptr<ClipboardItem>& item) {
                                     return item && item->isCloudItem();
                                 });
    m_items.erase(newEnd, m_items.end());
    m_initialItemCount = m_items.size();
}

bool ClipboardHistoryManager::addCloudTextItem(const QString &content, int cloudItemId)
{
    for (const auto &existingItem : m_items) {
        if (!existingItem) {
            continue;
        }
        if (existingItem->isCloudItem() && existingItem->cloudItemId() == cloudItemId) {
            return false;
        }
    }

    auto item = std::make_unique<CliText>(content);
    item->setCloudItem(true);
    item->setCloudItemId(cloudItemId);
    m_items.push_back(std::move(item));
    return true;
}

void ClipboardHistoryManager::moveToPinnedFront(ClipboardItem* item) {
    if (!item) {
        return;
    }

    auto it = std::find_if(m_items.begin(), m_items.end(),
                           [item](const std::unique_ptr<ClipboardItem>& ptr) {
                               return ptr.get() == item;
                           });

    if (it == m_items.end() || it == m_items.begin()) {
        return;
    }

    std::unique_ptr<ClipboardItem> tmp = std::move(*it);
    m_items.erase(it);
    m_items.insert(m_items.begin(), std::move(tmp));
}

void ClipboardHistoryManager::updatePinnedStatus(ClipboardItem* item) {
    if (!item || item->isCloudItem()) {
        return;
    }

    if (item->id() != -1) {
        m_dbService.dbClip().updatePinnedStatusById(item->id(), item->isPinned());
    } else {
        m_dbService.dbClip().updatePinnedStatus(item->serialize(), item->isPinned());
    }
}
