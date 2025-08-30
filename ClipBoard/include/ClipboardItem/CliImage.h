#ifndef IMAGECLIPBOARDITEM_H
#define IMAGECLIPBOARDITEM_H

#include "ClipboardItem.h"
#include <QPixmap>
#include <QByteArray>

// 图片类型剪贴板项（继承自基类）
class CliImage : public ClipboardItem {
public:
    // 构造函数：支持直接传入QPixmap或二进制数据
    explicit CliImage(const QPixmap& pixmap);
    explicit CliImage(const QByteArray& data);

    // 重写基类纯虚函数
    QListWidgetItem* createListWidgetItem() const override;
    void copyToClipboard(QClipboard* clipboard) const override;
    QString serialize() const override;

    // 获取图片数据
    const QPixmap& pixmap() const { return m_pixmap; }
    const QByteArray& data() const { return m_data; }

private:
    QPixmap m_pixmap;   // 图片对象（用于UI展示）
    QByteArray m_data;  // 二进制数据（PNG格式，用于序列化）
};

#endif // IMAGECLIPBOARDITEM_H
