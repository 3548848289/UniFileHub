#include "include/ShellContextMenu.h"
#include "include/SettingManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>

#ifdef Q_OS_WIN

// ============ Windows：注册表右键菜单 ============

namespace {

// 文件项的父键：对任意文件类型生效
const char *kFileShellKey =
    "HKEY_CURRENT_USER\\Software\\Classes\\*\\shell";
// 文件夹项的父键：右键文件夹时生效
const char *kDirectoryShellKey =
    "HKEY_CURRENT_USER\\Software\\Classes\\Directory\\shell";

struct VerbDesc {
    ShellContextMenu::Verb verb;
    const char *rootKey;     // 注册表父键
    const char *subKey;      // 父键下的子键名
    const char *menuText;    // 资源管理器菜单项文字
    const char *settingKey;  // settings.ini 开关键
    const char *cliToken;    // 命令行动作标识，传给 exe 的第一个参数
};

const VerbDesc kVerbs[] = {
    { ShellContextMenu::Verb::Open,   kFileShellKey,      "UniFileHub",        "UniFileHub 打开",       "all_setting/context_menu",         "open"       },
    { ShellContextMenu::Verb::Backup, kFileShellKey,      "UniFileHub_Backup", "UniFileHub 文件备份",   "all_setting/context_menu_backup",  "backup"     },
    { ShellContextMenu::Verb::Tag,    kFileShellKey,      "UniFileHub_Tag",    "UniFileHub 备忘标签",   "all_setting/context_menu_tag",     "tag"        },
    { ShellContextMenu::Verb::Upload, kFileShellKey,      "UniFileHub_Upload", "UniFileHub 上传网盘",   "all_setting/context_menu_upload",  "upload"     },
    { ShellContextMenu::Verb::Folder, kDirectoryShellKey, "UniFileHub",        "UniFileHub 打开文件夹", "all_setting/context_menu_folder",  "openfolder" },
};

const VerbDesc &verbDesc(ShellContextMenu::Verb verb)
{
    for (const VerbDesc &desc : kVerbs) {
        if (desc.verb == verb) {
            return desc;
        }
    }
    return kVerbs[0];
}

bool verbEnabled(const VerbDesc &desc)
{
    return SettingManager::Instance().value(QLatin1String(desc.settingKey), true).toBool();
}

// 命令形如 "C:\...\UniFileHub.exe" "backup" "%1"，
// 资源管理器会把 %1 替换为右键的文件/文件夹路径
void registerVerb(const VerbDesc &desc)
{
    const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString keyPath = QLatin1String(desc.rootKey) + "\\" + QLatin1String(desc.subKey);

    QSettings reg(keyPath, QSettings::NativeFormat);
    // 菜单文字含中文，必须按 UTF-8 解码；源码在 /utf-8 下编译
    reg.setValue(".", QString::fromUtf8(desc.menuText));
    reg.setValue("Icon", "\"" + appPath + "\",0");
    reg.setValue("command/.",
                 "\"" + appPath + "\" \"" + QLatin1String(desc.cliToken) + "\" \"%1\"");
    reg.sync();

    qDebug() << "Register context menu" << desc.subKey << ":" << reg.status();
}

void unregisterVerb(const VerbDesc &desc)
{
    // 在父键上 remove 子键，才会连子键本身一起删除
    QSettings reg(QLatin1String(desc.rootKey), QSettings::NativeFormat);
    reg.remove(QLatin1String(desc.subKey));
    reg.sync();

    qDebug() << "Unregister context menu" << desc.subKey << ":" << reg.status();
}

} // namespace

namespace ShellContextMenu {

QString settingKey(Verb verb)
{
    return QLatin1String(verbDesc(verb).settingKey);
}

QString cliToken(Verb verb)
{
    return QLatin1String(verbDesc(verb).cliToken);
}

void applySettings()
{
    qDebug() << "========== Sync Windows Context Menus ==========";
    for (const VerbDesc &desc : kVerbs) {
        if (verbEnabled(desc)) {
            registerVerb(desc);
        } else {
            unregisterVerb(desc);
        }
    }
    qDebug() << "================================================";
}

void setVerbEnabled(Verb verb, bool enabled)
{
    if (enabled) {
        registerVerb(verbDesc(verb));
    } else {
        unregisterVerb(verbDesc(verb));
    }
}

} // namespace ShellContextMenu

#elif defined(Q_OS_LINUX)

// ============ Linux：用户级 .desktop 桌面条目 ============
//
// 文件管理器（GNOME Files/KDE Dolphin 等）"用其他应用打开"列表来自
// ~/.local/share/applications 下的 .desktop 条目（freedesktop 标准）。
// 每个 .desktop 的 Exec 带上动作标识（open/openfolder），
// 与 Windows 共用同一套命令行解析和动作分发。
// 备份/标签/上传是 Windows 右键菜单专有，Linux 下不注册。

