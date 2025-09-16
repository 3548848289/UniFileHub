#include "include/ClipboardView.h"
#include "ui/ui_ClipboardView.h"
#include "include/ClipboardItemFactory.h"
#include "include/ImagePreviewDialog.h"
#include "include/FileTypeDetector.h"
#include <QGuiApplication>
#include <QMimeData>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QVariant>
#include <Qt>
#include<QToolTip>
#include <algorithm>
#include<QMouseEvent>


ClipboardView::ClipboardView(QWidget *parent)
    : QWidget(parent), ui(new Ui::ClipboardView),
    m_dbService(dbService::instance("./SmartDesk.db")),
    m_initialItemCount(0), m_currentRightClickedItem(nullptr) {
    ui->setupUi(this);

    // ui->listWidget



    initializeListWidget();

    m_imagePreviewLabel = new QLabel(this);
    // 设置窗口标志：悬浮窗（无标题栏）、置顶、始终在最前
    m_imagePreviewLabel->setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    // 设置样式：白色背景、灰色边框、内边距（模拟预览窗口）
    m_imagePreviewLabel->setStyleSheet("background-color: white; border: 1px solid #cccccc; padding: 5px;");
    // 设置图片对齐方式（居中）
    m_imagePreviewLabel->setAlignment(Qt::AlignCenter);
    // 限制最大预览尺寸（避免窗口过大）
    m_imagePreviewLabel->setMaximumSize(400, 300);
    // 初始隐藏
    m_imagePreviewLabel->hide();

    m_clipboard = QGuiApplication::clipboard();
    connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardView::onClipboardChanged);

    loadHistory();
    m_initialItemCount = ui->listWidget->count(); // 记录初始项数（后续只保存新增项）
}

ClipboardView::~ClipboardView() {
    on_saveButton_clicked();
    delete m_imagePreviewLabel;
    delete ui;
}

void ClipboardView::initializeListWidget() {
    ui->listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidget->setWordWrap(true);
    ui->listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listWidget->scrollToBottom();
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->listWidget->setIconSize(QSize(80, 60)); // 图标尺寸（图片/文件缩略图）
    ui->listWidget->setStyleSheet(
        R"(
            QListWidget::item {
                border: 1px solid lightgray;
                padding: 5px;
                margin: 2px;
            }

            QListWidget::item:selected {
                background-color: #a8c5f7;
                color: black;
            }

            QListWidget::item:hover {
                background-color: #e0e0e0;
            }
        )"
    );
    ui->listWidget->setMouseTracking(true);
    connect(ui->listWidget, &QListWidget::itemEntered,
            this, &ClipboardView::onItemEntered);

}

// 加载历史记录：从数据库读取 → 工厂类创建项 → 添加到UI
void ClipboardView::loadHistory() {
    int hours = SettingManager::Instance().clip_board_hours();
    QList<QString> serializedItems = m_dbService.dbClip().loadRecentHistory(hours);

    for (const QString& serialized : serializedItems) {
        std::unique_ptr<ClipboardItem> item =
            ClipboardItemFactory::createFromSerializedString(serialized);
        if (item) {
            m_clipboardItems.push_back(std::move(item));
        }
    }

    for (const auto& item : m_clipboardItems) {
        ui->listWidget->addItem(item->createListWidgetItem());
    }
}

// 添加剪贴板项：去重 + 插入到UI最前面
void ClipboardView::addClipboardItem(std::unique_ptr<ClipboardItem> item) {
    if (!item) return;

    QString newItemSerialized = item->serialize();
    for (const auto& existing : m_clipboardItems) {
        if (existing->serialize() == newItemSerialized) {
            return;
        }
    }

    m_clipboardItems.push_back(std::move(item));

    ui->listWidget->insertItem(0, m_clipboardItems.back()->createListWidgetItem());
}

