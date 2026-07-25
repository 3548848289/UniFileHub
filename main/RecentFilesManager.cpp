#include "RecentFilesManager.h"
#include "../Setting/include/SettingManager.h"

RecentFilesManager::RecentFilesManager(QObject *parent) : QObject(parent) {
    loadHistory();
}

RecentFilesManager::~RecentFilesManager() {
    saveHistory();
}

void RecentFilesManager::addFile(const QString &filePath) {
    const QString normalized = normalizedPath(filePath);
    if (normalized.isEmpty()) return;

    fileHistory.removeAll(normalized);
    fileHistory.prepend(normalized);

    if (fileHistory.size() > maxRecentFiles)
        fileHistory.removeLast();

    saveHistory();
    updateMenu();
}

void RecentFilesManager::addFolder(const QString &folderPath) {
    const QString normalized = normalizedPath(folderPath);
    if (normalized.isEmpty()) return;

    folderHistory.removeAll(normalized);
    folderHistory.prepend(normalized);

    if (folderHistory.size() > maxRecentFiles)
        folderHistory.removeLast();

    saveHistory();
    updateMenu();
}

void RecentFilesManager::loadHistory() {
    QSettings settings(SettingManager::getSettingsFilePath(), QSettings::IniFormat);
    int size = settings.beginReadArray("fileHistory");
    fileHistory.clear();
    folderHistory.clear();
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        const QString path = normalizedPath(settings.value("path").toString());
        if (path.isEmpty()) {
            continue;
        }

        const QFileInfo fileInfo(path);
        if (fileInfo.exists() && fileInfo.isDir()) {
            if (!folderHistory.contains(path)) {
                folderHistory.append(path);
            }
        } else if (!fileHistory.contains(path)) {
            fileHistory.append(path);
        }
    }
    settings.endArray();

    size = settings.beginReadArray("folderHistory");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        const QString path = normalizedPath(settings.value("path").toString());
        if (!path.isEmpty() && !folderHistory.contains(path)) {
            folderHistory.append(path);
        }
    }
    settings.endArray();

    while (fileHistory.size() > maxRecentFiles) {
        fileHistory.removeLast();
    }
    while (folderHistory.size() > maxRecentFiles) {
        folderHistory.removeLast();
    }
}

void RecentFilesManager::saveHistory() {
    QSettings settings(SettingManager::getSettingsFilePath(), QSettings::IniFormat);
    settings.remove("fileHistory");
    settings.remove("folderHistory");

    settings.beginWriteArray("fileHistory");
    for (int i = 0; i < fileHistory.size() && i < maxRecentFiles; ++i) {
        settings.setArrayIndex(i);
        settings.setValue("path", fileHistory[i]);
    }
    settings.endArray();

    settings.beginWriteArray("folderHistory");
    for (int i = 0; i < folderHistory.size() && i < maxRecentFiles; ++i) {
        settings.setArrayIndex(i);
        settings.setValue("path", folderHistory[i]);
    }
    settings.endArray();
    settings.sync();
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
            const QFileInfo fileInfo(filePath);
            if (fileInfo.exists() && fileInfo.isFile()) {
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

    if (!fileHistory.isEmpty() && !folderHistory.isEmpty()) {
        recentMenu->addSeparator();
    }

    for (const QString &folderPath : std::as_const(folderHistory)) {
        QAction *action = new QAction(folderPath, recentMenu);
        action->setToolTip(folderPath);
        connect(action, &QAction::triggered, this, [this, folderPath]() {
            const QFileInfo folderInfo(folderPath);
            if (folderInfo.exists() && folderInfo.isDir()) {
                emit folderOpened(folderPath);
            } else {
                QMessageBox::warning(
                    nullptr, tr(""), tr("文件夹 %1 已不存在，可能已被清理").arg(folderPath)
                );
                folderHistory.removeAll(folderPath);
                saveHistory();

                updateMenu();
            }
        });

        recentMenu->addAction(action);
    }
}

QString RecentFilesManager::normalizedPath(const QString &path) const {
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    const QFileInfo fileInfo(trimmed);
    if (fileInfo.exists()) {
        const QString canonicalPath = fileInfo.canonicalFilePath();
        if (!canonicalPath.isEmpty()) {
            return QDir::toNativeSeparators(canonicalPath);
        }
    }

    return QDir::toNativeSeparators(QDir::cleanPath(trimmed));
}
