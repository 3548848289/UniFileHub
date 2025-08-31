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
#include <algorithm>

ClipboardView::ClipboardView(QWidget *parent)
    : QWidget(parent), ui(new Ui::ClipboardView),
    m_dbService(dbService::instance("./SmartDesk.db")),
    m_initialItemCount(0), m_currentRightClickedItem(nullptr) {
    ui->setupUi(this);

    // ui->listWidget



    initializeListWidget();

    m_clipboard = QGuiApplication::clipboard();
    connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardView::onClipboardChanged);

    loadHistory();
    m_initialItemCount = ui->listWidget->count(); // 记录初始项数（后续只保存新增项）
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
