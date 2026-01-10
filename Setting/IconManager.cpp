#include "include/IconManager.h"
#include <QApplication>
#include <QStyleHints>
#include <QSvgRenderer>
#include <QPainter>
#include <QHash>

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
    case IconManager::Icon::FileSystem: return "://Main/FileSystem.svg";
    case IconManager::Icon::Clipboard: return "://Main/Clipboard.svg";
    case IconManager::Icon::OnlineDoc: return "://Main/OnlineDoc.svg";
    case IconManager::Icon::FileBackup: return "://Main/FileBackup.svg";
    case IconManager::Icon::More: return "://Main/More.svg";
    case IconManager::Icon::Tag: return "://Main/Tag.svg";
    case IconManager::Icon::Drawing: return "://Main/Drawing.svg";
    case IconManager::Icon::Login: return "://Main/Login.svg";
    case IconManager::Icon::Drive: return "://Main/Drive.svg";
    case IconManager::Icon::Email: return "://Main/Email.svg";
    }
    return {};
}

static QPixmap renderSvg(const QString &file, const QColor &color, QSize size)
{
    QSvgRenderer renderer(file);

    if (!renderer.isValid()) {
        // 如果SVG文件无效，返回一个空的透明图像
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);
        return pixmap;
    }

    // 创建一个与目标尺寸相同的QImage
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    // 将SVG渲染到整个图像区域
    renderer.render(&p);

    // 关键：统一染色
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(img.rect(), color);
    p.end();

    return QPixmap::fromImage(img);
}

QIcon IconManager::icon(Icon type, QSize size)
{
    static QHash<QString, QIcon> cache;

    QColor color = currentIconColor();
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

