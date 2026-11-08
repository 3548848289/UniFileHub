#include "include/ShellContextMenu.h"
#include "include/SettingManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QDebug>

#ifdef Q_OS_WIN

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

#else // !Q_OS_WIN

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

#endif // Q_OS_WIN
