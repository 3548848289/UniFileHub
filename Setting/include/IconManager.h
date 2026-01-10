#pragma once
#include <QIcon>
#include <QSize>

class IconManager
{
public:
    enum class Icon {
        File,
        Folder,
        Download,
        Delete,
        Rename,
        Move,
        FileSystem,
        Clipboard,
        OnlineDoc,
        FileBackup,
        More,
        Tag,
        Drawing,
        Login,
        Drive,
        Email
    };

    static QIcon icon(Icon type, QSize size);
    static void clearCache();
};
