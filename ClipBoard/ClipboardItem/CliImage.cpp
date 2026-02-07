#include "include/ClipboardItem/CliImage.h"
#include <QListWidgetItem>
#include <QVariant>
#include <Qt>
#include <QBuffer>
#include <QMimeData>
#include <QClipboard>

// 传入QPixmap：自动转换为PNG二进制数据
CliImage::CliImage(const QPixmap& pixmap)
    : ClipboardItem(ClipboardItemType::Image), m_pixmap(pixmap) {
    QBuffer buffer(&m_data);
    buffer.open(QIODevice::WriteOnly);
    m_pixmap.save(&buffer, "PNG"); // 固定为PNG格式存储
}

// 传入二进制数据：反序列化为QPixmap
CliImage::CliImage(const QByteArray& data)
    : ClipboardItem(ClipboardItemType::Image), m_data(data) {
    m_pixmap.loadFromData(data, "PNG");
}

// 创建列表项：显示图片缩略图和尺寸
QListWidgetItem* CliImage::createListWidgetItem() const {
    QListWidgetItem* item = new QListWidgetItem();
    // 缩放到80x60（保持宽高比）
    item->setIcon(QIcon(m_pixmap.scaled(80, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    // 直接设置显示文本，不添加序号
    item->setText(QString("[图片] %1x%2").arg(m_pixmap.width()).arg(m_pixmap.height()));
    item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(this)));
    item->setData(Qt::UserRole + 1, "image");
    item->setToolTip(QString("<img src='%1' width='%2'/>")
                         .arg(savePixmapToTempFile(m_pixmap))
                         .arg(400));

    item->setTextAlignment(Qt::AlignVCenter);
    return item;
}

// 复制图片到剪贴板（通过QMimeData传递）
void CliImage::copyToClipboard(QClipboard* clipboard) const {
    QMimeData* mimeData = new QMimeData();
    mimeData->setImageData(m_pixmap);
    clipboard->setMimeData(mimeData);
}

// 序列化：前缀标识 + Base64编码（避免二进制数据问题）
QString CliImage::serialize() const {
    return "IMAGE_DATA:" + m_data.toBase64();
}

QString CliImage::savePixmapToTempFile(const QPixmap& pixmap) const {
    QPixmap scaled = pixmap.scaled(1000, 1000, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QTemporaryFile tempFile(QDir::tempPath() + "/img_XXXXXX.png");
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        scaled.save(&tempFile, "PNG");
        m_tempImageFiles.append(tempFile.fileName());
        return tempFile.fileName();
    }
    return QString();
}

CliImage::~CliImage() {
    // 析构时清理所有临时文件
    for (const QString& f : m_tempImageFiles) {
        QFile::remove(f);
    }
    m_tempImageFiles.clear();
}
