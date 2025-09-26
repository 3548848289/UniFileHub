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
    QString content;
    bool isPinned;
};

class dbClipboard : public dbManager
{
public:
    dbClipboard(const QString &dbName);
    QList<QString> loadRecentHistory(int hours);
    bool insertClipboardItem(const QString &content, const QString &type, const QDateTime &timestamp, const QString &source, bool isFavorite, bool isPinned, const QString &tags, const QString &format);
    bool setHistory(const QString &content, bool isPinned);
    bool updatePinnedStatus(const QString &content, bool isPinned);
    QList<DbClipRecord> loadRecentNormalHistory(int hours);
    QList<DbClipRecord> loadPinnedHistory();
};

#endif // DBCLIPBOARD_H
