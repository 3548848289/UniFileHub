#include "include/ClipboardView.h"
#include "ui/ui_ClipboardView.h"
#include "include/ClipboardController.h"
#include "include/ClipboardItemFactory.h"
#include "include/ClipboardItem/ClipboardItem.h"
#include "include/ImagePreviewDialog.h"
#include "include/FileTypeDetector.h"
#include "include/ClipboardMenuBuilder.h"
#include "include/ClipboardItem/CliImage.h"
#include "include/ClipboardItem/CliFile.h"
#include "include/ClipboardItem/CliText.h"

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
#include <QShortcut>
#include "../Setting/include/SettingManager.h"

Q_DECLARE_METATYPE(quintptr) // 确保能用 QVariant 存取 quintptr

ClipboardView::ClipboardView(ClipboardController* controller, QWidget *parent)
    : QWidget(parent), ui(new Ui::ClipboardView), m_controller(controller), m_currentRightClickedItem(nullptr) {
    ui->setupUi(this);

    initializeListWidget();
    
    // 设置Controller的View引用
    m_controller->setView(this);

    // 连接Controller的信号到View的槽
    connect(m_controller, &ClipboardController::itemAddedToModel, this, &ClipboardView::onItemAdded);
    connect(m_controller, &ClipboardController::itemRemovedFromModel, this, &ClipboardView::onItemRemoved);
    connect(m_controller, &ClipboardController::modelCleared, this, &ClipboardView::onModelCleared);
    connect(m_controller, &ClipboardController::itemPinnedChanged, this, &ClipboardView::onItemPinnedChanged);

    QShortcut* ctrlCShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), ui->listWidget);
    ctrlCShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(ctrlCShortcut, &QShortcut::activated, this, [this](){        
        QListWidgetItem* item = ui->listWidget->currentItem();
        if (!item) return;

        m_currentRightClickedItem = item;
        copyItem();
        if (auto w = this->window()) {
            if (SettingManager::Instance().all_setting_fenable_tray()) {
                w->hide();
            } else {
                w->showNormal();
                w->setWindowState(Qt::WindowMinimized);
            }
        }
    });

    connect(qApp, &QApplication::aboutToQuit, this, &ClipboardView::on_saveButton_clicked);

    // 加载历史记录
    int hours = SettingManager::Instance().clip_board_hours();
    m_controller->loadHistory(hours);
}

void ClipboardView::insertNewItem(ClipboardItem* newItem) {
    QListWidgetItem* listItem = newItem->createListWidgetItem();
    quintptr addr = reinterpret_cast<quintptr>(newItem);
    listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));

    if (newItem->isPinned()) {
        // 置顶项插入到所有置顶项的最前面
        // 找到第一个非置顶项的位置，在它前面插入
        int insertRow = 0;
        for (; insertRow < ui->listWidget->count(); ++insertRow) {
            QListWidgetItem* it = ui->listWidget->item(insertRow);
            ClipboardItem* ci = findItemForListWidgetItem(it);
            if (!ci || !ci->isPinned()) {
                break;
            }
        }
        ui->listWidget->insertItem(0, listItem); // 新的置顶项总是放在最前面
        QIcon pinIcon(":/usedimage/pin.svg");
        QPixmap pixmap = pinIcon.pixmap(QSize(16, 16)); // 指定小图尺寸
        listItem->setIcon(QIcon(pixmap));
    } else {
        // 普通项插入到所有置顶项的后面，普通项的最前面
        int insertRow = 0;
        // 找到最后一个置顶项的位置
        for (; insertRow < ui->listWidget->count(); ++insertRow) {
            QListWidgetItem* it = ui->listWidget->item(insertRow);
            ClipboardItem* ci = findItemForListWidgetItem(it);
            if (!ci || !ci->isPinned()) {
                break;
            }
        }
        ui->listWidget->insertItem(insertRow, listItem); // 新的普通项放在普通项区域的最前面
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

// 根据 list item 的 UserRole 去取对应 ClipboardItem*
ClipboardItem* ClipboardView::findItemForListWidgetItem(QListWidgetItem* listItem) {
    if (!listItem) return nullptr;
    quintptr itemAddr = listItem->data(Qt::UserRole).value<quintptr>();
    return reinterpret_cast<ClipboardItem*>(itemAddr);
}

// 根据 ClipboardItem 查找对应的 QListWidgetItem
QListWidgetItem* ClipboardView::findListWidgetItemForClipboardItem(ClipboardItem* item) {
    if (!item) return nullptr;
    
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem* listItem = ui->listWidget->item(i);
        if (findItemForListWidgetItem(listItem) == item) {
            return listItem;
        }
    }
    
    return nullptr;
}

