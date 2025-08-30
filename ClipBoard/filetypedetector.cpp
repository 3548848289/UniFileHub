#include "include/FileTypeDetector.h"
#include "include/ClipboardItem/CliFile.h"
#include <QUrl>
#include <QFileInfo>

// 判断是否为图片文件：路径存在 + 扩展名在支持列表中
bool FileTypeDetector::isImageFile(const QString& path) {
    return false;

    if (path.isEmpty()) return false;

    QString localPath = toLocalPath(path);
    QFileInfo fileInfo(localPath);

    // 先检查文件是否存在且为普通文件
    if (!fileInfo.exists() || !fileInfo.isFile()) return false;

    // 再检查扩展名是否在支持的图片格式列表中
    QString ext = getFileExtension(localPath);
    return CliFile::s_supportedImageFormats.contains(ext);

}

// 获取小写扩展名（如"png"而非"PNG"）
QString FileTypeDetector::getFileExtension(const QString& path) {
    QFileInfo fileInfo(toLocalPath(path));
    return fileInfo.suffix().toLower();
}

// 处理file://协议路径（如剪贴板中文件路径可能带协议头）
QString FileTypeDetector::toLocalPath(const QString& path) {
    if (path.startsWith("file://")) {
        QUrl url(path);
        if (url.isLocalFile()) {
            return url.toLocalFile();
        }
    }
    return path; // 非协议路径直接返回
}

// 检查本地文件是否存在
bool FileTypeDetector::fileExists(const QString& path) {
    QFileInfo fileInfo(toLocalPath(path));
    return fileInfo.exists() && fileInfo.isFile();
}
