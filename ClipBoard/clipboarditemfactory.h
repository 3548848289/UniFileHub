#ifndef CLIPBOARDITEMFACTORY_H
#define CLIPBOARDITEMFACTORY_H

#include "ClipboardItem.h"
#include "TextClipboardItem.h"
#include "ImageClipboardItem.h"
#include "FileClipboardItem.h"
#include <QMimeData>
#include <memory> // 用于智能指针

// 剪贴板项工厂类（工厂模式核心）
class ClipboardItemFactory {
public:
    // 静态方法：根据QMimeData（剪贴板数据）创建ClipboardItem
    static std::unique_ptr<ClipboardItem> createFromMimeData(const QMimeData* mimeData);
    // 静态方法：根据序列化字符串创建ClipboardItem（用于加载历史记录）
    static std::unique_ptr<ClipboardItem> createFromSerializedString(const QString& serialized);
};

#endif // CLIPBOARDITEMFACTORY_H
