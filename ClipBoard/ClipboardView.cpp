#include "include/ClipboardView.h"
#include "ui/ui_ClipboardView.h"
#include "include/ClipboardItemFactory.h"
#include "include/ImagePreviewDialog.h"
#include "include/FileTypeDetector.h"
#include "include/ClipboardMenuBuilder.h"

#include <QGuiApplication>
#include <QMimeData>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QVariant>
#include <Qt>
#include <QToolTip>
#include <algorithm>
#include <QMouseEvent>

Q_DECLARE_METATYPE(quintptr) // 确保能用 QVariant 存取 quintptr

ClipboardView::ClipboardView(QWidget *parent)
    : QWidget(parent),ui(new Ui::ClipboardView),m_currentRightClickedItem(nullptr) {
    ui->setupUi(this);

    initializeListWidget();

    m_clipboardMonitor = new ClipboardMonitor(this);
    connect(m_clipboardMonitor, &ClipboardMonitor::newItemCaptured,
            this, [this](ClipboardItem* rawItem) {
                std::unique_ptr<ClipboardItem> item(rawItem); // 接回 unique_ptr
                if (item && m_historyManager.addItem(std::move(item))) {
                    ui->listWidget->insertItem(0, m_historyManager.items().back()->createListWidgetItem());
                }
    });


    int hours = SettingManager::Instance().clip_board_hours();
    m_historyManager.loadHistory(hours);
    refreshUI();
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
}

// 将 historyManager.items() 刷到 listWidget，并在 UserRole 中存储对应 ClipboardItem*（quintptr）
void ClipboardView::refreshUI() {
    ui->listWidget->clear();
    for (const auto& uptr : m_historyManager.items()) {
        // createListWidgetItem() 应由 ClipboardItem 实现并返回一个新的 QListWidgetItem*
        QListWidgetItem* listItem = uptr->createListWidgetItem();
        // 把真实对象地址存入 UserRole，后面用 findItemForListWidgetItem 取回
        quintptr addr = reinterpret_cast<quintptr>(uptr.get());
        listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));
        ui->listWidget->addItem(listItem);
    }
}

// 根据 list item 的 UserRole 去取对应 ClipboardItem*
ClipboardItem* ClipboardView::findItemForListWidgetItem(QListWidgetItem* listItem) {
    if (!listItem) return nullptr;
    quintptr itemAddr = listItem->data(Qt::UserRole).value<quintptr>();
    return reinterpret_cast<ClipboardItem*>(itemAddr);
}


void ClipboardView::copyItem() {
    if (!m_currentRightClickedItem) return;
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (item) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        item->copyToClipboard(clipboard);
    }
}


void ClipboardView::previewImage() {
    if (!m_currentRightClickedItem) return;
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item) return;

    QPixmap previewPixmap;
    switch (item->type()) {
    case ClipboardItemType::Image: {
        auto* imageItem = static_cast<CliImage*>(item);
        previewPixmap = imageItem->pixmap();
        break;
    }
    case ClipboardItemType::File: {
        auto* fileItem = static_cast<CliFile*>(item);
        if (fileItem->isImageFile()) previewPixmap.load(fileItem->filePaths().first());
        break;
    }
    case ClipboardItemType::Text: {
        auto* textItem = static_cast<CliText*>(item);
        QString path = textItem->text();
        if (FileTypeDetector::isImageFile(path)) previewPixmap.load(FileTypeDetector::toLocalPath(path));
        break;
    }
    }

    if (!previewPixmap.isNull()) {
        ImagePreviewDialog dialog(previewPixmap, this);
        dialog.exec();
    } else {
        QMessageBox::warning(this, "预览失败", "无法加载图片数据");
    }
}

void ClipboardView::openFileLocation() {
    if (!m_currentRightClickedItem) return;
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item || item->type() != ClipboardItemType::File) return;

    auto* fileItem = static_cast<CliFile*>(item);
    if (fileItem->filePaths().isEmpty()) return;

    QFileInfo fileInfo(fileItem->filePaths().first());
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.path()));
}

void ClipboardView::deleteItem() {
    QList<QListWidgetItem*> selectedItems = ui->listWidget->selectedItems();
    for (QListWidgetItem* listItem : selectedItems) {
        ClipboardItem* item = findItemForListWidgetItem(listItem);
        if (!item) continue;

        int row = ui->listWidget->row(listItem);
        delete ui->listWidget->takeItem(row);

        m_historyManager.removeItem(item);
    }
    m_currentRightClickedItem = nullptr;
}

void ClipboardView::on_clearButton_clicked() {
    ui->listWidget->clear();
    m_historyManager.clear();
}

void ClipboardView::on_saveButton_clicked() {
    m_historyManager.saveIncremental();
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

    ClipboardMenuBuilder builder;

    QMenu* menu = builder.buildMenu(item,
        [this]{ copyItem(); },
        [this]{ previewImage(); },
        [this]{ openFileLocation(); },
        [this]{ deleteItem(); }
    );

    if (menu) {
        menu->exec(ui->listWidget->mapToGlobal(pos));
        delete menu; // 用完释放
    }
}
