#include "include/ClipboardItem/CliFile.h"
#include "include/FileTypeDetector.h"
#include <QListWidgetItem>
#include <QVariant>
#include <Qt>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>
#include <QClipboard>

// 初始化支持的图片格式（静态成员）
QStringList CliFile::s_supportedImageFormats = {
    "jpg", "jpeg", "png", "bmp", "gif", "tiff", "webp"
};

CliFile::CliFile(const QStringList& filePaths)
    : ClipboardItem(ClipboardItemType::File), m_filePaths(filePaths) {}

// 创建列表项：显示文件名/文件数量，图片文件显示缩略图
QListWidgetItem* CliFile::createListWidgetItem() const {
    QListWidgetItem* item = new QListWidgetItem();
    QString displayText;

    // 单文件：显示文件名；多文件：显示数量
    if (m_filePaths.size() == 1) {
        QFileInfo fileInfo(m_filePaths.first());
        if (fileInfo.isDir()) {
            displayText = QString("[文件夹] %1").arg(fileInfo.fileName());
        } else {
            displayText = QString("[文件] %1").arg(fileInfo.fileName());
        }
    } else {
        displayText = QString("[多个项目] 共 %1 个").arg(m_filePaths.size());
    }

    item->setText(displayText);

    // 统一使用默认文件图标，不生成图片缩略图
    item->setIcon(QIcon::fromTheme("document", QIcon("://usedimage/history.svg")));

    // 设置 ToolTip
    if (m_filePaths.size() > 1) {
        // 多文件/文件夹显示每个路径
        QString toolTip;
        for (const auto& path : m_filePaths) {
            toolTip += path + "\n";
        }
        item->setToolTip(toolTip.trimmed());
    } else {
        QFileInfo info(m_filePaths.first());
        if (info.isDir()) {
            // 单个文件夹显示完整路径
            item->setToolTip(m_filePaths.first());
        } else {
            item->setToolTip(displayText);
        }
    }

    item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(this)));
    item->setData(Qt::UserRole + 1, "file");
    item->setTextAlignment(Qt::AlignVCenter);
    return item;
}


// 复制文件到剪贴板（通过URL列表传递）
void CliFile::copyToClipboard(QClipboard* clipboard) const {
    QMimeData* mimeData = new QMimeData();
    QList<QUrl> urls;

    for (const QString& path : m_filePaths) {
        urls.append(QUrl::fromLocalFile(path));
    }

    mimeData->setUrls(urls);
    clipboard->setMimeData(mimeData);
}

// 序列化：前缀标识 + 分号分隔的文件路径
QString CliFile::serialize() const {
    return "FILE_DATA:" + m_filePaths.join(";");
}

// 判断是否为图片文件（依赖FileTypeDetector工具类）
bool CliFile::isImageFile() const {
    if (m_filePaths.size() != 1) return false;
    return FileTypeDetector::isImageFile(m_filePaths.first());
}

bool CliFile::isDirectory(const QString& path) const {
    QFileInfo info(path);
    return info.exists() && info.isDir();
}

