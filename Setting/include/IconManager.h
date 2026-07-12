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
        Location,
        Preview,
        FileSystem,
        Clipboard,
        OnlineDoc,
        FileBackup,
        More,
        Tag,
        Drawing,
        Login,
        Drive,
        Email,
        MenuDownload,
        MenuFileClose,
        MenuFileSave,
        MenuHelp,
        MenuFileBackup,
        MenuFileOpen,
        MenuNew,
        MenuSearch,
        MenuSettings,
        Pin,
        Cloud,
        Close
    };

    static QIcon icon(Icon type, QSize size, QColor color = QColor());
    static void clearCache();
    static void setDefaultIconColor(const QColor &color);
    static QColor defaultIconColor();
};