void ClipboardView::copyItem() {
    if (!m_currentRightClickedItem) return;
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (item) {
        m_controller->copyItemToClipboard(item);
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
    if (!item) return;
    
    m_controller->openFileLocation(item);
}

void ClipboardView::pinItem() {
    if (!m_currentRightClickedItem) return;
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item) return;
    
    m_controller->pinItem(item);
}

void ClipboardView::deleteItem() {
    QList<QListWidgetItem*> selectedItems = ui->listWidget->selectedItems();
    for (QListWidgetItem* listItem : selectedItems) {
        ClipboardItem* item = findItemForListWidgetItem(listItem);
        if (!item) continue;
        
        m_controller->deleteItem(item);
        // UI更新将通过onItemRemoved槽处理
    }
    m_currentRightClickedItem = nullptr;
}

void ClipboardView::on_clearButton_clicked() {
    m_controller->clearAllItems();
    // UI更新将通过onModelCleared槽处理
}

void ClipboardView::on_saveButton_clicked() {
    m_controller->saveHistory();
}

void ClipboardView::on_listWidget_itemDoubleClicked(QListWidgetItem *item) {
    if (!item) return;
    m_currentRightClickedItem = item;
    copyItem();

    if (auto w = this->window()) {
        if (SettingManager::Instance().all_setting_fenable_tray()) {
            w->hide();
        } else {
            w->showNormal();
            w->setWindowState(Qt::WindowMinimized);
        }
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
                                    [this]{ pinItem(); }
                                    );

    if (menu) {
        menu->exec(ui->listWidget->mapToGlobal(pos));
        delete menu; // 用完释放
    }
}

void ClipboardView::on_TextBtn_clicked() {
    // 过滤显示文本类型项
    filterItemsByType(ClipboardItemType::Text);
}

void ClipboardView::on_ImageBtn_clicked() {
    // 过滤显示图片类型项
    filterItemsByType(ClipboardItemType::Image);
}

void ClipboardView::on_FileBtn_clicked() {
    // 过滤显示文件类型项
    filterItemsByType(ClipboardItemType::File);
}

void ClipboardView::on_restoreBtn_clicked()
{
    // 显示所有类型的剪贴板项，不进行筛选
    filterItemsByType(ClipboardItemType::Unknown);
}


void ClipboardView::filterItemsByType(ClipboardItemType type) {
    // 重新加载所有项目并过滤
    if (!m_controller || !m_controller->getHistoryManager()) return;
    
    ui->listWidget->clear();
    
    auto addToListWidget = [this](ClipboardItem* item) {
        QListWidgetItem* listItem = item->createListWidgetItem();
        quintptr addr = reinterpret_cast<quintptr>(item);
        listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));
        if (item->isPinned()) {
            QIcon pinIcon(":/usedimage/pin.svg");
            QPixmap pixmap = pinIcon.pixmap(QSize(16, 16));
            listItem->setIcon(QIcon(pixmap));
        }
        ui->listWidget->addItem(listItem);
    };
    
    // 收集置顶项和普通项
    std::vector<ClipboardItem*> pinnedItems;
    std::vector<ClipboardItem*> normalItems;
    
    // 从最新到最旧收集项目，确保新项排在前面
    auto& items = m_controller->getHistoryManager()->items();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        ClipboardItem* item = it->get();
        if ((item->type() == type) || (type == ClipboardItemType::Unknown)) {
            if (item->isPinned())
                pinnedItems.push_back(item);
            else
                normalItems.push_back(item);
        }
    }
    
    // 按优先级排序显示：置顶项（新到旧）、普通项（新到旧）
    for (auto* item : pinnedItems) addToListWidget(item);
    for (auto* item : normalItems) addToListWidget(item);
}

