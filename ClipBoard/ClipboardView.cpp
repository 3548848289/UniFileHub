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
#include <QAbstractAnimation>
#include <QApplication>
#include <QEasingCurve>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QMenu>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QScrollBar>
#include <QShortcut>
#include <QVariant>
#include <QWheelEvent>

Q_DECLARE_METATYPE(quintptr)

ClipboardView::ClipboardView(ClipboardController* controller, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::ClipboardView),
      m_controller(controller),
      m_currentRightClickedItem(nullptr),
      m_imagePreviewLabel(nullptr),
      m_smoothScrollAnimation(nullptr),
      m_smoothScrollTarget(0)
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
        copyItemAndMaybeCollapse(SettingManager::Instance().clip_board_ctrl_c_copy_minimize());
    });

    connect(qApp, &QApplication::aboutToQuit, this, &ClipboardView::on_saveButton_clicked);

    const int hours = SettingManager::Instance().clip_board_hours();
    m_controller->loadHistory(hours);

    // 初始化"复制即同步"勾选框状态
    ui->autoSyncCheckBox->setChecked(SettingManager::Instance().clip_board_copy_auto_sync());
    connect(ui->autoSyncCheckBox, &QCheckBox::toggled, this, [](bool checked) {
        SettingManager::Instance().setValue(QStringLiteral("clip_board/copy_auto_sync"), checked);
    });

    connect(&SettingManager::Instance(), &SettingManager::settingChanged,
            this, [this](const QString &key, const QVariant &value) {
                if (key == QStringLiteral("clip_board/hours") && m_controller) {
                    m_controller->loadHistory(value.toInt());
                }
            });
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
    QFont listFont = QApplication::font();
    listFont.setPointSize(SettingManager::Instance().all_setting_font_size());
    ui->listWidget->setFont(listFont);
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

    initializeSmoothScrolling();
}

void ClipboardView::initializeSmoothScrolling()
{
    QScrollBar* scrollBar = ui->listWidget->verticalScrollBar();
    scrollBar->setSingleStep(24);

    m_smoothScrollTarget = scrollBar->value();
    m_smoothScrollAnimation = new QPropertyAnimation(scrollBar, "value", this);
    m_smoothScrollAnimation->setDuration(160);
    m_smoothScrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
    ui->listWidget->viewport()->installEventFilter(this);
}

bool ClipboardView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->listWidget->viewport() && event->type() == QEvent::Wheel) {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);
        if (wheelEvent->modifiers().testFlag(Qt::ControlModifier)) {
            return QWidget::eventFilter(watched, event);
        }

        int delta = 0;
        const QPoint pixelDelta = wheelEvent->pixelDelta();
        if (!pixelDelta.isNull()) {
            delta = -pixelDelta.y();
        } else {
            const int angleDelta = wheelEvent->angleDelta().y();
            if (angleDelta != 0) {
                const int pixelsPerStep = QApplication::wheelScrollLines()
                                          * ui->listWidget->verticalScrollBar()->singleStep();
                delta = -(angleDelta * pixelsPerStep) / 120;
            }
        }

        if (delta != 0) {
            smoothScrollBy(delta);
            event->accept();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void ClipboardView::smoothScrollBy(int delta)
{
    QScrollBar* scrollBar = ui->listWidget->verticalScrollBar();
    if (!scrollBar) {
        return;
    }

    if (!m_smoothScrollAnimation) {
        scrollBar->setValue(scrollBar->value() + delta);
        return;
    }

    if (m_smoothScrollAnimation->state() != QAbstractAnimation::Running) {
        m_smoothScrollTarget = scrollBar->value();
    }

    m_smoothScrollTarget = qBound(scrollBar->minimum(),
                                  m_smoothScrollTarget + delta,
                                  scrollBar->maximum());

    m_smoothScrollAnimation->stop();
    m_smoothScrollAnimation->setStartValue(scrollBar->value());
    m_smoothScrollAnimation->setEndValue(m_smoothScrollTarget);
    m_smoothScrollAnimation->start();
}

void ClipboardView::insertNewItem(ClipboardItem* newItem)
{
    if (!shouldDisplayItem(newItem)) {
        return;
    }

    QListWidgetItem* listItem = newItem->createListWidgetItem();
    const quintptr addr = reinterpret_cast<quintptr>(newItem);
    listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));
    applyItemFont(listItem);

    if (newItem->isPinned()) {
        int insertRow = 0;
        for (; insertRow < ui->listWidget->count(); ++insertRow) {
            QListWidgetItem* it = ui->listWidget->item(insertRow);
            ClipboardItem* ci = findItemForListWidgetItem(it);
            if (!ci || !ci->isPinned()) {
                break;
            }
        }
        ui->listWidget->insertItem(insertRow, listItem);
        QIcon pinIcon = IconManager::icon(IconManager::Icon::Pin, QSize(16, 16));
        listItem->setIcon(pinIcon);
    } else if (newItem->isCloudItem()) {
        int insertRow = 0;
        for (; insertRow < ui->listWidget->count(); ++insertRow) {
            QListWidgetItem* it = ui->listWidget->item(insertRow);
            ClipboardItem* ci = findItemForListWidgetItem(it);
            if (!ci || (!ci->isPinned() && !ci->isCloudItem())) {
                break;
            }
        }
        ui->listWidget->insertItem(insertRow, listItem);
        QIcon cloudIcon = IconManager::icon(IconManager::Icon::Cloud, QSize(16, 16));
        listItem->setIcon(cloudIcon);
    } else {
        int insertRow = 0;
        for (; insertRow < ui->listWidget->count(); ++insertRow) {
            QListWidgetItem* it = ui->listWidget->item(insertRow);
            ClipboardItem* ci = findItemForListWidgetItem(it);
            if (!ci || (!ci->isPinned() && !ci->isCloudItem())) {
                break;
            }
        }
        ui->listWidget->insertItem(insertRow, listItem);
    }
}

