#include "ClipboardView.h"
#include "ui_ClipboardView.h"
#include <QBuffer>
#include <QByteArray>
#include <QImageReader>
#include <QMimeData>
#include <QClipboard>
#include <QMessageBox>
#include <QFileInfo>
#include <QUrl>

// 图片预览对话框实现
ImagePreviewDialog::ImagePreviewDialog(const QPixmap& pixmap, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("图片预览");
    setMinimumSize(600, 400);

    QLabel *label = new QLabel(this);
    label->setPixmap(pixmap);
    label->setScaledContents(false);
    label->setAlignment(Qt::AlignCenter);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(label);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(scrollArea);
    setLayout(layout);
}

// ClipboardView类实现
ClipboardView::ClipboardView(QWidget *parent) : QWidget(parent),
    ui(new Ui::ClipboardView), dbservice(dbService::instance("./SmartDesk.db")){
    ui->setupUi(this);

    // 配置列表控件
    ui->listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidget->setWordWrap(true);
    ui->listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listWidget->scrollToBottom();
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->listWidget->setIconSize(QSize(80, 60));

    // 初始化剪贴板
    clipboard = QGuiApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged, this, &ClipboardView::onClipboardChanged);

    // 加载历史记录
    int hours = SettingManager::Instance().clip_board_hours();
    QList<QString> items = dbservice.dbClip().loadRecentHistory(hours);
    for (const QString &content : items) {
        if (content.startsWith("IMAGE_DATA:")) {
            QByteArray imageData = QByteArray::fromBase64(content.mid(10).toUtf8());
            QPixmap pixmap;
            if (pixmap.loadFromData(imageData)) {
                addImageItem(pixmap);
            }
        } else if (isImageFile(content)) {
            // 如果是图片路径，加载图片
            QPixmap pixmap(content);
            if (!pixmap.isNull()) {
                addImageItem(pixmap);
            } else {
                // 图片不存在，添加路径作为文本
                addTextItem(content);
            }
        } else {
            addTextItem(content);
        }
    }
    initialItemCount = ui->listWidget->count();
}

ClipboardView::~ClipboardView()
{
    on_saveButton_clicked();
    delete ui;
}

// 检查路径是否为有效的图片文件
bool ClipboardView::isImageFile(const QString& path)
{
    if (path.isEmpty()) return false;

    // 支持的图片格式
    QStringList imageFormats = {"jpg", "jpeg", "png", "bmp", "gif", "tiff", "webp"};

    // 处理URL格式（如file:///开头）
    QString localPath = path;
    if (path.startsWith("file://")) {
        QUrl url(path);
        if (url.isLocalFile()) {
            localPath = url.toLocalFile();
        }
    }

    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) return false;

    // 检查文件扩展名
    QString ext = fileInfo.suffix().toLower();
    return imageFormats.contains(ext);
}

// 从路径加载图片
QPixmap ClipboardView::loadImageFromPath(const QString& path)
{
    if (!isImageFile(path)) return QPixmap();

    QString localPath = path;
    if (path.startsWith("file://")) {
        QUrl url(path);
        if (url.isLocalFile()) {
            localPath = url.toLocalFile();
        }
    }

    QPixmap pixmap;
    if (pixmap.load(localPath)) {
        return pixmap;
    }
    return QPixmap();
}

// 处理剪贴板变化
void ClipboardView::onClipboardChanged()
{
    const QMimeData *mimeData = clipboard->mimeData();

    // 优先处理图片数据
    if(mimeData->hasImage()) {
        QVariant imageData = mimeData->imageData();
        if (imageData.canConvert<QPixmap>()) {
            QPixmap pixmap = imageData.value<QPixmap>();
            if (!pixmap.isNull()) {
                addImageItem(pixmap);
                return; // 处理完图片就返回，避免重复处理
            }
        }
    }

    // 处理文本（可能是图片路径）
    if(mimeData->hasText()) {
        QString text = mimeData->text();
        if (!text.isEmpty() && !text.trimmed().isEmpty()) {
            // 检查是否是图片文件路径
            if (isImageFile(text)) {
                QPixmap pixmap = loadImageFromPath(text);
                if (!pixmap.isNull()) {
                    addImageItem(pixmap);
                    return;
                }
            }
            // 如果不是图片路径，作为文本处理
            addTextItem(text);
        }
    }

    // 处理文件URL（从文件管理器复制的图片）
    if(mimeData->hasUrls() && mimeData->urls().count() == 1) {
        QUrl url = mimeData->urls().first();
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            if (isImageFile(filePath)) {
                QPixmap pixmap(filePath);
                if (!pixmap.isNull()) {
                    addImageItem(pixmap);
                }
            }
        }
    }
}

// 添加文本项到列表
void ClipboardView::addTextItem(const QString& text)
{
    // 检查重复
    for(int i = 0; i < ui->listWidget->count(); i++) {
        if(ui->listWidget->item(i)->data(Qt::UserRole + 1).toString() == "text" &&
            ui->listWidget->item(i)->data(Qt::UserRole).toString() == text) {
            return;
        }
    }

    QListWidgetItem *item = new QListWidgetItem(text);
    item->setData(Qt::UserRole, text);
    item->setData(Qt::UserRole + 1, "text");
    item->setToolTip(text);
    item->setTextAlignment(Qt::AlignTop);
    ui->listWidget->insertItem(0, item);
}

