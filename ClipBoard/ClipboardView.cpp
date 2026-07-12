#include "include/ClipboardView.h"
#include "ui/ui_ClipboardView.h"
#include "include/ClipboardController.h"
#include "include/ClipboardItem/CliFile.h"
#include "include/ClipboardItem/CliImage.h"
#include "include/ClipboardItem/CliText.h"
#include "include/ClipboardItemDelegate.h"
#include "include/ClipboardMenuBuilder.h"
#include "include/FileTypeDetector.h"
#include "include/ImagePreviewDialog.h"
#include "../Setting/include/IconManager.h"
#include "../Setting/include/SettingManager.h"
#include "../Setting/include/ThemeManager.h"
#include <QApplication>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>
#include <QShortcut>
#include <QVariant>

Q_DECLARE_METATYPE(quintptr)

ClipboardView::ClipboardView(ClipboardController* controller, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::ClipboardView),
      m_controller(controller),
      m_currentRightClickedItem(nullptr),
      m_imagePreviewLabel(nullptr)
{
    ui->setupUi(this);

    initializeListWidget();
    m_controller->setView(this);

    connect(m_controller, &ClipboardController::itemAddedToModel, this, &ClipboardView::onItemAdded);
    connect(m_controller, &ClipboardController::itemRemovedFromModel, this, &ClipboardView::onItemRemoved);
    connect(m_controller, &ClipboardController::modelCleared, this, &ClipboardView::onModelCleared);
    connect(m_controller, &ClipboardController::itemPinnedChanged, this, &ClipboardView::onItemPinnedChanged);
    connect(m_controller, &ClipboardController::infoMessageRequested, this, [this](const QString &message) {
        QMessageBox::information(this, QStringLiteral("提示"), message);
    });
    connect(m_controller, &ClipboardController::errorMessageRequested, this, [this](const QString &message) {
        QMessageBox::warning(this, QStringLiteral("提示"), message);
    });

    QShortcut* ctrlCShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_C), ui->listWidget);
    ctrlCShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(ctrlCShortcut, &QShortcut::activated, this, [this]() {
        QListWidgetItem* item = ui->listWidget->currentItem();
        if (!item) {
            return;
        }

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

    const int hours = SettingManager::Instance().clip_board_hours();
    m_controller->loadHistory(hours);
}

ClipboardView::~ClipboardView()
{
    on_saveButton_clicked();
    delete ui;
}

void ClipboardView::refreshCloudItems()
{
    if (m_controller) {
        m_controller->refreshCloudItems();
    }
}

void ClipboardView::initializeListWidget()
{
    ui->listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidget->setWordWrap(true);
    ui->listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listWidget->scrollToBottom();
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->listWidget->setIconSize(QSize(80, 60));

    ThemeManager& themeManager = ThemeManager::Instance();
    QColor secondaryColor = themeManager.secondaryColor();
    QColor borderColor = themeManager.borderColor();
    QColor selectedColor = secondaryColor;
    selectedColor.setAlpha(128);

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

void ClipboardView::insertNewItem(ClipboardItem* newItem)
{
    QListWidgetItem* listItem = newItem->createListWidgetItem();
    const quintptr addr = reinterpret_cast<quintptr>(newItem);
    listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));

    if (newItem->isCloudItem()) {
        ui->listWidget->insertItem(0, listItem);
    } else if (newItem->isPinned()) {
        int insertRow = 0;
        for (; insertRow < ui->listWidget->count(); ++insertRow) {
            QListWidgetItem* it = ui->listWidget->item(insertRow);
            ClipboardItem* ci = findItemForListWidgetItem(it);
            if (!ci || (!ci->isCloudItem() && !ci->isPinned())) {
                break;
            }
        }
        ui->listWidget->insertItem(insertRow, listItem);
        QIcon pinIcon = IconManager::icon(IconManager::Icon::Pin, QSize(16, 16));
        listItem->setIcon(pinIcon);
    } else {
        int insertRow = 0;
        for (; insertRow < ui->listWidget->count(); ++insertRow) {
            QListWidgetItem* it = ui->listWidget->item(insertRow);
            ClipboardItem* ci = findItemForListWidgetItem(it);
            if (!ci || (!ci->isCloudItem() && !ci->isPinned())) {
                break;
            }
        }
        ui->listWidget->insertItem(insertRow, listItem);
    }
}

ClipboardItem* ClipboardView::findItemForListWidgetItem(QListWidgetItem* listItem)
{
    if (!listItem) {
        return nullptr;
    }
    const quintptr itemAddr = listItem->data(Qt::UserRole).value<quintptr>();
    return reinterpret_cast<ClipboardItem*>(itemAddr);
}

