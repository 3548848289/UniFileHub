#ifndef DB_DRIVE_DOWNLOAD_H
#define DB_DRIVE_DOWNLOAD_H

#include <QString>
#include <QList>
#include <QDateTime>
#include "dbManager.h"

struct DriveDownloadRecord {
    int id;
    int fileId;
    QString fileName;
    qint64 fileSize;
    QString savePath;
    QDateTime downloadTime;
    QString downloadStatus;
    QString fileType;
};

class dbDriveDownload : public dbManager {
public:
    dbDriveDownload(const QString &dbName);
    bool addDownloadRecord(const DriveDownloadRecord &record);
    QList<DriveDownloadRecord> getDownloadHistory(int limit = 50);
    bool clearDownloadHistory();
    bool deleteDownloadRecord(int id);
    bool updateDownloadStatus(int id, const QString &status);
    int getRecordIdBySavePath(const QString &savePath);
};

#endif // DB_DRIVE_DOWNLOAD_H