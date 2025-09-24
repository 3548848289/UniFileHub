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
                std::unique_ptr<ClipboardItem> item(rawItem);
                if (item && m_historyManager.addItem(std::move(item))) {
                    insertNewItem(m_historyManager.items().back().get());
                }
            });


    int hours = SettingManager::Instance().clip_board_hours();
    m_historyManager.loadHistory(hours);
    refreshUI();
}

void ClipboardView::insertNewItem(ClipboardItem* newItem) {
    QListWidgetItem* listItem = newItem->createListWidgetItem();
    quintptr addr = reinterpret_cast<quintptr>(newItem);
    listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));

    if (newItem->isPinned()) {
        // pinned 直接插到最前
        ui->listWidget->insertItem(0, listItem);
    } else {
        // 找到最后一个 pinned 项的位置，在它后面插入
        int insertRow = 0;
        for (int i = 0; i < ui->listWidget->count(); ++i) {
            QListWidgetItem* it = ui->listWidget->item(i);
            ClipboardItem* ci = findItemForListWidgetItem(it);
            if (ci && ci->isPinned()) {
                insertRow = i + 1;
            } else {
                break;
            }
        }
        ui->listWidget->insertItem(insertRow, listItem);
    }
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

void ClipboardView::refreshUI() {
    ui->listWidget->clear();

    // 先显示置顶项，再显示普通项
    std::vector<ClipboardItem*> pinnedItems;
    std::vector<ClipboardItem*> normalItems;

    for (const auto& uptr : m_historyManager.items()) {
        if (uptr->isPinned())
            pinnedItems.push_back(uptr.get());
        else
            normalItems.push_back(uptr.get());
    }

    auto addToListWidget = [this](ClipboardItem* item) {
        QListWidgetItem* listItem = item->createListWidgetItem();
        quintptr addr = reinterpret_cast<quintptr>(item);
        listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));
        if (item->isPinned()) {
            QIcon pinIcon(":/usedimage/pin.svg");
            QPixmap pixmap = pinIcon.pixmap(QSize(16, 16)); // 指定小图尺寸
            listItem->setIcon(QIcon(pixmap));
        }

        ui->listWidget->addItem(listItem);
    };

    for (auto* item : pinnedItems) addToListWidget(item);
    for (auto* item : normalItems) addToListWidget(item);
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

void ClipboardView::pinItem() {
    if (!m_currentRightClickedItem) return;
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item) return;

    if (!item->isPinned()) {
        // 置顶：先设置标志，再在 history 中把它移动到最前
        item->setPinned(true);

        m_historyManager.moveToPinnedFront(item);
        m_historyManager.updatePinnedStatus(item);

    } else {
        // 取消置顶：仅清除标志（保留在现有位置）
        item->setPinned(false);
        m_historyManager.updatePinnedStatus(item);
        // 可选：如果你想取消置顶后把它移到普通区的最前/最后，可在这里调用对应函数
    }

    refreshUI();  // 刷新 UI（会按 m_items 顺序：先 pinned 再普通）
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

    if (auto w = this->window()) {
        w->showMinimized();
    }
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
                                    [this]{ deleteItem(); },
                                    [this]{ pinItem(); }   // 新增
                                    );

    if (menu) {
        menu->exec(ui->listWidget->mapToGlobal(pos));
        delete menu; // 用完释放
    }
}