// 接收Controller通知的槽函数
void ClipboardView::onItemAdded(ClipboardItem* item) {
    insertNewItem(item);
}

void ClipboardView::onItemRemoved(ClipboardItem* item) {
    QListWidgetItem* listItem = findListWidgetItemForClipboardItem(item);
    if (listItem) {
        int row = ui->listWidget->row(listItem);
        delete ui->listWidget->takeItem(row);
    }
}

void ClipboardView::onModelCleared() {
    ui->listWidget->clear();
}

void ClipboardView::onItemPinnedChanged(ClipboardItem* item) {
    // 刷新整个UI以重新排序置顶项
    refreshAllItems();
}

void ClipboardView::refreshAllItems() {
    ui->listWidget->clear();
    
    if (!m_controller || !m_controller->getHistoryManager()) return;
    
    // 收集置顶项和普通项
    std::vector<ClipboardItem*> pinnedItems;
    std::vector<ClipboardItem*> normalItems;

    // 从最新到最旧收集项目，确保新项排在前面
    auto& items = m_controller->getHistoryManager()->items();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        if (it->get()->isPinned())
            pinnedItems.push_back(it->get());
        else
            normalItems.push_back(it->get());
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



void ClipboardView::on_lineEdit_textChanged(const QString &text)
{
    // 实时搜索功能，直接调用editingFinished中的搜索逻辑
    on_lineEdit_editingFinished();
}

void ClipboardView::on_lineEdit_returnPressed()
{
    // 回车触发搜索，直接调用editingFinished中的搜索逻辑
    on_lineEdit_editingFinished();
}

void ClipboardView::on_lineEdit_editingFinished()
{
    if (!m_controller || !m_controller->getHistoryManager()) return;
    
    // 获取搜索文本
    QString searchText = ui->lineEdit->text().trimmed();
    
    // 清空当前列表
    ui->listWidget->clear();
    
    auto addToListWidget = [this](ClipboardItem* item) {
        QListWidgetItem* listItem = item->createListWidgetItem();
        quintptr addr = reinterpret_cast<quintptr>(item);
        listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));
        if (item->isPinned()) {
            QIcon pinIcon(":/usedimage/pin.svg");
            QPixmap pixmap = pinIcon.pixmap(QSize(16, 16));
            listItem->setIcon(QIcon(pixmap));
        }
        ui->listWidget->addItem(listItem);
    };
    
    // 收集符合条件的置顶项和普通项
    std::vector<ClipboardItem*> pinnedItems;
    std::vector<ClipboardItem*> normalItems;
    
    // 从最新到最旧收集项目，确保新项排在前面
    auto& items = m_controller->getHistoryManager()->items();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        ClipboardItem* item = it->get();
        
        // 只处理文本类型的项
        if (item->type() == ClipboardItemType::Text) {
            // 动态转换为CliText类型以访问text()方法
            CliText* textItem = dynamic_cast<CliText*>(item);
            // 如果搜索文本为空，显示所有项；否则执行模糊匹配
            if (searchText.isEmpty() || (textItem && textItem->text().contains(searchText, Qt::CaseInsensitive))) {
                if (item->isPinned())
                    pinnedItems.push_back(item);
                else
                    normalItems.push_back(item);
            }
        }
    }
    
    // 按优先级排序显示：置顶项（新到旧）、普通项（新到旧）
    for (auto* item : pinnedItems) addToListWidget(item);
    for (auto* item : normalItems) addToListWidget(item);
}

