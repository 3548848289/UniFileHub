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
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QVariant>
#include <QGuiApplication>

// 初始化支持的图片格式
QStringList FileClipboardItem::s_supportedImageFormats = {"jpg", "jpeg", "png", "bmp", "gif", "tiff", "webp"};

// 文本剪贴板项实现
QListWidgetItem* TextClipboardItem::createListWidgetItem() const {
    QListWidgetItem* item = new QListWidgetItem(m_text);
    item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(this)));
    item->setData(Qt::UserRole + 1, "text");
    item->setToolTip(m_text);
    item->setTextAlignment(Qt::AlignTop);
    return item;
}

void TextClipboardItem::copyToClipboard(QClipboard* clipboard) const {
    clipboard->setText(m_text);
}

// 图片剪贴板项实现
ImageClipboardItem::ImageClipboardItem(const QPixmap& pixmap)
    : ClipboardItem(Image), m_pixmap(pixmap) {
    QBuffer buffer(&m_data);
    buffer.open(QIODevice::WriteOnly);
    m_pixmap.save(&buffer, "PNG");
}

ImageClipboardItem::ImageClipboardItem(const QByteArray& data)
    : ClipboardItem(Image), m_data(data) {
    m_pixmap.loadFromData(data, "PNG");
}

QListWidgetItem* ImageClipboardItem::createListWidgetItem() const {
    QListWidgetItem* item = new QListWidgetItem();
    item->setIcon(QIcon(m_pixmap.scaled(80, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    item->setText(QString("[图片] %1x%2").arg(m_pixmap.width()).arg(m_pixmap.height()));
    item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(this)));
    item->setData(Qt::UserRole + 1, "image");
    item->setToolTip("双击复制图片 | 右键预览");
    item->setTextAlignment(Qt::AlignVCenter);
    return item;
}

void ImageClipboardItem::copyToClipboard(QClipboard* clipboard) const {
    QMimeData* mimeData = new QMimeData();
    mimeData->setImageData(m_pixmap);
    clipboard->setMimeData(mimeData);
}

QString ImageClipboardItem::serialize() const {
    return "IMAGE_DATA:" + m_data.toBase64();
}

// 文件剪贴板项实现
FileClipboardItem::FileClipboardItem(const QStringList& filePaths)
    : ClipboardItem(File), m_filePaths(filePaths) {}

QListWidgetItem* FileClipboardItem::createListWidgetItem() const {
    QListWidgetItem* item = new QListWidgetItem();

    // 设置显示文本
    QString displayText;
    if (m_filePaths.size() == 1) {
        QFileInfo fileInfo(m_filePaths.first());
        displayText = QString("[文件] %1").arg(fileInfo.fileName());
    } else {
        displayText = QString("[多个文件] 共 %1 个文件").arg(m_filePaths.size());
    }
    item->setText(displayText);

    // 如果是图片文件，显示缩略图
    if (m_filePaths.size() == 1 && isImageFile()) {
        QPixmap pixmap(m_filePaths.first());
        item->setIcon(QIcon(pixmap.scaled(80, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    } else {
        item->setIcon(QIcon::fromTheme("document", QIcon(":/icons/file.png")));
    }

    item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(this)));
    item->setData(Qt::UserRole + 1, "file");
    item->setToolTip(displayText);
    item->setTextAlignment(Qt::AlignVCenter);

    return item;
}

void FileClipboardItem::copyToClipboard(QClipboard* clipboard) const {
    QMimeData* mimeData = new QMimeData();
    QList<QUrl> urls;

    for (const QString& path : m_filePaths) {
        urls.append(QUrl::fromLocalFile(path));
    }

    mimeData->setUrls(urls);
    clipboard->setMimeData(mimeData);
}

QString FileClipboardItem::serialize() const {
    return "FILE_DATA:" + m_filePaths.join(";");
}

bool FileClipboardItem::isImageFile() const {
    if (m_filePaths.size() != 1) return false;
    return FileTypeDetector::isImageFile(m_filePaths.first());
}

// 图片预览对话框实现
ImagePreviewDialog::ImagePreviewDialog(const QPixmap& pixmap, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("图片预览");
    setMinimumSize(600, 400);

    m_imageLabel = new QLabel(this);
    m_imageLabel->setPixmap(pixmap);
    m_imageLabel->setScaledContents(false);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_scrollArea);
    setLayout(layout);
}

// 文件类型检测工具类实现
bool FileTypeDetector::isImageFile(const QString& path) {
    if (path.isEmpty()) return false;

    QString localPath = toLocalPath(path);
    QFileInfo fileInfo(localPath);

    if (!fileInfo.exists() || !fileInfo.isFile()) return false;

    QString ext = getFileExtension(localPath);
    return FileClipboardItem::s_supportedImageFormats.contains(ext);
}

QString FileTypeDetector::getFileExtension(const QString& path) {
    QFileInfo fileInfo(toLocalPath(path));
    return fileInfo.suffix().toLower();
}

QString FileTypeDetector::toLocalPath(const QString& path) {
    if (path.startsWith("file://")) {
        QUrl url(path);
        if (url.isLocalFile()) {
            return url.toLocalFile();
        }
    }
    return path;
}

bool FileTypeDetector::fileExists(const QString& path) {
    QFileInfo fileInfo(toLocalPath(path));
    return fileInfo.exists() && fileInfo.isFile();
}

// ClipboardView类实现
ClipboardView::ClipboardView(QWidget *parent)
    : QWidget(parent), ui(new Ui::ClipboardView),
    m_dbService(dbService::instance("./SmartDesk.db")),
    m_initialItemCount(0), m_currentRightClickedItem(nullptr) {
    ui->setupUi(this);
    initializeListWidget();

    // 初始化剪贴板
    m_clipboard = QGuiApplication::clipboard();
    connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardView::onClipboardChanged);

    // 加载历史记录
    loadHistory();
    m_initialItemCount = ui->listWidget->count();
}