// 根据列表项找到对应的ClipboardItem（通过UserRole存储的指针地址）
ClipboardItem* ClipboardView::findItemForListWidgetItem(QListWidgetItem* listItem) {
    if (!listItem) return nullptr;

    // 从UserRole获取指针地址（存储时为quintptr，避免直接存储指针）
    quintptr itemAddr = listItem->data(Qt::UserRole).value<quintptr>();
    return reinterpret_cast<ClipboardItem*>(itemAddr);
}

// 剪贴板数据变化：通过工厂类创建项并添加
void ClipboardView::onClipboardChanged() {
    const QMimeData* mimeData = m_clipboard->mimeData();
    if (!mimeData) return;

    // 关键：通过工厂类创建对应项，主视图不直接依赖子类
    std::unique_ptr<ClipboardItem> newItem =
        ClipboardItemFactory::createFromMimeData(mimeData);
    if (newItem) {
        addClipboardItem(std::move(newItem));
    }
}

// 右键菜单：复制项到剪贴板
void ClipboardView::copyItem() {
    if (!m_currentRightClickedItem) return;

    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (item) {
        item->copyToClipboard(m_clipboard);
    }
}

// 右键菜单：预览图片（支持Image项、图片File项、图片路径Text项）
void ClipboardView::previewImage() {
    if (!m_currentRightClickedItem) return;

    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item) return;

    QPixmap previewPixmap;

    // 根据项类型获取图片
    switch (item->type()) {
    case ClipboardItemType::Image: {
        auto* imageItem = static_cast<CliImage*>(item);
        previewPixmap = imageItem->pixmap();
        break;
    }
    case ClipboardItemType::File: {
        auto* fileItem = static_cast<CliFile*>(item);
        if (fileItem->isImageFile()) {
            previewPixmap.load(fileItem->filePaths().first());
        }
        break;
    }
    case ClipboardItemType::Text: {
        auto* textItem = static_cast<CliText*>(item);
        QString path = textItem->text();
        if (FileTypeDetector::isImageFile(path)) {
            previewPixmap.load(FileTypeDetector::toLocalPath(path));
        }
        break;
    }
    }

    // 显示预览对话框
    if (!previewPixmap.isNull()) {
        ImagePreviewDialog dialog(previewPixmap, this);
        dialog.exec();
    } else {
        QMessageBox::warning(this, "预览失败", "无法加载图片数据");
    }
}

// 右键菜单：打开文件所在目录（仅File项支持）
void ClipboardView::openFileLocation() {
    if (!m_currentRightClickedItem) return;

    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item || item->type() != ClipboardItemType::File) return;

    auto* fileItem = static_cast<CliFile*>(item);
    if (fileItem->filePaths().isEmpty()) return;

    // 获取第一个文件的目录路径，通过桌面服务打开
    QFileInfo fileInfo(fileItem->filePaths().first());
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.path()));
}

// 右键菜单/按钮：删除选中项（UI + 集合同时删除）
void ClipboardView::deleteItem() {
    QList<QListWidgetItem*> selectedItems = ui->listWidget->selectedItems();
    for (QListWidgetItem* listItem : selectedItems) {
        // 1. 找到对应的ClipboardItem
        ClipboardItem* item = findItemForListWidgetItem(listItem);
        if (!item) continue;

        // 2. 从UI列表移除并释放列表项
        int row = ui->listWidget->row(listItem);
        delete ui->listWidget->takeItem(row);

        // 3. 从集合中移除（智能指针自动释放内存）
        auto it = std::find_if(m_clipboardItems.begin(), m_clipboardItems.end(),
                               [item](const std::unique_ptr<ClipboardItem>& ptr) {
                                   return ptr.get() == item;
                               });
        if (it != m_clipboardItems.end()) {
            m_clipboardItems.erase(it);
        }
    }
    m_currentRightClickedItem = nullptr; // 重置右键选中项
}

