#ifndef CLIPBOARDITEM_H
#define CLIPBOARDITEM_H

#include <QClipboard>
#include <QListWidgetItem>
#include <QString>
#include <QDateTime>

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
          m_cloudItemId(-1),
          m_copyTime(QDateTime::currentDateTime()) {}

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

    // 复制时间（用于 Tooltip 展示，不参与复制与序列化）
    QDateTime copyTime() const { return m_copyTime; }
    void setCopyTime(const QDateTime &time) { m_copyTime = time; }

    // 在 Tooltip 顶部追加"复制时间：yyyy-MM-dd HH:mm:ss"行
    QString toolTipWithCopyTime(const QString &originalToolTip) const {
        const QString timeLine = QStringLiteral("复制时间：%1")
                                     .arg(m_copyTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        return originalToolTip.isEmpty() ? timeLine : timeLine + QStringLiteral("\n") + originalToolTip;
    }

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
    QDateTime m_copyTime;
};

#endif // CLIPBOARDITEM_H
