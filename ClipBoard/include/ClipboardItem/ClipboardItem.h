#ifndef CLIPBOARDITEM_H
#define CLIPBOARDITEM_H

#include <QListWidgetItem>
#include <QClipboard>
#include <QString>

// 剪贴板项类型枚举
enum class ClipboardItemType {
    Text,  // 文本类型
    Image, // 图片类型
    File   // 文件类型
};

// 剪贴板项基类（抽象类，采用多态设计）
class ClipboardItem {
public:
    explicit ClipboardItem(ClipboardItemType type) : m_type(type), m_pinned(false) {}
    virtual ~ClipboardItem() = default;

    // 获取类型
    ClipboardItemType type() const { return m_type; }


    bool isPinned() const { return m_pinned; }
    void setPinned(bool pinned) { m_pinned = pinned; }

    // 纯虚函数：创建列表项（UI展示）
    virtual QListWidgetItem* createListWidgetItem() const = 0;
    // 纯虚函数：复制到剪贴板
    virtual void copyToClipboard(QClipboard* clipboard) const = 0;
    // 纯虚函数：序列化（用于持久化存储）
    virtual QString serialize() const = 0;

protected:
    bool m_pinned;
    ClipboardItemType m_type; // 类型标识
};

#endif // CLIPBOARDITEM_H