void ClipboardView::addItemToListWidget(ClipboardItem* item)
{
    if (!item) {
        return;
    }

    QListWidgetItem* listItem = item->createListWidgetItem();
    const quintptr addr = reinterpret_cast<quintptr>(item);
    listItem->setData(Qt::UserRole, QVariant::fromValue<quintptr>(addr));
    applyItemFont(listItem);
    if (item->isPinned()) {
        QIcon pinIcon = IconManager::icon(IconManager::Icon::Pin, QSize(24, 24));
        listItem->setIcon(pinIcon);
    } else if (item->isCloudItem()) {
        QIcon cloudIcon = IconManager::icon(IconManager::Icon::Cloud, QSize(24, 24));
        listItem->setIcon(cloudIcon);
    }
    ui->listWidget->addItem(listItem);
}

void ClipboardView::applyItemFont(QListWidgetItem* listItem) const
{
    if (!listItem) {
        return;
    }

    QFont itemFont = ui->listWidget->font();
    itemFont.setPointSize(SettingManager::Instance().all_setting_font_size());
    listItem->setFont(itemFont);
}

ClipboardItemType ClipboardView::currentFilterType() const
{
    switch (ui->typeComboBox->currentIndex()) {
    case 1:
        return ClipboardItemType::Text;
    case 2:
        return ClipboardItemType::Image;
    case 3:
        return ClipboardItemType::File;
    default:
        return ClipboardItemType::Unknown;
    }
}

bool ClipboardView::shouldDisplayItem(ClipboardItem* item) const
{
    if (!item) {
        return false;
    }
    if (item->isPinned() && !ui->showPinnedCheckBox->isChecked()) {
        return false;
    }

    const ClipboardItemType type = currentFilterType();
    if (type != ClipboardItemType::Unknown && item->type() != type) {
        return false;
    }

    const QString searchText = ui->lineEdit->text().trimmed();
    if (searchText.isEmpty()) {
        return true;
    }

    if (item->type() == ClipboardItemType::Text) {
        CliText* textItem = dynamic_cast<CliText*>(item);
        return textItem && textItem->text().contains(searchText, Qt::CaseInsensitive);
    }

    if (item->type() == ClipboardItemType::File) {
        const QString serialized = item->serialize();
        if (serialized.startsWith("FILE_DATA:")) {
            const QStringList filePaths = serialized.mid(10).split(";").filter(QRegularExpression(".+"));
            for (const QString& path : filePaths) {
                if (QFileInfo(path).fileName().contains(searchText, Qt::CaseInsensitive)) {
                    return true;
                }
            }
        }
    }

    return false;
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

void ClipboardView::copyItemAndMaybeCollapse(bool shouldCollapse)
{
    copyItem();
    if (shouldCollapse) {
        collapseWindow();
    }
}

void ClipboardView::collapseWindow()
{
    if (auto w = this->window()) {
        if (SettingManager::Instance().all_setting_fenable_tray()) {
            w->hide();
        } else {
            w->showNormal();
            w->setWindowState(Qt::WindowMinimized);
        }
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

void ClipboardView::on_clearCloudButton_clicked()
{
    const QMessageBox::StandardButton reply =
        QMessageBox::question(this,
                              QStringLiteral("清空云端记录"),
                              QStringLiteral("确定要清空当前账号的云端剪切板记录吗？"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    m_controller->clearCloudItems();
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
    copyItemAndMaybeCollapse(SettingManager::Instance().clip_board_double_click_copy_minimize());
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
                                    [this]{
                                        copyItemAndMaybeCollapse(
                                            SettingManager::Instance().clip_board_context_menu_copy_minimize());
                                    },
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
    Q_UNUSED(type);
    refreshAllItems();
}

void ClipboardView::onItemAdded(ClipboardItem* item)
{
    if (shouldDisplayItem(item)) {
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
        ClipboardItem* item = it->get();
        if (!shouldDisplayItem(item)) {
            continue;
        }
        if (item->isPinned()) {
            pinnedItems.push_back(item);
        } else if (item->isCloudItem()) {
            cloudItems.push_back(item);
        } else {
            normalItems.push_back(item);
        }
    }

    for (auto* item : pinnedItems) {
        addItemToListWidget(item);
    }
    for (auto it = cloudItems.rbegin(); it != cloudItems.rend(); ++it) {
        addItemToListWidget(*it);
    }
    for (auto* item : normalItems) {
        addItemToListWidget(item);
    }
}

void ClipboardView::on_showPinnedCheckBox_toggled(bool checked)
{
    Q_UNUSED(checked);
    refreshAllItems();
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
    refreshAllItems();
}
