#ifndef FILECLIPBOARDITEM_H
#define FILECLIPBOARDITEM_H

#include "ClipboardItem.h"
#include <QStringList>
#include <QIcon>

// 前置声明工具类
class FileTypeDetector;

// 文件类型剪贴板项（继承自基类）
class CliFile : public ClipboardItem {
public:
    explicit CliFile(const QStringList& filePaths);

    // 重写基类纯虚函数
    QListWidgetItem* createListWidgetItem() const override;
    void copyToClipboard(QClipboard* clipboard) const override;
    QString serialize() const override;

    // 获取文件路径列表
    const QStringList& filePaths() const { return m_filePaths; }
    // 判断是否为图片文件（单文件场景）
    bool isImageFile() const;
    bool isDirectory(const QString &path) const;

    // 静态成员：支持的图片格式（全局共享）
    static QStringList s_supportedImageFormats;

private:
    QStringList m_filePaths; // 文件路径列表（支持多文件）
};

#endif // FILECLIPBOARDITEM_H
