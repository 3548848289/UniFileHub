#include "TextClipboardItem.h"
#include <QListWidgetItem>
#include <QVariant>
#include <Qt>

TextClipboardItem::TextClipboardItem(const QString& text)
    : ClipboardItem(ClipboardItemType::Text), m_text(text) {}

// 创建列表项：显示文本、设置类型标识和tooltip
QListWidgetItem* TextClipboardItem::createListWidgetItem() const {
    QListWidgetItem* item = new QListWidgetItem(m_text);
    // 存储指针（用于后续关联）
    item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(this)));
    // 存储类型标识
    item->setData(Qt::UserRole + 1, "text");
    item->setToolTip(m_text);
    item->setTextAlignment(Qt::AlignTop);
    return item;
}

// 复制文本到剪贴板
void TextClipboardItem::copyToClipboard(QClipboard* clipboard) const {
    clipboard->setText(m_text);
}

// 序列化：直接返回文本（无需额外编码）
QString TextClipboardItem::serialize() const {
    return m_text;
}
