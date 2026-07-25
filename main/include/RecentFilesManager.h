#ifndef RECENTFILESMANAGER_H
#define RECENTFILESMANAGER_H

#include <QObject>
#include <QStringList>
#include <QSettings>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include "../../Setting/include/SettingManager.h"

class RecentFilesManager : public QObject {
    Q_OBJECT
public:
    explicit RecentFilesManager(QObject *parent = nullptr);
    ~RecentFilesManager();

    void addFile(const QString &filePath);
    void addFolder(const QString &folderPath);
    void populateRecentFilesMenu(QMenu *menu);
signals:
    void fileOpened(const QString &filePath);
    void folderOpened(const QString &folderPath);

private:
    void loadHistory();
    void saveHistory();
    void updateMenu();
    QString normalizedPath(const QString &path) const;

    QStringList fileHistory;
    QStringList folderHistory;
    QMenu* recentMenu = nullptr;
    const int maxRecentFiles = 15;
};
#endif // RECENTFILESMANAGER_H
