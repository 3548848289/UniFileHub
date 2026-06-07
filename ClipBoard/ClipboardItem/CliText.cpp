#include "include/ClipboardItem/CliText.h"
#include "../../Setting/include/IconManager.h"
#include <QListWidgetItem>
#include <QVariant>
#include <Qt>

CliText::CliText(const QString& text)
    : ClipboardItem(ClipboardItemType::Text), m_text(text) {}

QListWidgetItem* CliText::createListWidgetItem() const {
    QListWidgetItem* item = new QListWidgetItem;

    const int maxLines = 8;
    const QStringList lines = m_text.split('\n');
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
    if (isCloudItem()) {
        item->setIcon(IconManager::icon(IconManager::Icon::Cloud, QSize(16, 16)));
    }

    return item;
}

void CliText::copyToClipboard(QClipboard* clipboard) const {
    clipboard->setText(m_text);
}

QString CliText::serialize() const {
    return m_text;
}
