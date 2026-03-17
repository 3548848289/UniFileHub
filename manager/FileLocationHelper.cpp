#include "include/FileLocationHelper.h"
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QSysInfo>

bool FileLocationHelper::openFileLocationWithSelection(const QString& filePath)
{
    if (!fileExists(filePath)) {
        return false;
    }
    
    QString nativeFilePath = QDir::toNativeSeparators(filePath);
    
    // 根据操作系统类型选择不同的文件管理器
    QString osName = QSysInfo::productType();
    if (osName == "windows") {
        return QProcess::startDetached("explorer.exe", {"/select,", nativeFilePath});
    } else if (osName == "ubuntu" || osName == "debian") {
        // Ubuntu 使用 Nautilus 文件管理器
        return QProcess::startDetached("nautilus", {"--select", nativeFilePath});
    }
    
    // 其他操作系统默认使用 QDesktopServices
    return QDesktopServices::openUrl(QUrl::fromLocalFile(nativeFilePath));
}

bool FileLocationHelper::openFileLocation(const QString& filePath)
{
    if (!fileExists(filePath)) {
        return false;
    }
    
    QString folderPath = getFolderPath(filePath);
    return QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

bool FileLocationHelper::fileExists(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.exists();
}

QString FileLocationHelper::getFolderPath(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.absolutePath();
}

FileLocationHelper::FileLocationHelper() {}
FileLocationHelper::~FileLocationHelper() {}