ClipboardView::~ClipboardView() {
    on_saveButton_clicked();
    delete ui;
}

void ClipboardView::initializeListWidget() {
    ui->listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidget->setWordWrap(true);
    ui->listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listWidget->scrollToBottom();
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->listWidget->setIconSize(QSize(80, 60));
}

void ClipboardView::loadHistory() {
    int hours = SettingManager::Instance().clip_board_hours();
    QList<QString> items = m_dbService.dbClip().loadRecentHistory(hours);

    for (const QString &content : items) {
        if (content.startsWith("IMAGE_DATA:")) {
            QByteArray imageData = QByteArray::fromBase64(content.mid(10).toUtf8());
            m_clipboardItems.push_back(std::make_unique<ImageClipboardItem>(imageData));
        }
        else if (content.startsWith("FILE_DATA:")) {
            QStringList filePaths = content.mid(10).split(";");
            m_clipboardItems.push_back(std::make_unique<FileClipboardItem>(filePaths));
        }
        else {
            // 检查是否是图片文件路径
            if (FileTypeDetector::isImageFile(content)) {
                QPixmap pixmap(FileTypeDetector::toLocalPath(content));
                if (!pixmap.isNull()) {
                    m_clipboardItems.push_back(std::make_unique<ImageClipboardItem>(pixmap));
                } else {
                    m_clipboardItems.push_back(std::make_unique<TextClipboardItem>(content));
                }
            } else {
                m_clipboardItems.push_back(std::make_unique<TextClipboardItem>(content));
            }
        }
    }

    // 将加载的项添加到列表
    for (const auto& item : m_clipboardItems) {
        ui->listWidget->addItem(item->createListWidgetItem());
    }
}

void ClipboardView::addClipboardItem(std::unique_ptr<ClipboardItem> item) {
    // 检查重复
    for (const auto& existingItem : m_clipboardItems) {
        if (existingItem->serialize() == item->serialize()) {
            return;
        }
    }

    // 添加到集合
    m_clipboardItems.push_back(std::move(item));

    // 添加到列表控件，插入到最前面
    ui->listWidget->insertItem(0, m_clipboardItems.back()->createListWidgetItem());
}

ClipboardItem* ClipboardView::findItemForListWidgetItem(QListWidgetItem* listItem) {
    if (!listItem) return nullptr;
    quintptr addr = listItem->data(Qt::UserRole).value<quintptr>();
    return reinterpret_cast<ClipboardItem*>(addr);
}


void ClipboardView::onClipboardChanged() {
    const QMimeData *mimeData = m_clipboard->mimeData();

    // 处理文件URL（支持多个文件）
    if (mimeData->hasUrls() && !mimeData->urls().isEmpty()) {
        QStringList filePaths;
        for (const QUrl& url : mimeData->urls()) {
            if (url.isLocalFile()) {
                filePaths.append(url.toLocalFile());
            }
        }

        if (!filePaths.isEmpty()) {
            addClipboardItem(std::make_unique<FileClipboardItem>(filePaths));
            return;
        }
    }

    // 处理图片数据

    if (mimeData->hasImage()) {
        const QVariant imageVar = mimeData->imageData();
        QPixmap pixmap;

        if (imageVar.canConvert<QImage>()) {
            QImage img = imageVar.value<QImage>();
            if (!img.isNull()) {
                pixmap = QPixmap::fromImage(img);
            }
        } else if (imageVar.canConvert<QPixmap>()) {
            pixmap = imageVar.value<QPixmap>();
        }

        if (!pixmap.isNull()) {
            addClipboardItem(std::make_unique<ImageClipboardItem>(pixmap));
            return;
        }
    }


    // 处理文本
    if (mimeData->hasText()) {
        QString text = mimeData->text();
        if (!text.isEmpty() && !text.trimmed().isEmpty()) {
            addClipboardItem(std::make_unique<TextClipboardItem>(text));
        }
    }
}

