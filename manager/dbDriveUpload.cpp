#include "dbDriveUpload.h"
#include <QSqlQuery>
#include <QDebug>

// 表名
const QString TABLE_NAME = "drive_upload_records";

dbDriveUpload::dbDriveUpload(const QString &dbName) : dbManager(dbName) {
    // 确保表存在
    QSqlQuery query(dbsqlite);
    QString createTableSql = QString(R"(
        CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            file_name TEXT NOT NULL,
            cloud_file_name TEXT,
            file_size INTEGER NOT NULL,
            local_path TEXT NOT NULL,
            upload_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            upload_status TEXT DEFAULT 'success',
            file_type TEXT,
            parent_id INTEGER DEFAULT 0
        );
    )").arg(TABLE_NAME);
    
    if (!query.exec(createTableSql)) {
        qDebug() << "创建上传记录表失败:" << query.lastError().text();
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
            qDebug() << "新增上传云端文件名字段失败:" << alterQuery.lastError().text();
        } else {
            canMigrateCloudFileName = true;
        }
    }
    if (canMigrateCloudFileName) {
        QSqlQuery migrateQuery(dbsqlite);
        migrateQuery.exec(QString("UPDATE %1 SET cloud_file_name = file_name WHERE cloud_file_name IS NULL OR cloud_file_name = ''").arg(TABLE_NAME));
    }
}

bool dbDriveUpload::addUploadRecord(const DriveUploadRecord &record) {
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString(R"(
        INSERT INTO %1 (file_id, file_name, cloud_file_name, file_size, local_path, upload_time, upload_status, file_type, parent_id)
        VALUES (:file_id, :file_name, :cloud_file_name, :file_size, :local_path, :upload_time, :upload_status, :file_type, :parent_id)
    )").arg(TABLE_NAME));
    
    query.bindValue(":file_id", record.fileId);
    query.bindValue(":file_name", record.fileName);
    query.bindValue(":cloud_file_name", record.cloudFileName);
    query.bindValue(":file_size", record.fileSize);
    query.bindValue(":local_path", record.localPath);
    query.bindValue(":upload_time", record.uploadTime.toString(Qt::ISODate));
    query.bindValue(":upload_status", record.uploadStatus);
    query.bindValue(":file_type", record.fileType);
    query.bindValue(":parent_id", record.parentId);
    
    if (!query.exec()) {
        qDebug() << "添加上传记录失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QList<DriveUploadRecord> dbDriveUpload::getUploadHistory(int limit) {
    QList<DriveUploadRecord> records;
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString(R"(
        SELECT id, file_id, file_name, cloud_file_name, file_size, local_path, upload_time, upload_status, file_type, parent_id
        FROM %1
        ORDER BY upload_time DESC, id DESC
        LIMIT :limit
    )").arg(TABLE_NAME));
    
    query.bindValue(":limit", limit);
    
    if (!query.exec()) {
        qDebug() << "获取上传历史失败:" << query.lastError().text();
        return records;
    }
    
    while (query.next()) {
        DriveUploadRecord record;
        record.id = query.value(0).toInt();
        record.fileId = query.value(1).toInt();
        record.fileName = query.value(2).toString();
        record.cloudFileName = query.value(3).toString();
        if (record.cloudFileName.isEmpty()) {
            record.cloudFileName = record.fileName;
        }
        record.fileSize = query.value(4).toLongLong();
        record.localPath = query.value(5).toString();
        record.uploadTime = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
        record.uploadStatus = query.value(7).toString();
        record.fileType = query.value(8).toString();
        record.parentId = query.value(9).toInt();
        records.append(record);
    }
    
    return records;
}

bool dbDriveUpload::clearUploadHistory() {
    QSqlQuery query(dbsqlite);
    
    if (!query.exec(QString("DELETE FROM %1").arg(TABLE_NAME))) {
        qDebug() << "清空上传历史失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool dbDriveUpload::deleteUploadRecord(int id) {
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString("DELETE FROM %1 WHERE id = :id").arg(TABLE_NAME));
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        qDebug() << "删除上传记录失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool dbDriveUpload::updateUploadStatus(int id, const QString &status) {
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString("UPDATE %1 SET upload_status = :status WHERE id = :id").arg(TABLE_NAME));
    query.bindValue(":status", status);
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        qDebug() << "更新上传状态失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool dbDriveUpload::updateUploadResult(int id, int fileId, const QString &cloudFileName, const QString &status) {
    QSqlQuery query(dbsqlite);

    query.prepare(QString(R"(
        UPDATE %1
        SET file_id = :file_id, cloud_file_name = :cloud_file_name, upload_status = :status
        WHERE id = :id
    )").arg(TABLE_NAME));
    query.bindValue(":file_id", fileId);
    query.bindValue(":cloud_file_name", cloudFileName);
    query.bindValue(":status", status);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "更新上传结果失败:" << query.lastError().text();
        return false;
    }

    return true;
}

int dbDriveUpload::getRecordIdByLocalPath(const QString &localPath) {
    QSqlQuery query(dbsqlite);
    
    query.prepare(QString("SELECT id FROM %1 WHERE local_path = :localPath ORDER BY id DESC LIMIT 1").arg(TABLE_NAME));
    query.bindValue(":localPath", localPath);
    
    if (!query.exec()) {
        qDebug() << "查询记录ID失败:" << query.lastError().text();
        return -1;
    }
    
    if (query.next()) {
        return query.value(0).toInt();
    }
    
    return -1;
}