QListWidgetItem* ClipboardView::findListWidgetItemForClipboardItem(ClipboardItem* item)
{
    if (!item) {
        return nullptr;
    }

    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem* listItem = ui->listWidget->item(i);
        if (findItemForListWidgetItem(listItem) == item) {
            return listItem;
        }
    }
    return nullptr;
}

void ClipboardView::copyItem()
{
    if (!m_currentRightClickedItem) {
        return;
    }
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (item) {
        m_controller->copyItemToClipboard(item);
    }
}

void ClipboardView::previewImage()
{
    if (!m_currentRightClickedItem) {
        return;
    }
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item) {
        return;
    }

    QPixmap previewPixmap;
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
        const QString path = textItem->text();
        if (FileTypeDetector::isImageFile(path)) {
            previewPixmap.load(FileTypeDetector::toLocalPath(path));
        }
        break;
    }
    default:
        break;
    }

    if (!previewPixmap.isNull()) {
        ImagePreviewDialog dialog(previewPixmap, this);
        dialog.exec();
    } else {
        QMessageBox::warning(this, QStringLiteral("预览失败"), QStringLiteral("无法加载图片数据"));
    }
}

void ClipboardView::openFileLocation()
{
    if (!m_currentRightClickedItem) {
        return;
    }
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (item) {
        m_controller->openFileLocation(item);
    }
}

void ClipboardView::pinItem()
{
    if (!m_currentRightClickedItem) {
        return;
    }
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (item) {
        m_controller->pinItem(item);
    }
}

void ClipboardView::syncItemToCloud()
{
    if (!m_currentRightClickedItem) {
        return;
    }
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (item) {
        m_controller->syncItemToCloud(item);
    }
}

void ClipboardView::unsyncItemFromCloud()
{
    if (!m_currentRightClickedItem) {
        return;
    }
    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (item) {
        m_controller->unsyncItemFromCloud(item);
    }
}

void ClipboardView::deleteItem()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget->selectedItems();
    for (QListWidgetItem* listItem : selectedItems) {
        ClipboardItem* item = findItemForListWidgetItem(listItem);
        if (!item) {
            continue;
        }
        m_controller->deleteItem(item);
    }
    m_currentRightClickedItem = nullptr;
}

void ClipboardView::on_clearButton_clicked()
{
    m_controller->clearAllItems();
}

void ClipboardView::on_saveButton_clicked()
{
    m_controller->saveHistory();
}

void ClipboardView::on_refreshButton_clicked()
{
    refreshCloudItems();
}

void ClipboardView::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
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

void ClipboardView::on_listWidget_customContextMenuRequested(const QPoint &pos)
{
    m_currentRightClickedItem = ui->listWidget->itemAt(pos);
    if (!m_currentRightClickedItem) {
        return;
    }

    ClipboardItem* item = findItemForListWidgetItem(m_currentRightClickedItem);
    if (!item) {
        return;
    }

    ClipboardMenuBuilder builder;
    QMenu* menu = builder.buildMenu(item,
                                    [this]{ copyItem(); },
                                    [this]{ previewImage(); },
                                    [this]{ openFileLocation(); },
                                    [this]{ deleteItem(); },
                                    [this]{ pinItem(); },
                                    [this]{ syncItemToCloud(); },
                                    [this]{ unsyncItemFromCloud(); });

    if (menu) {
        menu->exec(ui->listWidget->mapToGlobal(pos));
        delete menu;
    }
}

void ClipboardView::on_typeComboBox_currentIndexChanged(int index)
{
    ClipboardItemType type;
    switch (index) {
    case 0:
        type = ClipboardItemType::Unknown;
        break;
    case 1:
        type = ClipboardItemType::Text;
        break;
    case 2:
        type = ClipboardItemType::Image;
        break;
    case 3:
        type = ClipboardItemType::File;
        break;
    default:
        type = ClipboardItemType::Unknown;
        break;
    }
    filterItemsByType(type);
}

