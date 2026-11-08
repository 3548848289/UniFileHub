#ifndef SHELLCONTEXTMENU_H
#define SHELLCONTEXTMENU_H

#include <QString>

// Windows 资源管理器右键菜单的统一注册与注销。
// 全部菜单项（打开/文件备份/备忘标签/上传网盘/打开文件夹）在
// ShellContextMenu.cpp 的 kVerbs 表中描述：注册表父键、子键名、菜单文字、
// settings.ini 开关键、命令行动作标识。文件项注册在 HKCU\Software\Classes\*\shell，
// 文件夹项注册在 HKCU\Software\Classes\Directory\shell，均无需管理员权限。
namespace ShellContextMenu {

enum class Verb {
    Open,     // 用 UniFileHub 打开（文件）
    Backup,   // 文件备份
    Tag,      // 备忘标签
    Upload,   // 上传网盘
    Folder,   // 打开文件夹（右键文件夹，定位文件系统面包屑）
};

// 动词对应的 settings.ini 开关键
QString settingKey(Verb verb);

// 动词对应的命令行动作标识（"open"/"backup"/"tag"/"upload"）
QString cliToken(Verb verb);

// 程序启动时按设置同步注册表：勾选的注册，未勾选的清理残留
void applySettings();

// 设置开关切换时调用，立即注册(true)/注销(false)单个菜单项
void setVerbEnabled(Verb verb, bool enabled);

}

#endif // SHELLCONTEXTMENU_H
