#ifndef DBCLIPBOARD_H
#define DBCLIPBOARD_H

#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDateTime>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "dbManager.h"

struct DbClipRecord {
    int id;
    QString content;
    bool isPinned;
};

class dbClipboard : public dbManager
{
public:
    dbClipboard(const QString &dbName);
    QList<QString> loadRecentHistory(int hours);
    bool insertClipboardItem(const QString &content, const QString &type, const QDateTime &timestamp, const QString &source, bool isFavorite, bool isPinned, const QString &tags, const QString &format);
    // 修改返回类型为int，返回插入的ID，-1表示失败
    int setHistory(const QString &content, bool isPinned = false);
    // 新增：根据ID更新置顶状态
    bool updatePinnedStatusById(int id, bool isPinned);
    // 保留旧方法以保持兼容性
    bool updatePinnedStatus(const QString &content, bool isPinned);
    QList<DbClipRecord> loadRecentNormalHistory(int hours);
    QList<DbClipRecord> loadPinnedHistory();
    // 新增：清除所有历史记录
    bool clearAllHistory();
};

#endif // DBCLIPBOARD_H
