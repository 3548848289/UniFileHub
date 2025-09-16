#include "include/ClipboardItem/CliText.h"
#include <QListWidgetItem>
#include <QVariant>
#include <Qt>

CliText::CliText(const QString& text)
    : ClipboardItem(ClipboardItemType::Text), m_text(text) {}

// 创建列表项：显示文本、设置类型标识和tooltip
QListWidgetItem* CliText::createListWidgetItem() const {
    QListWidgetItem* item = new QListWidgetItem;

    // 设置缩略文本（例如前100个字符）
    QString displayText = m_text;
    if (displayText.length() > 100) {
        displayText = displayText.left(100) + "...";
    }
    item->setText(displayText);


    // 存储指针地址用于查找
    item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(
                                    reinterpret_cast<quintptr>(this)));
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
