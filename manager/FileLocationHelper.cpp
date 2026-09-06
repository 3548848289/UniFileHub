#include "include/FileLocationHelper.h"
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QSysInfo>
#include <QMap>
#include "../../Setting/include/SettingManager.h"

namespace {

QString managerKey() { return QStringLiteral("FileManager/PreferredCommand"); }

} // namespace

// Linux 下按优先级排列的文件管理器候选。
// 国产系统：UOS/deepin 用 dde-file-manager，麒麟(KOS)用 peony。
// 列表顺序只影响第一次探测，之后由配置缓存和 xdg-mime 查询结果决定。
const QMap<QString, FileLocationHelper::ArgMode>& FileLocationHelper::linuxManagers()
{
    static const QMap<QString, ArgMode> managers = {
        {"dde-file-manager", ArgMode::ShowItem},
        {"peony",            ArgMode::ShowItem},
        {"nautilus",         ArgMode::SelectFlag},
        {"dolphin",          ArgMode::SelectFlag},
        {"nemo",             ArgMode::ParentOpen},
        {"thunar",           ArgMode::ParentOpen},
    };
    return managers;
}

bool FileLocationHelper::openFileLocationWithSelection(const QString& filePath)
{
    if (!fileExists(filePath)) {
        return false;
    }

    QString nativeFilePath = QDir::toNativeSeparators(filePath);

    if (QSysInfo::productType() == "windows") {
        // explorer.exe 在 Windows 上必然存在，无需探测
        return QProcess::startDetached("explorer.exe", {"/select,", nativeFilePath});
    }

    // 非 Windows（Ubuntu / UOS / KOS 等）：不再按系统名猜资源管理器，
    // 而是优先用上次成功的，其次 xdg-mime 查系统默认，最后按固定优先级逐个试。
    // startDetached 返回 false 说明程序不存在（什么都没打开），可以安全尝试下一个。
    QStringList tried;
    QStringList ordered;

    const QString preferred = preferredManager();
    if (!preferred.isEmpty())
        ordered.append(preferred);

    const QString xdgDefault = queryXdgDefaultFileManager();
    if (!xdgDefault.isEmpty() && !ordered.contains(xdgDefault))
        ordered.append(xdgDefault);

    const QStringList candidates = candidatePrograms();
    for (const QString& program : candidates) {
        if (!ordered.contains(program))
            ordered.append(program);
    }

    for (const QString& program : ordered) {
        if (tried.contains(program))
            continue;
        tried.append(program);

        const ArgMode mode = argModeForProgram(program);
        const QStringList args = buildArgs(program, mode, nativeFilePath);

        if (QProcess::startDetached(program, args)) {
            // 只有“真正支持选中/定位”的管理器才值得记住；
            // xdg-open 等通用兜底不缓存，下次继续探测
            if (linuxManagers().contains(program))
                setPreferredManager(program);
            return true;
        }
    }

    // 全部失败：至少打开所在目录
    return QDesktopServices::openUrl(QUrl::fromLocalFile(getFolderPath(filePath)));
}

QStringList FileLocationHelper::candidatePrograms()
{
    return linuxManagers().keys();
}

// 通过 xdg-mime 查询系统默认的目录处理程序，返回可直接启动的程序名。
// 查不到（无 xdg-mime、超时、未知 .desktop）返回空串。
QString FileLocationHelper::queryXdgDefaultFileManager()
{
    QProcess process;
    process.start("xdg-mime", {"query", "default", "inode/directory"});
    if (!process.waitForFinished(2000))
        return QString();

    const QString desktopId = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    if (desktopId.isEmpty() || !desktopId.endsWith(QLatin1String(".desktop")))
        return QString();

    // "org.gnome.Nautilus.desktop" -> "nautilus"，"dde-file-manager.desktop" -> "dde-file-manager"
    const QString baseName = desktopId.section('.', 0, -2).section('.', -1).toLower();
    return linuxManagers().contains(baseName) ? baseName : QString();
}

FileLocationHelper::ArgMode FileLocationHelper::argModeForProgram(const QString& program)
{
    auto it = linuxManagers().find(program);
    if (it != linuxManagers().end())
        return it.value();
    // 未知管理器：只传所在目录，避免误把文件本身当作目录打开
    return ArgMode::ParentOpen;
}

QStringList FileLocationHelper::buildArgs(const QString& program, ArgMode mode, const QString& nativeFilePath)
{
    Q_UNUSED(program);

    switch (mode) {
    case ArgMode::SelectFlag:
        // nautilus / dolphin：--select <file>
        return {"--select", nativeFilePath};
    case ArgMode::ShowItem:
        // dde-file-manager / peony：--show-item(s) <file>
        return {"--show-item", nativeFilePath};
    case ArgMode::ParentOpen:
        // nemo / thunar / 未知：打开所在目录
        return {QDir::toNativeSeparators(QFileInfo(nativeFilePath).absolutePath())};
    }
    return {nativeFilePath};
}

QString FileLocationHelper::preferredManager()
{
    return SettingManager::Instance().value(managerKey()).toString().trimmed();
}

void FileLocationHelper::setPreferredManager(const QString& program)
{
    SettingManager::Instance().setValue(managerKey(), program);
}

bool FileLocationHelper::openFileLocation(const QString& filePath)
{
    if (!fileExists(filePath)) {
        return false;
    }

    QString folderPath = getFolderPath(filePath);
    return QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

bool FileLocationHelper::fileExists(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.exists();
}

QString FileLocationHelper::getFolderPath(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.absolutePath();
}

FileLocationHelper::FileLocationHelper() {}
FileLocationHelper::~FileLocationHelper() {}