// 添加图片项到列表
void ClipboardView::addImageItem(const QPixmap& pixmap)
{
    // 转换图片数据为字节数组
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");

    // 创建列表项
    QListWidgetItem *item = new QListWidgetItem();
    item->setIcon(QIcon(pixmap.scaled(80, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    item->setText(QString("[图片] %1x%2").arg(pixmap.width()).arg(pixmap.height()));
    item->setData(Qt::UserRole, byteArray); // 存储图片二进制数据
    item->setData(Qt::UserRole + 1, "image");
    item->setToolTip("双击复制图片 | 右键预览");
    item->setTextAlignment(Qt::AlignVCenter);

    ui->listWidget->insertItem(0, item);
}

// 复制选中项到剪贴板
void ClipboardView::copyItem()
{
    if (!currentRightClickedItem) return;

    QString type = currentRightClickedItem->data(Qt::UserRole + 1).toString();

    if (type == "text") {
        // 检查文本是否是图片路径，如果是则复制图片而不是路径
        QString text = currentRightClickedItem->data(Qt::UserRole).toString();
        if (isImageFile(text)) {
            QPixmap pixmap = loadImageFromPath(text);
            if (!pixmap.isNull()) {
                QMimeData *mimeData = new QMimeData();
                mimeData->setImageData(pixmap);
                clipboard->setMimeData(mimeData);
                return;
            }
        }
        // 不是图片路径，正常复制文本
        clipboard->setText(text);
    }
    else if (type == "image") {
        // 从存储的二进制数据加载图片
        QByteArray byteArray = currentRightClickedItem->data(Qt::UserRole).toByteArray();
        QPixmap pixmap;
        if (pixmap.loadFromData(byteArray, "PNG")) {
            QMimeData *mimeData = new QMimeData();
            mimeData->setImageData(pixmap);
            clipboard->setMimeData(mimeData);
        } else {
            QMessageBox::warning(this, "错误", "图片数据损坏，无法复制");
        }
    }
}

// 其他函数保持不变...
void ClipboardView::previewImage()
{
    if (!currentRightClickedItem) return;

    QString type = currentRightClickedItem->data(Qt::UserRole + 1).toString();
    if (type == "image") {
        QByteArray byteArray = currentRightClickedItem->data(Qt::UserRole).toByteArray();
        QPixmap pixmap;
        if (pixmap.loadFromData(byteArray, "PNG")) {
            ImagePreviewDialog* dialog = new ImagePreviewDialog(pixmap, this);
            dialog->exec();
            delete dialog;
        } else {
            QMessageBox::warning(this, "错误", "无法加载图片");
        }
    }
    // 支持预览文本中的图片路径
    else if (type == "text") {
        QString text = currentRightClickedItem->data(Qt::UserRole).toString();
        if (isImageFile(text)) {
            QPixmap pixmap = loadImageFromPath(text);
            if (!pixmap.isNull()) {
                ImagePreviewDialog* dialog = new ImagePreviewDialog(pixmap, this);
                dialog->exec();
                delete dialog;
            } else {
                QMessageBox::warning(this, "错误", "无法加载图片文件");
            }
        }
    }
}

void ClipboardView::on_clearButton_clicked()
{
    ui->listWidget->clear();
}

void ClipboardView::on_saveButton_clicked()
{
    int successCount = 0;
    for (int i = initialItemCount; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        QString type = item->data(Qt::UserRole + 1).toString();

        if (type == "text") {
            QString content = item->data(Qt::UserRole).toString();
            if (dbservice.dbClip().setHistory(content))
                successCount++;
        }
        else if (type == "image") {
            QByteArray byteArray = item->data(Qt::UserRole).toByteArray();
            QString base64Data = "IMAGE_DATA:" + byteArray.toBase64();
            if (dbservice.dbClip().setHistory(base64Data))
                successCount++;
        }
    }
}

void ClipboardView::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;

    currentRightClickedItem = item;
    copyItem();
}

void ClipboardView::on_listWidget_customContextMenuRequested(const QPoint &pos)
{
    currentRightClickedItem = ui->listWidget->itemAt(pos);
    if (!currentRightClickedItem) return;

    QMenu menu(this);
    QAction *copyAction = menu.addAction("复制到剪贴板");

    QString type = currentRightClickedItem->data(Qt::UserRole + 1).toString();
    if (type == "image" || (type == "text" && isImageFile(currentRightClickedItem->data(Qt::UserRole).toString()))) {
        QAction *previewAction = menu.addAction("预览图片");
        connect(previewAction, &QAction::triggered, this, &ClipboardView::previewImage);
    }

    QAction *deleteAction = menu.addAction("删除");

    connect(copyAction, &QAction::triggered, this, &ClipboardView::copyItem);
    connect(deleteAction, &QAction::triggered, this, &ClipboardView::deleteItem);

    menu.exec(ui->listWidget->mapToGlobal(pos));
}

void ClipboardView::deleteItem()
{
    QList<QListWidgetItem *> selectedItems = ui->listWidget->selectedItems();
    for (QListWidgetItem *item : selectedItems) {
        delete ui->listWidget->takeItem(ui->listWidget->row(item));
    }
    currentRightClickedItem = nullptr;
}

#include "ClipboardView.moc"
