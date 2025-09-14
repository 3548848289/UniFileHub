#ifndef RECENTFILESMANAGER_H
#define RECENTFILESMANAGER_H

#include <QObject>
#include <QStringList>
#include <QSettings>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QFileInfo>
#include "../../Setting/include/SettingManager.h"

class RecentFilesManager : public QObject {
    Q_OBJECT
public:
    explicit RecentFilesManager(QObject *parent = nullptr);
    ~RecentFilesManager();

    void addFile(const QString &filePath);
    void populateRecentFilesMenu(QMenu *menu);
signals:
    void fileOpened(const QString &filePath);

private:
    void loadHistory();
    void saveHistory();
    void updateMenu();

    QStringList fileHistory;
    QMenu* recentMenu = nullptr;
    const int maxRecentFiles = 10;
};
#endif // RECENTFILESMANAGER_H