// 清除按钮：清空所有项（UI + 集合）
void ClipboardView::on_clearButton_clicked() {
    ui->listWidget->clear();
    m_clipboardItems.clear();
    m_initialItemCount = 0; // 重置初始项数（后续保存无历史项）
}

// 保存按钮：增量保存新增项到数据库
void ClipboardView::on_saveButton_clicked() {
    int successCount = 0;
    // 只保存新增项（从初始项数之后的项开始）
    for (size_t i = m_initialItemCount; i < m_clipboardItems.size(); ++i) {
        const auto& item = m_clipboardItems[i];
        if (m_dbService.dbClip().setHistory(item->serialize())) {
            successCount++;
        }
    }
    // 更新初始项数（避免重复保存）
    m_initialItemCount = m_clipboardItems.size();
}

// 列表项双击事件：复制到剪贴板
void ClipboardView::on_listWidget_itemDoubleClicked(QListWidgetItem *item) {
    if (!item) return;
    m_currentRightClickedItem = item;
    copyItem();
}

// 列表右键菜单：根据项类型动态生成菜单
void ClipboardView::on_listWidget_customContextMenuRequested(const QPoint &pos) {
    m_currentRightClickedItem = ui->listWidget->itemAt(pos);
    if (!m_currentRightClickedItem) return;

    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item) return;

    QMenu menu(this);
    QAction *copyAction = menu.addAction("复制到剪贴板");

    // 根据项目类型添加特定操作
    bool canPreview = false;
    if (item->type() == ClipboardItemType::Image) {
        canPreview = true;
    } else if (item->type() == ClipboardItemType::File) {
        auto* fileItem = static_cast<CliFile*>(item);
        canPreview = fileItem->isImageFile();
    } else if (item->type() == ClipboardItemType::Text) {
        auto* textItem = static_cast<CliText*>(item);
        canPreview = FileTypeDetector::isImageFile(textItem->text());
    }

    if (canPreview) {
        QAction *previewAction = menu.addAction("预览图片");
        connect(previewAction, &QAction::triggered, this, &ClipboardView::previewImage);
    }

    if (item->type() == ClipboardItemType::File) {
        QAction *openLocationAction = menu.addAction("打开文件位置");
        connect(openLocationAction, &QAction::triggered, this, &ClipboardView::openFileLocation);
    }

    QAction *deleteAction = menu.addAction("删除");

    connect(copyAction, &QAction::triggered, this, &ClipboardView::copyItem);
    connect(deleteAction, &QAction::triggered, this, &ClipboardView::deleteItem);

    menu.exec(ui->listWidget->mapToGlobal(pos));
}

