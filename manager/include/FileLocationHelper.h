#ifndef FILELOCATIONHELPER_H
#define FILELOCATIONHELPER_H

#include <QString>
#include <QFileInfo>

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
};

#endif // FILELOCATIONHELPER_H