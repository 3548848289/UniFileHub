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
#include "include/ClipboardItemDelegate.h"

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
#include "../Setting/include/ThemeManager.h"
#include "../Setting/include/IconManager.h"

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
        // 使用IconManager加载pin图标
        QIcon pinIcon = IconManager::icon(IconManager::Icon::Pin, QSize(16, 16));
        listItem->setIcon(pinIcon);
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
    
    // 获取主题颜色
    ThemeManager& themeManager = ThemeManager::Instance();
    QColor secondaryColor = themeManager.secondaryColor();
    QColor borderColor = themeManager.borderColor();
    
    // 为选中和悬停状态创建一个半透明的辅助颜色
    QColor selectedColor = secondaryColor;
    selectedColor.setAlpha(128); // 50% opacity
    
    // 设置自定义的 delegate 来显示序号，并传递主题颜色
    ClipboardItemDelegate* delegate = new ClipboardItemDelegate(this);
    delegate->setThemeColors(borderColor, selectedColor);
    ui->listWidget->setItemDelegate(delegate);

    QString style = QString(R"(
        QListWidget::item {
            border-top: 1px solid %1;
            padding: 5px;
            margin: 2px;
        }

        QListWidget::item:selected {
            background-color: %2;
            color: black;
        }

        QListWidget::item:hover {
            background-color: %2;
        }
    )").arg(borderColor.name(QColor::HexRgb), selectedColor.name(QColor::HexRgb));

    ui->listWidget->setStyleSheet(style);

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

void ClipboardView::on_typeComboBox_currentIndexChanged(int index) {
    // 根据选择的索引过滤不同类型的剪贴板项
    ClipboardItemType type;
    switch (index) {
    case 0: // 全部
        type = ClipboardItemType::Unknown;
        break;
    case 1: // 文本
        type = ClipboardItemType::Text;
        break;
    case 2: // 图片
        type = ClipboardItemType::Image;
        break;
    case 3: // 文件
        type = ClipboardItemType::File;
        break;
    default:
        type = ClipboardItemType::Unknown;
        break;
    }
    filterItemsByType(type);
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
            // 使用IconManager加载pin图标
            QIcon pinIcon = IconManager::icon(IconManager::Icon::Pin, QSize(16, 16));
            listItem->setIcon(pinIcon);
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
    // 检查当前的筛选条件，只有符合条件的项才添加到UI列表中
    ClipboardItemType currentType = ClipboardItemType::Unknown;
    int index = ui->typeComboBox->currentIndex();
    switch (index) {
    case 0: // 全部
        currentType = ClipboardItemType::Unknown;
        break;
    case 1: // 文本
        currentType = ClipboardItemType::Text;
        break;
    case 2: // 图片
        currentType = ClipboardItemType::Image;
        break;
    case 3: // 文件
        currentType = ClipboardItemType::File;
        break;
    default:
        currentType = ClipboardItemType::Unknown;
        break;
    }
    
    // 检查类型筛选条件
    bool matchType = (currentType == ClipboardItemType::Unknown) || (item->type() == currentType);
    
    // 检查搜索文本筛选条件
    QString searchText = ui->lineEdit->text().trimmed();
    bool matchSearch = searchText.isEmpty();
    
    if (!matchSearch) {
        // 根据不同类型的剪贴板项目进行搜索
        if (item->type() == ClipboardItemType::Text) {
            CliText* textItem = dynamic_cast<CliText*>(item);
            if (textItem && textItem->text().contains(searchText, Qt::CaseInsensitive)) {
                matchSearch = true;
            }
        } else if (item->type() == ClipboardItemType::File) {
            QString serialized = item->serialize();
            if (serialized.startsWith("FILE_DATA:")) {
                QStringList filePaths = serialized.mid(10).split(";").filter(QRegularExpression(".+"));
                for (const QString& path : filePaths) {
                    if (QFileInfo(path).fileName().contains(searchText, Qt::CaseInsensitive)) {
                        matchSearch = true;
                        break;
                    }
                }
            }
        }
    }
    
    // 只有同时符合类型和搜索条件的项才添加到UI中
    if (matchType && matchSearch) {
        insertNewItem(item);
        // 更新序号
        updateSequenceNumbers();
    }
}

void ClipboardView::onItemRemoved(ClipboardItem* item) {
    QListWidgetItem* listItem = findListWidgetItemForClipboardItem(item);
    if (listItem) {
        int row = ui->listWidget->row(listItem);
        delete ui->listWidget->takeItem(row);
        // 更新序号
        updateSequenceNumbers();
    }
}

void ClipboardView::onModelCleared() {
    ui->listWidget->clear();
}

void ClipboardView::onItemPinnedChanged(ClipboardItem* item) {
    // 刷新整个UI以重新排序置顶项
    refreshAllItems();
    // 更新序号
    updateSequenceNumbers();
}

void ClipboardView::updateSequenceNumbers() {
    // 不再需要更新序号到列表项文本中，因为序号由自定义的 delegate 显示
    // 这里可以保留方法的空实现，以避免其他代码调用时出错
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
            // 使用IconManager加载pin图标
            QIcon pinIcon = IconManager::icon(IconManager::Icon::Pin, QSize(24, 24));
            listItem->setIcon(pinIcon);
        }

        ui->listWidget->addItem(listItem);
    };

    for (auto* item : pinnedItems) addToListWidget(item);
    for (auto* item : normalItems) addToListWidget(item);

    // 更新序号
    updateSequenceNumbers();
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
    if (!m_controller) return;

    // 获取搜索文本
    QString searchText = ui->lineEdit->text().trimmed();

    // 调用Controller执行搜索，符合MVC架构
    // 注意：这里需要在Controller中添加searchItems方法
    m_controller->searchItems(searchText);
}

