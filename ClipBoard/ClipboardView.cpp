#include "ClipboardView.h"
#include "ui_ClipboardView.h"
#include "ClipboardItemFactory.h"
#include "ImagePreviewDialog.h"
#include "FileTypeDetector.h"
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

// 构造函数：初始化UI、剪贴板、数据库服务
ClipboardView::ClipboardView(QWidget *parent)
    : QWidget(parent), ui(new Ui::ClipboardView),
    m_dbService(dbService::instance("./SmartDesk.db")), // 数据库单例
    m_initialItemCount(0), m_currentRightClickedItem(nullptr) {
    ui->setupUi(this);
    initializeListWidget();

    // 绑定剪贴板数据变化信号（系统剪贴板更新时触发）
    m_clipboard = QGuiApplication::clipboard();
    connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardView::onClipboardChanged);

    // 加载历史记录（通过工厂类创建项）
    loadHistory();
    m_initialItemCount = ui->listWidget->count(); // 记录初始项数（后续只保存新增项）
}

// 析构函数：保存未保存的项 + 释放UI
ClipboardView::~ClipboardView() {
    on_saveButton_clicked(); // 退出时自动保存新增项
    delete ui;
}

// 初始化列表控件：设置滚动、选择模式、右键菜单等
void ClipboardView::initializeListWidget() {
    ui->listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection); // 支持多选
    ui->listWidget->setWordWrap(true); // 自动换行
    ui->listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // 始终显示垂直滚动条
    ui->listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // 像素级滚动
    ui->listWidget->scrollToBottom(); // 默认滚动到底部
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu); // 启用自定义右键菜单
    ui->listWidget->setIconSize(QSize(80, 60)); // 图标尺寸（图片/文件缩略图）
}

// 加载历史记录：从数据库读取 → 工厂类创建项 → 添加到UI
void ClipboardView::loadHistory() {
    // 从配置获取历史保留时长（小时）
    int hours = SettingManager::Instance().clip_board_hours();
    QList<QString> serializedItems = m_dbService.dbClip().loadRecentHistory(hours);

    // 遍历序列化字符串，通过工厂类创建对应ClipboardItem
    for (const QString& serialized : serializedItems) {
        std::unique_ptr<ClipboardItem> item =
            ClipboardItemFactory::createFromSerializedString(serialized);
        if (item) {
            m_clipboardItems.push_back(std::move(item));
        }
    }

    // 将创建的项添加到UI列表
    for (const auto& item : m_clipboardItems) {
        ui->listWidget->addItem(item->createListWidgetItem());
    }
}

// 添加剪贴板项：去重 + 插入到UI最前面
void ClipboardView::addClipboardItem(std::unique_ptr<ClipboardItem> item) {
    if (!item) return;

    // 去重：比较序列化结果（避免重复添加相同项）
    QString newItemSerialized = item->serialize();
    for (const auto& existing : m_clipboardItems) {
        if (existing->serialize() == newItemSerialized) {
            return; // 重复项，不添加
        }
    }

    // 加入集合（智能指针自动管理内存）
    m_clipboardItems.push_back(std::move(item));

    // 插入到UI列表最前面（最新项显示在顶部）
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
        auto* imageItem = static_cast<ImageClipboardItem*>(item);
        previewPixmap = imageItem->pixmap();
        break;
    }
    case ClipboardItemType::File: {
        auto* fileItem = static_cast<FileClipboardItem*>(item);
        if (fileItem->isImageFile()) {
            previewPixmap.load(fileItem->filePaths().first());
        }
        break;
    }
    case ClipboardItemType::Text: {
        auto* textItem = static_cast<TextClipboardItem*>(item);
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

    auto* fileItem = static_cast<FileClipboardItem*>(item);
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

    // 创建右键菜单
    QMenu menu(this);
    QAction *copyAction = menu.addAction("复制到剪贴板");

    // 根据项目类型添加特定操作
    bool canPreview = false;
    if (item->type() == ClipboardItemType::Image) {
        canPreview = true;
    } else if (item->type() == ClipboardItemType::File) {
        auto* fileItem = static_cast<FileClipboardItem*>(item);
        canPreview = fileItem->isImageFile();
    } else if (item->type() == ClipboardItemType::Text) {
        auto* textItem = static_cast<TextClipboardItem*>(item);
        canPreview = FileTypeDetector::isImageFile(textItem->text());
    }

    // 图片预览动作
    if (canPreview) {
        QAction *previewAction = menu.addAction("预览图片");
        connect(previewAction, &QAction::triggered, this, &ClipboardView::previewImage);
    }

    // 文件位置动作（仅文件项）
    if (item->type() == ClipboardItemType::File) {
        QAction *openLocationAction = menu.addAction("打开文件位置");
        connect(openLocationAction, &QAction::triggered, this, &ClipboardView::openFileLocation);
    }

    // 删除动作
    QAction *deleteAction = menu.addAction("删除");

    // 绑定动作与槽函数
    connect(copyAction, &QAction::triggered, this, &ClipboardView::copyItem);
    connect(deleteAction, &QAction::triggered, this, &ClipboardView::deleteItem);

    // 显示菜单
    menu.exec(ui->listWidget->mapToGlobal(pos));
}

// 确保Qt元对象系统能识别此类（用于信号槽）
#include "ClipboardView.moc"
