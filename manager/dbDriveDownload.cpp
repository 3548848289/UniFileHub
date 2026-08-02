#include "dbDriveDownload.h"

#include <QSqlQuery>
#include <QDebug>
#include <QFileInfo>

// 表名
const QString TABLE_NAME = "drive_download_history";

dbDriveDownload::dbDriveDownload(const QString &dbName) : dbManager(dbName) {
    // 确保表存在
    QSqlQuery query(dbsqlite);
    QString createTableSql = QString(R"(
        CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            file_name TEXT NOT NULL,
            cloud_file_name TEXT,
            file_size INTEGER DEFAULT 0,
            save_path TEXT NOT NULL,
            download_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            download_status TEXT DEFAULT 'success',
            file_type TEXT
        );
    )").arg(TABLE_NAME);
    
    if (!query.exec(createTableSql)) {
        qDebug() << "创建下载历史表失败:" << query.lastError().text();
    }

    QSqlQuery columnQuery(dbsqlite);
    bool hasCloudFileName = false;
    if (columnQuery.exec(QString("PRAGMA table_info(%1)").arg(TABLE_NAME))) {
        while (columnQuery.next()) {
            if (columnQuery.value(1).toString() == QStringLiteral("cloud_file_name")) {
                hasCloudFileName = true;
                break;
            }
        }
    }

    bool canMigrateCloudFileName = hasCloudFileName;
    if (!hasCloudFileName) {
        QSqlQuery alterQuery(dbsqlite);
        if (!alterQuery.exec(QString("ALTER TABLE %1 ADD COLUMN cloud_file_name TEXT").arg(TABLE_NAME))) {
            qDebug() << "新增云端文件名字段失败:" << alterQuery.lastError().text();
        } else {
            canMigrateCloudFileName = true;
        }
    }
    if (canMigrateCloudFileName) {
        QSqlQuery migrateQuery(dbsqlite);
        migrateQuery.exec(QString("UPDATE %1 SET cloud_file_name = file_name WHERE cloud_file_name IS NULL OR cloud_file_name = ''").arg(TABLE_NAME));
    }
}

bool dbDriveDownload::addDownloadRecord(const DriveDownloadRecord &record) {
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString(R"(
        INSERT INTO %1 (file_id, file_name, cloud_file_name, file_size, save_path, download_time, download_status, file_type)
        VALUES (:file_id, :file_name, :cloud_file_name, :file_size, :save_path, :download_time, :download_status, :file_type)
    )").arg(TABLE_NAME));
    
    query.bindValue(":file_id", record.fileId);
    query.bindValue(":file_name", record.fileName);
    query.bindValue(":cloud_file_name", record.cloudFileName);
    query.bindValue(":file_size", record.fileSize);
    query.bindValue(":save_path", record.savePath);
    query.bindValue(":download_time", record.downloadTime.toString(Qt::ISODate));
    query.bindValue(":download_status", record.downloadStatus);
    query.bindValue(":file_type", record.fileType);
    
    if (!query.exec()) {
        qDebug() << "添加下载记录失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QList<DriveDownloadRecord> dbDriveDownload::getDownloadHistory(int limit) {
    QList<DriveDownloadRecord> records;
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString(R"(
        SELECT id, file_id, file_name, cloud_file_name, file_size, save_path, download_time, download_status, file_type
        FROM %1
        ORDER BY download_time DESC, id DESC
        LIMIT :limit
    )").arg(TABLE_NAME));
    
    query.bindValue(":limit", limit);
    
    if (!query.exec()) {
        qDebug() << "获取下载历史失败:" << query.lastError().text();
        return records;
    }
    
    while (query.next()) {
        DriveDownloadRecord record;
        record.id = query.value(0).toInt();
        record.fileId = query.value(1).toInt();
        record.fileName = query.value(2).toString();
        record.cloudFileName = query.value(3).toString();
        const QString storedFileName = record.fileName;
        record.fileSize = query.value(4).toLongLong();
        record.savePath = query.value(5).toString();
        record.downloadTime = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
        record.downloadStatus = query.value(7).toString();
        record.fileType = query.value(8).toString();

        if (record.cloudFileName.isEmpty()) {
            record.cloudFileName = storedFileName;
        }
        const QString localFileName = QFileInfo(record.savePath).fileName();
        if (!localFileName.isEmpty()) {
            record.fileName = localFileName;
        }
        records.append(record);
    }
    
    return records;
}

bool dbDriveDownload::clearDownloadHistory() {
    QSqlQuery query(dbsqlite);
    
    if (!query.exec(QString("DELETE FROM %1").arg(TABLE_NAME))) {
        qDebug() << "清空下载历史失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool dbDriveDownload::deleteDownloadRecord(int id) {
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString("DELETE FROM %1 WHERE id = :id").arg(TABLE_NAME));
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        qDebug() << "删除下载记录失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool dbDriveDownload::updateDownloadStatus(int id, const QString &status) {
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString("UPDATE %1 SET download_status = :status WHERE id = :id").arg(TABLE_NAME));
    query.bindValue(":status", status);
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        qDebug() << "更新下载状态失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

int dbDriveDownload::getRecordIdBySavePath(const QString &savePath) {
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString("SELECT id FROM %1 WHERE save_path = :savePath ORDER BY id DESC LIMIT 1").arg(TABLE_NAME));
    query.bindValue(":savePath", savePath);
    
    if (!query.exec()) {
        qDebug() << "查询记录ID失败:" << query.lastError().text();
        return -1;
    }
    
    if (query.next()) {
        return query.value(0).toInt();
    }
    
    return -1;
}
