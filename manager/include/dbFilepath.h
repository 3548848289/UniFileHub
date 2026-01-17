#ifndef DBFILEPATH_H
#define DBFILEPATH_H

#include <QString>
#include <QDateTime>
#include <QStringList>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "dbManager.h"

#define ADDFILEPATH \
    "INSERT INTO FilePaths (file_path) VALUES (:filePath)"

#define GETFILEID \
    "SELECT id FROM FilePaths WHERE file_path = :filePath"

#define SEARCHFILES \
    "SELECT fp.file_path, t.tag_name, fp.expiration_date, a.annotation, " \
    "fp.reminder_time, fp.interval_time, fp.notify_display_time, fp.last_reminder_index " \
        "FROM FilePaths fp " \
        "LEFT JOIN Tags t ON fp.id = t.file_id " \
        "LEFT JOIN Annotations a ON fp.id = a.file_id " \
        "WHERE fp.file_path LIKE :keyword " \
        "OR t.tag_name LIKE :keyword " \
        "OR a.annotation LIKE :keyword"

#define GETALLFILEPATHS \
    "SELECT file_path FROM FilePaths"

#define SAVETAGS1 \
    "DELETE FROM Tags WHERE file_id = :fileId"
#define SAVETAGS2 \
    "INSERT INTO Tags (file_id, tag_name) VALUES (:fileId, :tagName)"
#define SAVEANNOTATION \
    "INSERT OR REPLACE INTO Annotations (file_id, annotation) VALUES (:fileId, :annotation)"
#define SAVEEXPIRATIONDATE \
    "UPDATE FilePaths SET expiration_date = :expiration_date WHERE id = :file_id"

#define DELETETAG1 \
    "DELETE FROM Tags WHERE file_id = :file_id"
#define DELETETAG2 \
    "DELETE FROM Annotations WHERE file_id = :file_id"
#define DELETETAG3 \
    "DELETE FROM FilePaths WHERE id = :file_id"

#define UPDATEFILEPATH \
    "UPDATE FilePaths SET file_path = :newFilePath WHERE file_path = :oldFilePath"

#define UPDATEFILEINFO1 \
    "UPDATE FilePaths SET expiration_date = :expiration_date, " \
    "reminder_time = :reminder_time, interval_time = :interval_time, " \
    "notify_display_time = :notify_display_time, last_reminder_index = :last_reminder_index WHERE file_path = :file_path"
#define UPDATEFILEINFO2 \
    "SELECT id FROM FilePaths WHERE file_path = :file_path"
#define UPDATEFILEINFO3 \
    "INSERT OR REPLACE INTO Tags (file_id, tag_name) VALUES (:file_id, :tag)"
#define UPDATEFILEINFO4 \
    "UPDATE Annotations SET annotation = :annotation WHERE file_id = :file_id"
#define UPDATE_LAST_REMINDER_INDEX \
    "UPDATE FilePaths SET last_reminder_index = :last_reminder_index WHERE file_path = :file_path"
#define HASTAGSFORFILE \
    "SELECT COUNT(*) FROM FilePaths WHERE file_path = :filePath"
#define GETALLTAGS \
    "SELECT DISTINCT tag_name FROM Tags"
#define GETFILEPATHSBYTAG1 \
    "SELECT DISTINCT fp.file_path, t.tag_name, fp.expiration_date, a.annotation, " \
    "fp.reminder_time, fp.interval_time, fp.notify_display_time, fp.last_reminder_index " \
    "FROM FilePaths fp " \
    "LEFT JOIN Tags t ON fp.id = t.file_id " \
    "LEFT JOIN Annotations a ON fp.id = a.file_id" 
#define GETFILEPATHSBYTAG2 \
    "SELECT DISTINCT fp.file_path, t.tag_name, fp.expiration_date, a.annotation, " \
    "fp.reminder_time, fp.interval_time, fp.notify_display_time, fp.last_reminder_index " \
    "FROM FilePaths fp " \
    "LEFT JOIN Tags t ON fp.id = t.file_id " \
    "LEFT JOIN Annotations a ON fp.id = a.file_id " \
    "WHERE t.tag_name = :tag"

#define GETTAGS \
    "SELECT tag_name FROM Tags WHERE file_id = :fileId"

#define GETANNOTATION \
    "SELECT annotation FROM Annotations WHERE file_id = :fileId"
#define GETFILEINFOBYFILEPATH1 \
    "SELECT expiration_date, reminder_time, interval_time, notify_display_time, last_reminder_index FROM FilePaths WHERE file_path = :file_path"
#define GETFILEINFOBYFILEPATH2 \
    "SELECT tag_name FROM Tags WHERE file_id = (SELECT id FROM FilePaths WHERE file_path = :file_path)"
#define GETFILEINFOBYFILEPATH3 \
    "SELECT annotation FROM Annotations WHERE file_id = (SELECT id FROM FilePaths WHERE file_path = :file_path)"

struct FilePathInfo {
    QString filePath;
    QString tagName;
    QDateTime expirationDate;
    QString annotation;
    int reminderTime; // 提前多久提醒(小时)
    int intervalTime; // 每次提醒间隔时间(分钟)
    QTime notifyDisplayTime;
    int lastReminderIndex; // 已成功提醒的最大索引
};

class dbFilepath : public dbManager {
public:
    dbFilepath(const QString &dbName);
    ~dbFilepath();


    bool addFilePath(const QString &filePath, int &fileId);
    bool getFileId(const QString &filePath, int &fileId);
    QStringList getAllFilePaths();
    QList<FilePathInfo> searchFiles(const QString &keyword);

    // QList<QPair<QString, QString>> searchFiles(const QString &keyword);


    bool saveTags(int fileId, const QString &tags);
    bool deleteTag(int fileId);
    bool updateFileInfo(const FilePathInfo& fileInfo);
    bool updateFilePath(const QString &newFilePath, const QString &oldFilePath);

    bool saveAnnotation(int fileId, const QString &annotation);
    void saveExpirationDate(int fileId, const QDateTime &expirationDateTime);

    bool hasTagsForFile(const QString &filePath) const;
    QStringList getAllTags();
    QList<FilePathInfo> getFilePathsByTag(const QString &tag);

    QList<FilePathInfo> getSortByExp();
    bool getAnnotation(int fileId, QString &annotation);

    bool getTags(int fileId, QStringList &tags);

    bool getFileInfoByFilePath(const QString &filePath, FilePathInfo &fileInfo);
    bool updateLastReminderIndex(const QString &filePath, int index);
};


#endif // DBFILEPATH_H
