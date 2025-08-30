#ifndef TEXTCLIPBOARDITEM_H
#define TEXTCLIPBOARDITEM_H

#include "ClipboardItem.h"
#include <QString>

// 文本类型剪贴板项（继承自基类）
class CliText : public ClipboardItem {
public:
    explicit CliText(const QString& text);

    // 重写基类纯虚函数
    QListWidgetItem* createListWidgetItem() const override;
    void copyToClipboard(QClipboard* clipboard) const override;
    QString serialize() const override;

    // 获取文本内容
    const QString& text() const { return m_text; }

private:
    QString m_text; // 文本数据
};

#endif // CliText_H
