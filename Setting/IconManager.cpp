#include "include/IconManager.h"
#include <QApplication>
#include <QStyleHints>
#include <QSvgRenderer>
#include <QPainter>
#include <QHash>

// 静态变量存储默认图标颜色
static QColor g_defaultIconColor = QColor("#7598db");

static QColor currentIconColor()
{
    return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark
               ? Qt::white
               : Qt::black;
}

static QString iconPath(IconManager::Icon icon)
{
    switch (icon) {
    case IconManager::Icon::File: return ":/PersonalDrive/file.svg";
    case IconManager::Icon::Folder: return ":/PersonalDrive/folder.svg";
    case IconManager::Icon::Download: return ":/PersonalDrive/download.svg";
    case IconManager::Icon::Delete:   return ":/PersonalDrive/delete.svg";
    case IconManager::Icon::Rename:   return ":/PersonalDrive/rename.svg";
    case IconManager::Icon::Move:     return ":/PersonalDrive/move.svg";
    case IconManager::Icon::FileSystem: return ":/Main/FileSystem.svg";
    case IconManager::Icon::Clipboard: return ":/Main/Clipboard.svg";
    case IconManager::Icon::OnlineDoc: return ":/Main/OnlineDoc.svg";
    case IconManager::Icon::FileBackup: return ":/Main/FileBackup.svg";
    case IconManager::Icon::More: return ":/Main/More.svg";
    case IconManager::Icon::Tag: return ":/Main/Tag.svg";
    case IconManager::Icon::Drawing: return ":/Main/Drawing.svg";
    case IconManager::Icon::Login: return ":/Main/Login.svg";
    case IconManager::Icon::Drive: return ":/Main/Drive.svg";
    case IconManager::Icon::Email: return ":/Main/Email.svg";
    case IconManager::Icon::MenuDownload: return ":/Menu/download.svg";
    case IconManager::Icon::MenuFileClose: return ":/Menu/file_close.svg";
    case IconManager::Icon::MenuFileSave: return ":/Menu/file_save.svg";
    case IconManager::Icon::MenuHelp: return ":/Menu/help.svg";
    case IconManager::Icon::MenuFileBackup: return ":/Menu/file_backup.svg";
    case IconManager::Icon::MenuFileOpen: return ":/Menu/file_open.svg";
    case IconManager::Icon::MenuNew: return ":/Menu/new.svg";
    case IconManager::Icon::MenuSearch: return ":/Menu/search.svg";
    case IconManager::Icon::MenuSettings: return ":/Menu/settings.svg";
    case IconManager::Icon::Pin: return ":/ClipBoard/pin.svg";
    case IconManager::Icon::Close: return ":/Menu/file_close.svg";
    }
    return {};
}

static QPixmap renderSvg(const QString &file,
                         const QColor &color,
                         QSize logicalSize,
                         bool tint = true)
{
    qreal dpr = qApp->devicePixelRatio();   // 关键

    QSize deviceSize = logicalSize * dpr;   // 真正渲染尺寸

    QSvgRenderer renderer(file);
    if (!renderer.isValid()) {
        QPixmap pm(deviceSize);
        pm.fill(Qt::transparent);
        pm.setDevicePixelRatio(dpr);
        return pm;
    }

    QImage img(deviceSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    renderer.render(&p, QRect(QPoint(0,0), deviceSize));

    if (tint) {
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(img.rect(), color);
    }
    p.end();

    QPixmap pm = QPixmap::fromImage(img);
    pm.setDevicePixelRatio(dpr);   // ★★★★★ 核心
    return pm;
}


QIcon IconManager::icon(Icon type, QSize size, QColor color)
{
    if (!color.isValid()) {
        color = g_defaultIconColor;
    }

    static QHash<QString, QIcon> cache;

    QString key = QString("%1_%2_%3_%4")
                      .arg(int(type))
                      .arg(size.width())
                      .arg(size.height())
                      .arg(color.name());

    if (cache.contains(key))
        return cache.value(key);

    QIcon icon(renderSvg(iconPath(type), color, size));
    cache.insert(key, icon);
    return icon;
}


void IconManager::clearCache()
{
    static QHash<QString, QIcon> empty;
    empty.clear();
}

void IconManager::setDefaultIconColor(const QColor &color)
{
    if (color.isValid()) {
        g_defaultIconColor = color;
        // 清除缓存，确保下次获取图标时使用新颜色
        clearCache();
    }
}

QColor IconManager::defaultIconColor()
{
    return g_defaultIconColor;
}