namespace {

struct LinuxVerbDesc {
    ShellContextMenu::Verb verb;
    const char *desktopBaseName; // ~/.local/share/applications/<base>.desktop
    const char *menuText;
    const char *settingKey;
    const char *cliToken;
    const char *mimeType;
};

const LinuxVerbDesc kLinuxVerbs[] = {
    { ShellContextMenu::Verb::Open,
      "unifilehub-open",
      "UniFileHub 打开",
      "all_setting/context_menu",
      "open",
      "text/plain;text/csv;text/markdown;application/json;application/pdf;application/zip;"
      "application/octet-stream;image/png;image/jpeg;image/gif;image/bmp;image/webp;image/svg+xml;"
      "video/mp4;audio/mpeg;application/msword;application/vnd.ms-excel;"
      "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet;"
      "application/vnd.ms-powerpoint;"
      "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
    { ShellContextMenu::Verb::Folder,
      "unifilehub-openfolder",
      "UniFileHub 打开文件夹",
      "all_setting/context_menu_folder",
      "openfolder",
      "inode/directory" },
};

const LinuxVerbDesc *linuxVerbDesc(ShellContextMenu::Verb verb)
{
    for (const LinuxVerbDesc &desc : kLinuxVerbs) {
        if (desc.verb == verb) {
            return &desc;
        }
    }
    return nullptr;
}

QString applicationsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
}

// .desktop 的 Icon 需要真实文件路径，从内嵌资源导出一份到用户数据目录
QString iconFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + QStringLiteral("/unifilehub/icon.png");
}

bool ensureIconFile()
{
    const QString path = iconFilePath();
    if (QFileInfo::exists(path)) {
        return true;
    }
    QFile resource(QStringLiteral(":/conf/UniFileHub.png"));
    if (!resource.open(QIODevice::ReadOnly)) {
        return false;
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile target(path);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    target.write(resource.readAll());
    return true;
}

bool writeDesktopEntry(const LinuxVerbDesc &desc)
{
    if (!ensureIconFile()) {
        qDebug() << "Write desktop entry failed: cannot export icon";
        return false;
    }

    const QString content = QStringLiteral(
                                "[Desktop Entry]\n"
                                "Type=Application\n"
                                "Version=1.0\n"
                                "Name=%1\n"
                                "Exec=\"%2\" %3 %f\n"
                                "Icon=%4\n"
                                "Terminal=false\n"
                                "StartupNotify=true\n"
                                "MimeType=%5\n")
                                .arg(QString::fromUtf8(desc.menuText),
                                     QCoreApplication::applicationFilePath(),
                                     QLatin1String(desc.cliToken),
                                     iconFilePath(),
                                     QLatin1String(desc.mimeType));

    QDir().mkpath(applicationsDir());
    QFile file(applicationsDir() + QLatin1Char('/') + QLatin1String(desc.desktopBaseName)
               + QStringLiteral(".desktop"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "Write desktop entry failed:" << file.fileName() << file.errorString();
        return false;
    }
    file.write(content.toUtf8());
    file.close();

    qDebug() << "Register context menu (Linux):" << file.fileName();
    return file.error() == QFileDevice::NoError;
}

void removeDesktopEntry(const LinuxVerbDesc &desc)
{
    const QString path = applicationsDir() + QLatin1Char('/')
                         + QLatin1String(desc.desktopBaseName) + QStringLiteral(".desktop");
    if (QFile::remove(path)) {
        qDebug() << "Unregister context menu (Linux):" << path;
    }
}

// 让文件管理器刷新"打开方式"列表；工具不存在时静默忽略
void refreshDesktopDatabase()
{
    QProcess::execute(QStringLiteral("update-desktop-database"), { applicationsDir() });
}

} // namespace

namespace ShellContextMenu {

QString settingKey(Verb verb)
{
    const LinuxVerbDesc *desc = linuxVerbDesc(verb);
    return desc ? QLatin1String(desc->settingKey) : QString();
}

QString cliToken(Verb verb)
{
    const LinuxVerbDesc *desc = linuxVerbDesc(verb);
    return desc ? QLatin1String(desc->cliToken) : QString();
}

void applySettings()
{
    for (const LinuxVerbDesc &desc : kLinuxVerbs) {
        const bool enabled = SettingManager::Instance()
                                 .value(QLatin1String(desc.settingKey), true).toBool();
        if (enabled) {
            writeDesktopEntry(desc);
        } else {
            removeDesktopEntry(desc);
        }
    }
    refreshDesktopDatabase();
}

void setVerbEnabled(Verb verb, bool enabled)
{
    const LinuxVerbDesc *desc = linuxVerbDesc(verb);
    if (!desc) {
        return;
    }
    if (enabled) {
        writeDesktopEntry(*desc);
    } else {
        removeDesktopEntry(*desc);
    }
    refreshDesktopDatabase();
}

} // namespace ShellContextMenu

#else

// ============ 其他平台：不支持 ============

namespace ShellContextMenu {

QString settingKey(Verb)
{
    return QString();
}

QString cliToken(Verb)
{
    return QString();
}

void applySettings() {}

void setVerbEnabled(Verb, bool) {}

} // namespace ShellContextMenu

#endif // 平台分支
