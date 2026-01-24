#include "dbClipboard.h"

dbClipboard::dbClipboard(const QString &dbName) : dbManager(dbName) { }

int dbClipboard::setHistory(const QString &content, bool isPinned)
{
    QSqlQuery query(dbsqlite);
    query.prepare(R"(
        INSERT INTO clipboard_history (content, is_pinned)
        VALUES (:content, :is_pinned)
    )");

    query.bindValue(":content", content);
    query.bindValue(":is_pinned", isPinned ? 1 : 0);

    if (!query.exec()) {
        qDebug() << "插入失败:" << query.lastError().text();
        return -1; // 返回-1表示失败
    }
    
    // 返回新插入记录的ID
    return query.lastInsertId().toInt();
}


// 获取最近 X 小时的普通项（非置顶）
QList<DbClipRecord> dbClipboard::loadRecentNormalHistory(int hours) {
    QList<DbClipRecord> list;
    QSqlQuery query(dbsqlite);
    query.prepare(R"(
        SELECT id, content, is_pinned FROM clipboard_history
        WHERE is_pinned = 0 AND datetime(timestamp) >= datetime('now', ?)
        ORDER BY timestamp DESC
    )");
    QString timeFilter = QString("-%1 hour").arg(hours);
    query.bindValue(0, timeFilter);
    query.exec();

    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toBool()});
    }
    return list;
}

// 获取所有置顶项
QList<DbClipRecord> dbClipboard::loadPinnedHistory() {
    QList<DbClipRecord> list;
    QSqlQuery query(dbsqlite);
    query.prepare(R"(
        SELECT id, content, is_pinned FROM clipboard_history
        WHERE is_pinned = 1
        ORDER BY timestamp DESC
    )");
    query.exec();

    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toBool()});
    }
    return list;
}

bool dbClipboard::insertClipboardItem(const QString &content,
                                      const QString &type,
                                      const QDateTime &timestamp,
                                      const QString &source,
                                      bool isFavorite,
                                      bool isPinned,
                                      const QString &tags,
                                      const QString &format) {
    QSqlQuery query(dbsqlite);
    query.prepare(R"(
        INSERT INTO clipboard_history (
            content, type, timestamp, source, is_favorite, is_pinned, tags, format
        ) VALUES (
            :content, :type, :timestamp, :source, :is_favorite, :is_pinned, :tags, :format
        )
    )");

    query.bindValue(":content", content);
    query.bindValue(":type", type);
    query.bindValue(":timestamp", timestamp.toString(Qt::ISODate));
    query.bindValue(":source", source);
    query.bindValue(":is_favorite", isFavorite ? 1 : 0);
    query.bindValue(":is_pinned", isPinned ? 1 : 0);
    query.bindValue(":tags", tags);
    query.bindValue(":format", format);

    if (!query.exec()) {
        qDebug() << "插入失败:" << query.lastError().text();
        return false;
    }
    return true;
}

// 新增：根据ID更新置顶状态
bool dbClipboard::updatePinnedStatusById(int id, bool isPinned)
{
    QSqlQuery query(dbsqlite);

    query.prepare(R"(
        UPDATE clipboard_history
        SET is_pinned = :is_pinned
        WHERE id = :id
    )");

    query.bindValue(":is_pinned", isPinned ? 1 : 0);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "根据ID更新置顶状态失败:" << query.lastError().text();
        return false;
    }

    return true;
}

// 保留旧方法以保持兼容性
bool dbClipboard::updatePinnedStatus(const QString &content, bool isPinned)
{
    QSqlQuery query(dbsqlite);

    query.prepare(R"(
        UPDATE clipboard_history
        SET is_pinned = :is_pinned
        WHERE content = :content
    )");

    query.bindValue(":is_pinned", isPinned ? 1 : 0);
    query.bindValue(":content", content);

    if (!query.exec()) {
        qDebug() << "更新置顶状态失败:" << query.lastError().text();
        return false;
    }

    return true;
}

// 新增：清除所有历史记录
bool dbClipboard::clearAllHistory()
{
    QSqlQuery query(dbsqlite);

    query.prepare(R"(
        DELETE FROM clipboard_history
    )");

    if (!query.exec()) {
        qDebug() << "清除所有历史记录失败:" << query.lastError().text();
        return false;
    }

    return true;
}
