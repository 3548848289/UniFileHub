#ifndef FILELOCATIONHELPER_H
#define FILELOCATIONHELPER_H

#include <QString>
#include <QFileInfo>
#include <QStringList>
#include <QMap>

class FileLocationHelper
{
public:
    static bool openFileLocationWithSelection(const QString& filePath);
    static bool openFileLocation(const QString& filePath);
    static bool fileExists(const QString& filePath);
    static QString getFolderPath(const QString& filePath);

private:
    FileLocationHelper();
    ~FileLocationHelper();

    // Linux 文件管理器候选的处理方式：
    // SelectFlag   - 支持“选中文件”参数（nautilus --select、dolphin --select）
    // ShowItem     - 支持定位到文件（dde-file-manager --show-item、peony --show-items）
    // ParentOpen   - 不支持选中，退化为打开所在目录
    enum class ArgMode { SelectFlag, ShowItem, ParentOpen };

    // Linux 下按优先级排列的文件管理器候选表（程序名 -> 参数方式）
    static const QMap<QString, ArgMode>& linuxManagers();

    static QStringList candidatePrograms();
    static QString queryXdgDefaultFileManager();
    static ArgMode argModeForProgram(const QString& program);
    static QStringList buildArgs(const QString& program, ArgMode mode, const QString& nativeFilePath);
    static QString preferredManager();
    static void setPreferredManager(const QString& program);
};

#endif // FILELOCATIONHELPER_H
