#ifndef FILETYPEDETECTOR_H
#define FILETYPEDETECTOR_H

#include <QString>
#include <QFileInfo>

// 文件类型检测工具类（纯静态方法，无状态）
class FileTypeDetector {
public:
    // 判断文件是否为支持的图片格式
    static bool isImageFile(const QString& path);
    // 获取文件扩展名（小写）
    static QString getFileExtension(const QString& path);
    // 将路径转换为本地路径（处理file://协议）
    static QString toLocalPath(const QString& path);
    // 检查文件是否存在（本地文件）
    static bool fileExists(const QString& path);
};

#endif // FILETYPEDETECTOR_H