void ClipboardView::onItemEntered(QListWidgetItem *item) {
    if (!item) {
        QToolTip::hideText();  // 无项时隐藏提示
        return;
    }

    // 1. 根据列表项找到对应的 ClipboardItem
    ClipboardItem* clipboardItem = findItemForListWidgetItem(item);
    if (!clipboardItem) {
        QToolTip::hideText();
        return;
    }

    // 2. 隐藏之前可能残留的提示（避免冲突）
    QToolTip::hideText();

    // 3. 按类型构建提示文本（重点处理文件类型）
    QString tooltipText;
    switch (clipboardItem->type()) {
    // --------------------------
    // 重点：文件类型项 - 显示完整路径
    // --------------------------
    case ClipboardItemType::File: {
        CliFile* fileItem = static_cast<CliFile*>(clipboardItem);
        const QList<QString>& allFilePaths = fileItem->filePaths();

        if (allFilePaths.isEmpty()) {
            tooltipText = "空文件列表";
        } else if (allFilePaths.size() == 1) {
            // 单个文件：直接显示完整路径（标注“文件”/“文件夹”）
            QString fullPath = allFilePaths.first();
            QFileInfo fileInfo(fullPath);
            if (fileInfo.isDir()) {
                tooltipText = QString("文件夹路径:\n%1").arg(fullPath);
            } else {
                tooltipText = QString("文件路径:\n%1").arg(fullPath);
            }
        } else {
            // 多个文件：按列表显示每个文件的完整路径
            tooltipText = QString("多文件路径（共%1个）:\n").arg(allFilePaths.size());
            for (const QString& path : allFilePaths) {
                tooltipText += "- " + path + "\n";  // 每个路径前加“-”区分
            }
            // 移除最后一个多余的换行符
            tooltipText.chop(1);
        }
        break;
    }

        // --------------------------
        // 其他类型项：保持原有逻辑（如文本、图片）
        // --------------------------
    case ClipboardItemType::Text: {
        CliText* textItem = static_cast<CliText*>(clipboardItem);
        // 长文本仍截取显示，悬停显示完整文本（保持原需求）
        tooltipText = textItem->text();
        break;
    }
    case ClipboardItemType::Image: {
        CliImage* imageItem = static_cast<CliImage*>(clipboardItem);
        // 图片项显示尺寸信息（如之前需求）
        tooltipText = QString("图片尺寸: %1 × %2 像素")
                          .arg(imageItem->pixmap().width())
                          .arg(imageItem->pixmap().height());
        break;
    }
    default:
        tooltipText = "未知类型";
    }

    // 4. 显示提示：在鼠标位置，关联列表项，10秒后自动消失（避免长期残留）
    QToolTip::showText(
        QCursor::pos(),                // 提示显示在鼠标位置
        tooltipText,                   // 完整文件路径/其他提示文本
        ui->listWidget,                // 关联列表控件（随列表移动）
        ui->listWidget->visualItemRect(item),  // 关联当前列表项
        300000 );
}

// 辅助函数：检查项是否为图片类型，并加载图片到预览 Label
bool ClipboardView::loadImageToPreviewLabel(ClipboardItem* clipboardItem) {
    if (!clipboardItem) return false;

    QPixmap previewPixmap;
    // 根据项类型加载图片
    switch (clipboardItem->type()) {
    case ClipboardItemType::Image: {
        auto* imageItem = static_cast<CliImage*>(clipboardItem);
        previewPixmap = imageItem->pixmap();
        break;
    }
    case ClipboardItemType::File: {
        auto* fileItem = static_cast<CliFile*>(clipboardItem);
        if (fileItem->filePaths().size() == 1 && fileItem->isImageFile()) {
            previewPixmap.load(fileItem->filePaths().first());
        }
        break;
    }
    default:
        return false; // 非图片类型，返回失败
    }

    // 若图片有效，缩放后显示到 Label
    if (!previewPixmap.isNull()) {
        // 保持比例缩放（减去内边距 10px）
        QPixmap scaledPix = previewPixmap.scaled(
            m_imagePreviewLabel->maximumWidth() - 10,
            m_imagePreviewLabel->maximumHeight() - 10,
            Qt::KeepAspectRatio,    // 保持宽高比
            Qt::SmoothTransformation // 平滑缩放（避免模糊）
            );
        m_imagePreviewLabel->setPixmap(scaledPix);
        // 调整 Label 大小为图片实际大小（加内边距）
        m_imagePreviewLabel->resize(scaledPix.width() + 10, scaledPix.height() + 10);
        return true;
    }
    return false;
}

void ClipboardView::leaveEvent(QEvent *event) {
    m_imagePreviewLabel->hide();
    QToolTip::hideText();
    QWidget::leaveEvent(event); // 调用父类事件（避免阻断）
}

// 鼠标在列表内移动时，若当前位置无项（空白区域），隐藏预览
void ClipboardView::mouseMoveEvent(QMouseEvent *event) {
    // 检查鼠标是否在某个列表项上
    QListWidgetItem* currentItem = ui->listWidget->itemAt(ui->listWidget->mapFromParent(event->pos()));
    if (!currentItem) {
        m_imagePreviewLabel->hide();
        QToolTip::hideText();
    }
    QWidget::mouseMoveEvent(event); // 调用父类事件
}
