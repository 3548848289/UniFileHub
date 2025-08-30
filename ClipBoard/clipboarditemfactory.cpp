#include "ClipboardItemFactory.h"
#include "FileTypeDetector.h"
#include <QMimeData>
#include <QUrl>
#include <QVariant>
#include <QPixmap>
#include <QImage>
#include <QByteArray>

// 根据剪贴板MimeData创建对应项
std::unique_ptr<ClipboardItem> ClipboardItemFactory::createFromMimeData(const QMimeData* mimeData) {
    if (!mimeData) return nullptr;

    // 1. 优先处理文件URL（支持多文件）
    if (mimeData->hasUrls() && !mimeData->urls().isEmpty()) {
        QStringList filePaths;
        for (const QUrl& url : mimeData->urls()) {
            if (url.isLocalFile()) {
                filePaths.append(url.toLocalFile());
            }
        }
        if (!filePaths.isEmpty()) {
            return std::make_unique<FileClipboardItem>(filePaths);
        }
    }

    // 2. 处理图片数据
    if (mimeData->hasImage()) {
        const QVariant imageVar = mimeData->imageData();
        QPixmap pixmap;

        if (imageVar.canConvert<QImage>()) {
            pixmap = QPixmap::fromImage(imageVar.value<QImage>());
        } else if (imageVar.canConvert<QPixmap>()) {
            pixmap = imageVar.value<QPixmap>();
        }

        if (!pixmap.isNull()) {
            return std::make_unique<ImageClipboardItem>(pixmap);
        }
    }

    // 3. 处理文本数据
    if (mimeData->hasText()) {
        QString text = mimeData->text();
        if (!text.isEmpty() && !text.trimmed().isEmpty()) {
            // 特殊情况：文本是图片文件路径 → 创建Image项
            if (FileTypeDetector::isImageFile(text)) {
                QPixmap pixmap(FileTypeDetector::toLocalPath(text));
                if (!pixmap.isNull()) {
                    return std::make_unique<ImageClipboardItem>(pixmap);
                }
            }
            // 普通文本 → 创建Text项
            return std::make_unique<TextClipboardItem>(text);
        }
    }

    return nullptr; // 不支持的数据类型
}

// 根据序列化字符串创建对应项（用于加载历史记录）
std::unique_ptr<ClipboardItem> ClipboardItemFactory::createFromSerializedString(const QString& serialized) {
    // 1. 图片数据（前缀：IMAGE_DATA:）
    if (serialized.startsWith("IMAGE_DATA:")) {
        QByteArray imageData = QByteArray::fromBase64(serialized.mid(10).toUtf8());
        return std::make_unique<ImageClipboardItem>(imageData);
    }

    // 2. 文件数据（前缀：FILE_DATA:）
    if (serialized.startsWith("FILE_DATA:")) {
        QStringList filePaths = serialized.mid(10).split(";");
        return std::make_unique<FileClipboardItem>(filePaths);
    }

    // 3. 文本数据（无前缀）
    // 检查文本是否为图片路径 → 是则创建Image项，否则创建Text
}
