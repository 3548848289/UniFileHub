#include "include/ClipboardItem/CliText.h"
#include <QListWidgetItem>
#include <QVariant>
#include <Qt>

CliText::CliText(const QString& text)
    : ClipboardItem(ClipboardItemType::Text), m_text(text) {}



// 创建列表项：显示文本（限制行数）、设置类型标识和tooltip
QListWidgetItem* CliText::createListWidgetItem() const {
    QListWidgetItem* item = new QListWidgetItem;

    const int maxLines = 8;
    QStringList lines = m_text.split('\n');
    QString displayText;

    if (lines.size() > maxLines) {
        displayText = lines.mid(0, maxLines).join("\n") + "\n...";
    } else {
        displayText = m_text;
    }

    item->setText(displayText);
    item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(this)));
    item->setToolTip(m_text);
    item->setTextAlignment(Qt::AlignTop);

    return item;
}


// 复制文本到剪贴板
void CliText::copyToClipboard(QClipboard* clipboard) const {
    clipboard->setText(m_text);
}

// 序列化：直接返回文本（无需额外编码）
QString CliText::serialize() const {
    return m_text;
}
