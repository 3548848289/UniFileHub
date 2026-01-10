#pragma once
#include <Qt>

namespace DriveRoles {
    constexpr int RoleIcon = Qt::UserRole + 1;   // 图标枚举
    constexpr int RoleText = Qt::UserRole + 2;   // 文件名
    constexpr int RoleId   = Qt::UserRole + 3;   // itemId
    constexpr int RoleIsDir= Qt::UserRole + 4;   // 是否文件夹
}
