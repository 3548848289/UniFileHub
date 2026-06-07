#ifndef CLIPBOARDITEM_H
#define CLIPBOARDITEM_H

#include <QClipboard>
#include <QListWidgetItem>
#include <QString>

enum class ClipboardItemType {
    Text,
    Image,
    File,
    Unknown
};

class ClipboardItem {
public:
    explicit ClipboardItem(ClipboardItemType type)
        : m_pinned(false),
          m_type(type),
          m_id(-1),
          m_sequenceNumber(0),
          m_isCloudItem(false),
          m_cloudItemId(-1) {}

    virtual ~ClipboardItem() = default;

    ClipboardItemType type() const { return m_type; }

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int sequenceNumber() const { return m_sequenceNumber; }
    void setSequenceNumber(int number) { m_sequenceNumber = number; }

    bool isPinned() const { return m_pinned; }
    void setPinned(bool pinned) { m_pinned = pinned; }

    bool isCloudItem() const { return m_isCloudItem; }
    void setCloudItem(bool cloudItem) { m_isCloudItem = cloudItem; }

    int cloudItemId() const { return m_cloudItemId; }
    void setCloudItemId(int cloudItemId) { m_cloudItemId = cloudItemId; }

    virtual QListWidgetItem* createListWidgetItem() const = 0;
    virtual void copyToClipboard(QClipboard* clipboard) const = 0;
    virtual QString serialize() const = 0;

protected:
    bool m_pinned;
    ClipboardItemType m_type;
    int m_id;
    int m_sequenceNumber;
    bool m_isCloudItem;
    int m_cloudItemId;
};

#endif // CLIPBOARDITEM_H