void ClipboardView::filterItemsByType(ClipboardItemType type)
{
    if (!m_controller || !m_controller->getHistoryManager()) {
        return;
    }

    ui->listWidget->clear();

    auto addToListWidget = [this](ClipboardItem* item) {
        QListWidgetItem* listItem = item->createListWidgetItem();
        const quintptr addr = reinterpret_cast<quintptr>(item);
        listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));
        if (item->isCloudItem()) {
            QIcon cloudIcon = IconManager::icon(IconManager::Icon::Cloud, QSize(16, 16));
            listItem->setIcon(cloudIcon);
        } else if (item->isPinned()) {
            QIcon pinIcon = IconManager::icon(IconManager::Icon::Pin, QSize(16, 16));
            listItem->setIcon(pinIcon);
        }
        ui->listWidget->addItem(listItem);
    };

    std::vector<ClipboardItem*> cloudItems;
    std::vector<ClipboardItem*> pinnedItems;
    std::vector<ClipboardItem*> normalItems;

    const auto& items = m_controller->getHistoryManager()->items();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        ClipboardItem* item = it->get();
        if (type == ClipboardItemType::Unknown || item->type() == type) {
            if (item->isCloudItem()) {
                cloudItems.push_back(item);
            } else if (item->isPinned()) {
                pinnedItems.push_back(item);
            } else {
                normalItems.push_back(item);
            }
        }
    }

    for (auto* item : cloudItems) {
        addToListWidget(item);
    }
    for (auto* item : pinnedItems) {
        addToListWidget(item);
    }
    for (auto* item : normalItems) {
        addToListWidget(item);
    }
}

void ClipboardView::onItemAdded(ClipboardItem* item)
{
    ClipboardItemType currentType = ClipboardItemType::Unknown;
    switch (ui->typeComboBox->currentIndex()) {
    case 1:
        currentType = ClipboardItemType::Text;
        break;
    case 2:
        currentType = ClipboardItemType::Image;
        break;
    case 3:
        currentType = ClipboardItemType::File;
        break;
    default:
        currentType = ClipboardItemType::Unknown;
        break;
    }

    bool matchType = (currentType == ClipboardItemType::Unknown) || (item->type() == currentType);
    QString searchText = ui->lineEdit->text().trimmed();
    bool matchSearch = searchText.isEmpty();

    if (!matchSearch) {
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

    if (matchType && matchSearch) {
        insertNewItem(item);
        updateSequenceNumbers();
    }
}

void ClipboardView::onItemRemoved(ClipboardItem* item)
{
    QListWidgetItem* listItem = findListWidgetItemForClipboardItem(item);
    if (listItem) {
        int row = ui->listWidget->row(listItem);
        delete ui->listWidget->takeItem(row);
        updateSequenceNumbers();
    }
}

void ClipboardView::onModelCleared()
{
    ui->listWidget->clear();
}

void ClipboardView::onItemPinnedChanged(ClipboardItem* item)
{
    Q_UNUSED(item);
    refreshAllItems();
    updateSequenceNumbers();
}

void ClipboardView::updateSequenceNumbers()
{
}

void ClipboardView::refreshAllItems()
{
    ui->listWidget->clear();

    if (!m_controller || !m_controller->getHistoryManager()) {
        return;
    }

    std::vector<ClipboardItem*> cloudItems;
    std::vector<ClipboardItem*> pinnedItems;
    std::vector<ClipboardItem*> normalItems;
    const auto& items = m_controller->getHistoryManager()->items();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        if (it->get()->isCloudItem()) {
            cloudItems.push_back(it->get());
        } else if (it->get()->isPinned()) {
            pinnedItems.push_back(it->get());
        } else {
            normalItems.push_back(it->get());
        }
    }

    auto addToListWidget = [this](ClipboardItem* item) {
        QListWidgetItem* listItem = item->createListWidgetItem();
        const quintptr addr = reinterpret_cast<quintptr>(item);
        listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));
        if (item->isCloudItem()) {
            QIcon cloudIcon = IconManager::icon(IconManager::Icon::Cloud, QSize(24, 24));
            listItem->setIcon(cloudIcon);
        } else if (item->isPinned()) {
            QIcon pinIcon = IconManager::icon(IconManager::Icon::Pin, QSize(24, 24));
            listItem->setIcon(pinIcon);
        }
        ui->listWidget->addItem(listItem);
    };

    for (auto* item : cloudItems) {
        addToListWidget(item);
    }
    for (auto* item : pinnedItems) {
        addToListWidget(item);
    }
    for (auto* item : normalItems) {
        addToListWidget(item);
    }
}

void ClipboardView::on_lineEdit_textChanged(const QString &text)
{
    Q_UNUSED(text);
    on_lineEdit_editingFinished();
}

void ClipboardView::on_lineEdit_returnPressed()
{
    on_lineEdit_editingFinished();
}

void ClipboardView::on_lineEdit_editingFinished()
{
    if (!m_controller) {
        return;
    }

    QString searchText = ui->lineEdit->text().trimmed();
    m_controller->searchItems(searchText);
}
