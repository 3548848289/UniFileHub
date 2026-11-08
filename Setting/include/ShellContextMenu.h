#ifndef SHELLCONTEXTMENU_H
#define SHELLCONTEXTMENU_H

#include <QString>

// 系统右键菜单的统一注册与注销。
// Windows：全部菜单项（打开/文件备份/备忘标签/上传网盘/打开文件夹）注册到注册表，
//   文件项在 HKCU\Software\Classes\*\shell，文件夹项在 ...\Directory\shell，
//   每个 Menu 项的 command 带动作标识（open/backup/tag/upload/openfolder）。
// Linux：在 ~/.local/share/applications 生成/删除带 MimeType 的 .desktop 条目
//   （仅"打开/打开文件夹"两个标准动作），进入文件管理器的"用其他应用打开"列表。
// 均无需管理员权限，开关状态与设置面板共用同一批 settings.ini 键。
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
