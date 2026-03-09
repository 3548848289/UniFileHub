#ifndef DB_DRIVE_UPLOAD_H
#define DB_DRIVE_UPLOAD_H

#include <QString>
#include <QList>
#include <QDateTime>
#include "dbManager.h"

struct DriveUploadRecord {
    int id;
    int fileId;
    QString fileName;
    qint64 fileSize;
    QString localPath;
    QDateTime uploadTime;
    QString uploadStatus;
    QString fileType;
    int parentId;
};

class dbDriveUpload : public dbManager {
public:
    dbDriveUpload(const QString &dbName);
    bool addUploadRecord(const DriveUploadRecord &record);
    QList<DriveUploadRecord> getUploadHistory(int limit = 50);
    bool clearUploadHistory();
    bool deleteUploadRecord(int id);
    bool updateUploadStatus(int id, const QString &status);
    int getRecordIdByLocalPath(const QString &localPath);
};

#endif // DB_DRIVE_UPLOAD_H