void ClipboardView::copyItem() {
    if (!m_currentRightClickedItem) return;

    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (item) {
        item->copyToClipboard(m_clipboard);
    }
}

void ClipboardView::previewImage() {
    if (!m_currentRightClickedItem) return;

    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item) return;

    QPixmap pixmap;

    if (item->type() == ClipboardItem::Image) {
        auto* imageItem = static_cast<ImageClipboardItem*>(item);
        pixmap = imageItem->pixmap();
    }
    else if (item->type() == ClipboardItem::File) {
        auto* fileItem = static_cast<FileClipboardItem*>(item);
        if (fileItem->isImageFile()) {
            pixmap.load(fileItem->filePaths().first());
        }
    }
    else if (item->type() == ClipboardItem::Text) {
        auto* textItem = static_cast<TextClipboardItem*>(item);
        QString path = textItem->text();
        if (FileTypeDetector::isImageFile(path)) {
            pixmap.load(FileTypeDetector::toLocalPath(path));
        }
    }

    if (!pixmap.isNull()) {
        ImagePreviewDialog dialog(pixmap, this);
        dialog.exec();
    } else {
        QMessageBox::warning(this, "错误", "无法加载图片");
    }
}

void ClipboardView::openFileLocation() {
    if (!m_currentRightClickedItem) return;

    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item || item->type() != ClipboardItem::File) return;

    auto* fileItem = static_cast<FileClipboardItem*>(item);
    if (!fileItem->filePaths().isEmpty()) {
        QFileInfo fileInfo(fileItem->filePaths().first());
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.path()));
    }
}

void ClipboardView::deleteItem() {
    QList<QListWidgetItem *> selectedItems = ui->listWidget->selectedItems();
    for (QListWidgetItem *item : selectedItems) {
        // 先取出关联的 ClipboardItem*
        ClipboardItem* clipboardItem = findItemForListWidgetItem(item);

        // 再从列表移除
        int row = ui->listWidget->row(item);
        QListWidgetItem* taken = ui->listWidget->takeItem(row);
        delete taken; // 删除 UI item

        // 从容器中移除实际对象
        if (clipboardItem) {
            auto it = std::find_if(m_clipboardItems.begin(), m_clipboardItems.end(),
                                   [clipboardItem](const std::unique_ptr<ClipboardItem>& ptr) {
                                       return ptr.get() == clipboardItem;
                                   });
            if (it != m_clipboardItems.end()) {
                m_clipboardItems.erase(it);
            }
        }
    }
    m_currentRightClickedItem = nullptr;
}

void ClipboardView::on_clearButton_clicked() {
    ui->listWidget->clear();
    m_clipboardItems.clear();
}

void ClipboardView::on_saveButton_clicked() {
    int successCount = 0;
    // 只保存新添加的项目
    for (size_t i = m_initialItemCount; i < m_clipboardItems.size(); ++i) {
        const auto& item = m_clipboardItems[i];
        if (m_dbService.dbClip().setHistory(item->serialize())) {
            successCount++;
        }
    }
}

void ClipboardView::on_listWidget_itemDoubleClicked(QListWidgetItem *item) {
    if (!item) return;
    m_currentRightClickedItem = item;
    copyItem();
}

void ClipboardView::on_listWidget_customContextMenuRequested(const QPoint &pos) {
    m_currentRightClickedItem = ui->listWidget->itemAt(pos);
    if (!m_currentRightClickedItem) return;

    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item) return;

    QMenu menu(this);
    QAction *copyAction = menu.addAction("复制到剪贴板");

    // 根据项目类型添加特定操作
    if (item->type() == ClipboardItem::Image ||
        (item->type() == ClipboardItem::File && static_cast<FileClipboardItem*>(item)->isImageFile()) ||
        (item->type() == ClipboardItem::Text && FileTypeDetector::isImageFile(static_cast<TextClipboardItem*>(item)->text()))) {
        QAction *previewAction = menu.addAction("预览图片");
        connect(previewAction, &QAction::triggered, this, &ClipboardView::previewImage);
    }

    if (item->type() == ClipboardItem::File) {
        QAction *openLocationAction = menu.addAction("打开文件位置");
        connect(openLocationAction, &QAction::triggered, this, &ClipboardView::openFileLocation);
    }

    QAction *deleteAction = menu.addAction("删除");

    connect(copyAction, &QAction::triggered, this, &ClipboardView::copyItem);
    connect(deleteAction, &QAction::triggered, this, &ClipboardView::deleteItem);

    menu.exec(ui->listWidget->mapToGlobal(pos));
}

#include "ClipboardView.moc"
