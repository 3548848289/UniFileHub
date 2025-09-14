#include "RecentFilesManager.h"

RecentFilesManager::RecentFilesManager(QObject *parent) : QObject(parent) {
    loadHistory();
}

RecentFilesManager::~RecentFilesManager() {
    saveHistory();
}

void RecentFilesManager::addFile(const QString &filePath) {
    if (filePath.isEmpty()) return;

    fileHistory.removeAll(filePath);
    fileHistory.prepend(filePath);

    if (fileHistory.size() > maxRecentFiles)
        fileHistory.removeLast();

    saveHistory();
    updateMenu();
}

void RecentFilesManager::loadHistory() {
    QSettings settings("settings.ini", QSettings::IniFormat);
    int size = settings.beginReadArray("fileHistory");
    fileHistory.clear();
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString path = settings.value("path").toString();
        if (!path.isEmpty())
            fileHistory.append(path);
    }
    settings.endArray();
}

void RecentFilesManager::saveHistory() {
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.beginWriteArray("fileHistory");
    for (int i = 0; i < fileHistory.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("path", fileHistory[i]);
    }
    settings.endArray();
}

void RecentFilesManager::populateRecentFilesMenu(QMenu *menu) {
    recentMenu = menu;
    updateMenu();
}

void RecentFilesManager::updateMenu() {
    if (!recentMenu) return;

    recentMenu->clear();

    for (const QString &filePath : std::as_const(fileHistory)) {
        QAction *action = new QAction(filePath, recentMenu);
        action->setToolTip(filePath);
        connect(action, &QAction::triggered, this, [this, filePath]() {
            if (QFile::exists(filePath)) {
                emit fileOpened(filePath);
            } else {
                QMessageBox::warning(
                    nullptr,tr(""),tr("文件 %1 已不存在，可能已被清理").arg(filePath)
                );
                fileHistory.removeAll(filePath);
                saveHistory();

                updateMenu();
            }
        });

        recentMenu->addAction(action);
    }
}
