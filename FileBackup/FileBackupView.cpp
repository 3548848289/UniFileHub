#include "include/FileBackupView.h"
#include "ui/ui_FileBackupView.h"
#include "../PersonalDrive/include/DriveUploadHelper.h"
#include "../manager/include/DataChangeNotifier.h"
#include "../Setting/include/ThemeManager.h"

#include <QStyledItemDelegate>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <algorithm>

namespace {

constexpr int kItemPadding = 10;

// 文件大小的可读表示
QString humanFileSize(qint64 bytes)
{
    if (bytes < 1024)
        return QString::number(bytes) + " B";
    double value = bytes / 1024.0;
    static const char *units[] = { "KB", "MB", "GB", "TB" };
    int unit = 0;
    while (value >= 1024.0 && unit < 3) {
        value /= 1024.0;
        ++unit;
    }
    return QString::number(value, 'f', 1) + " " + units[unit];
}

// 备份提交时间的相对显示（今天/昨天/今年/往年）
QString relativeSubmitTime(const QDateTime &time)
{
    if (!time.isValid())
        return QString();
    const QDate date = time.date();
    const QDate today = QDate::currentDate();
    if (date == today)
        return time.toString("今天 HH:mm");
    if (date == today.addDays(-1))
        return time.toString("昨天 HH:mm");
    if (date.year() == today.year())
        return time.toString("M月d日 HH:mm");
    return time.toString("yyyy年M月d日");
}

// 版本列表条目绘制：第一行备份文件名，第二行提交时间/大小，行尾状态徽标
class BackupItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(index);
        QFont smallFont = option.font;
        smallFont.setPointSizeF(smallFontSize(option));
        const int height = option.fontMetrics.height()
                           + QFontMetrics(smallFont).height() + 14;
        return QSize(option.rect.width(), height);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QString path = index.data(Qt::UserRole).toString();
        const bool missing = index.data(Qt::UserRole + 1).toBool();
        const int cloudStatus = index.data(FileBackupView::CloudStatusRole).toInt();
        const QDateTime submitTime = index.data(FileBackupView::SubmitTimeRole).toDateTime();
        const qint64 fileSize = index.data(FileBackupView::FileSizeRole).toLongLong();

        painter->save();
        painter->setClipRect(option.rect);  // 绘制内容不越出本条目区域
        const bool selected = option.state & QStyle::State_Selected;
        if (selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else if (option.state & QStyle::State_MouseOver) {
            // 半透明中性色做悬停高亮，明暗主题下都可见
            QColor hover = option.palette.mid().color();
            hover.setAlpha(40);
            painter->fillRect(option.rect, hover);
        }

        const QColor normalText = selected ? option.palette.highlightedText().color()
                                           : option.palette.text().color();
        const QColor secondaryText = selected ? normalText : option.palette.mid().color();

        // 第一行：备份文件名
        QRect nameRect = option.rect.adjusted(kItemPadding, 5, -kItemPadding, 0);
        nameRect.setHeight(option.fontMetrics.height());
        painter->setFont(option.font);
        painter->setPen(normalText);
        painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                          option.fontMetrics.elidedText(QFileInfo(path).fileName(),
                                                        Qt::ElideMiddle, nameRect.width()));

        // 第二行：提交时间 · 大小，右侧留给状态徽标
        QFont smallFont = option.font;
        smallFont.setPointSizeF(smallFontSize(option));
        const QFontMetrics smallMetrics(smallFont);

        QStringList metaParts;
        const QString timeText = relativeSubmitTime(submitTime);
        if (!timeText.isEmpty())
            metaParts << timeText;
        if (!missing && fileSize > 0)
            metaParts << humanFileSize(fileSize);
        const QString metaText = metaParts.join(" · ");

        const QStringList badges = badgeTexts(missing, cloudStatus);
        int badgeSpace = 0;
        for (const QString &badge : badges)
            badgeSpace += smallMetrics.horizontalAdvance(badge) + 12 + 6;

        // 注意 adjusted() 的参数是相对偏移，这里用绝对坐标直接构造，避免随条目位置累积偏移
        QRect metaRect = option.rect.adjusted(kItemPadding, nameRect.height() + 8,
                                              -kItemPadding - badgeSpace, -6);
        metaRect.setHeight(smallMetrics.height());
        if (!metaText.isEmpty()) {
            painter->setFont(smallFont);
            painter->setPen(secondaryText);
            painter->drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter,
                              smallMetrics.elidedText(metaText, Qt::ElideRight, metaRect.width()));
        }

        // 行尾徽标从右往左排布
        const bool darkMode = ThemeManager::Instance().isDarkMode();
        int right = option.rect.right() - kItemPadding;
        for (const QString &badge : badges) {
            const QColor color = badgeColor(badge, option.palette, darkMode);
            const int badgeHeight = smallMetrics.height() + 4;
            const int badgeWidth = smallMetrics.horizontalAdvance(badge) + 12;
            const QRect badgeRect(right - badgeWidth, metaRect.center().y() - badgeHeight / 2,
                                  badgeWidth, badgeHeight);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(color.red(), color.green(), color.blue(), 32));
            painter->drawRoundedRect(badgeRect, badgeHeight / 2, badgeHeight / 2);
            painter->setFont(smallFont);
            painter->setPen(color);
            painter->drawText(badgeRect, Qt::AlignCenter, badge);
            right -= badgeWidth + 6;
        }
        painter->restore();
    }

private:
    static qreal smallFontSize(const QStyleOptionViewItem &option)
    {
        return qMax(7.0, option.font.pointSizeF() * 0.85);
    }

    static QStringList badgeTexts(bool missing, int cloudStatus)
    {
        QStringList badges;
        if (missing)
            badges << QStringLiteral("本地缺失");
        if (cloudStatus == FileBackupView::CloudExists)
            badges << QStringLiteral("已上云");
        else if (cloudStatus == FileBackupView::CloudMissing)
            badges << QStringLiteral("未上云");
        return badges;
    }

    static QColor badgeColor(const QString &badge, const QPalette &palette, bool darkMode)
    {
        if (badge == QStringLiteral("本地缺失"))
            return QColor(0xE6, 0x7E, 0x22);
        if (badge == QStringLiteral("已上云"))
            return darkMode ? QColor(0x2E, 0xCC, 0x71) : QColor(0x1E, 0x84, 0x49);
        return palette.mid().color();  // 未上云
    }
};

}  // namespace

FileBackupView::FileBackupView(QWidget *parent) : QWidget(parent), ui(new Ui::FileBackupView)
    , serverManager(ServerManager::instance()) , dbservice(dbService::instance("./SmartDesk.db")) {
    ui->setupUi(this);
    m_messagePopup = new InlineMessagePopup(this);
    m_messagePopup->setPanelWidget(this); // 弹窗显示在面板内部顶部居中
    ui->fileListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->backupList->setContextMenuPolicy(Qt::CustomContextMenu);

    // 版本列表条目用自定义委托绘制：文件名 + 提交时间/大小/状态徽标
    ui->backupList->setItemDelegate(new BackupItemDelegate(ui->backupList));
    ui->backupList->setMouseTracking(true);  // 开启条目悬停高亮
    updateActionStates();  // 初始无选中，操作栏整体置灰

    // 为文件列表添加右键菜单
    connect(ui->fileListWidget, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QListWidgetItem *item = ui->fileListWidget->itemAt(pos);
        if (!item)
            return;
        ui->fileListWidget->setCurrentItem(item);  // 右键同步选中

        const QString filePath = item->data(Qt::UserRole).toString();
        if (filePath.isEmpty())
            return;

        QMenu contextMenu(this);
        QAction *changePathAction = contextMenu.addAction("更换文件路径");
        QAction *deleteRecordAction = contextMenu.addAction("删除文件记录");
        QAction *openInFileSystemAction = contextMenu.addAction("在文件系统打开");
        QAction *openFileLocationAction = contextMenu.addAction("打开文件所在位置");
        QAction *uploadToDriveAction = contextMenu.addAction("上传到网盘");

        // 处理菜单项点击
        connect(changePathAction, &QAction::triggered, this, [this, item, filePath]() {
            QString newFilePath = QFileDialog::getOpenFileName(this, tr("选择新的文件路径"), "", tr("所有文件 (*)"));
            if (newFilePath.isEmpty() || newFilePath == filePath)
                return;
            if (!dbservice.dbBackup().updateSubmissions(filePath, newFilePath)) {
                m_messagePopup->showMessage(tr("更新文件路径失败。"), true);
                return;
            }
            // 源文件路径变了，条目文本和缺失状态同步刷新
            const QFileInfo fileInfo(newFilePath);
            const bool missing = !fileInfo.exists();
            item->setData(Qt::UserRole, newFilePath);
            item->setText(fileInfo.fileName());
            item->setData(Qt::UserRole + 1, missing);
            item->setToolTip(newFilePath);
            item->setIcon(missing ? QIcon::fromTheme("dialog-warning", QIcon(":/usedimage/tips.png")) : QIcon());
        });

        connect(deleteRecordAction, &QAction::triggered, this, [this, item, filePath]() {
            QList<QString> backupFileNames = dbservice.dbBackup().getBackupFileList(filePath);
            const QMessageBox::StandardButton answer = QMessageBox::question(
                this, "删除文件记录",
                QString("确定删除该文件的全部备份吗？共 %1 个备份文件将从磁盘一并删除，且无法恢复。\n%2")
                    .arg(backupFileNames.size()).arg(filePath));
            if (answer != QMessageBox::Yes)
                return;

            if (!dbservice.dbBackup().deleteAll(filePath)) {
                m_messagePopup->showMessage(tr("从数据库中删除文件记录失败。"), true);
                return;
            }
            // 删除磁盘上的所有备份文件
            for (const QString &backupFile : backupFileNames) {
                deleteBackupFile(backupFile);  // 删除实际文件
            }

            delete item;  // 选中变化会自动清空右侧备份列表
        });

        connect(openInFileSystemAction, &QAction::triggered, this, [this, filePath]() {
            emit openInFileSystemRequested(filePath);
        });

        connect(openFileLocationAction, &QAction::triggered, this, [filePath]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
        });

        connect(uploadToDriveAction, &QAction::triggered, this, [this, filePath]() {
            uploadFileToDrive(filePath);
        });

        // 显示菜单
        contextMenu.exec(ui->fileListWidget->mapToGlobal(pos));
    });

    // 搜索框：按路径关键字过滤文件列表
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &FileBackupView::applyFileFilter);

    // 文件列表选中变化：提示缺失并刷新右侧备份列表
    connect(ui->fileListWidget, &QListWidget::itemSelectionChanged,
            this, &FileBackupView::onFileCurrentChanged);

    // 版本列表：单击仅选中（同步操作栏状态），双击才在编辑器中打开
    connect(ui->backupList, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *item) { openBackupItem(item); });
    connect(ui->backupList, &QListWidget::itemSelectionChanged,
            this, &FileBackupView::updateActionStates);

    // 底部操作栏
    connect(ui->openBtn, &QPushButton::clicked, this,
            [this] { openBackupItem(ui->backupList->currentItem()); });
    connect(ui->restoreBtn, &QPushButton::clicked, this, [this] {
        QListWidgetItem *item = ui->backupList->currentItem();
        if (!item)
            return;
        const QString file = item->data(Qt::UserRole).toString();
        file_restore_wid = new FileRestoreWid(file, item->data(Qt::UserRole + 1).toBool(), this);
        file_restore_wid->exec();
    });
    connect(ui->saveAsBtn, &QPushButton::clicked, this, [this] {
        QListWidgetItem *item = ui->backupList->currentItem();
        if (!item)
            return;
        const QString file = item->data(Qt::UserRole).toString();
        QString savePath = QFileDialog::getSaveFileName(
            this, tr("另存为"), QDir::homePath() + "/" + QFileInfo(file).fileName(),
            tr("所有文件 (*)"));
        if (savePath.isEmpty())
            return;
        QString error;
        if (!FileRestoreWid::copyFileSafely(file, savePath, &error)) {
            m_messagePopup->showMessage(error, true);
            return;
        }
        m_messagePopup->showMessage("文件另存成功!");
    });
    connect(ui->commitBtn, &QPushButton::clicked, this, [this] {
        QListWidgetItem *item = ui->backupList->currentItem();
        if (!item)
            return;
        serverManager->commitFile(item->data(Qt::UserRole).toString());
    });
    connect(ui->deleteBtn, &QPushButton::clicked, this, [this] {
        QListWidgetItem *item = ui->backupList->currentItem();
        if (!item)
            return;
        const QString file = item->data(Qt::UserRole).toString();

        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, "删除备份记录",
            QString("确定删除该备份记录吗？对应的备份文件将从磁盘一并删除，且无法恢复。\n%1").arg(file));
        if (answer != QMessageBox::Yes)
            return;

        if (!dbservice.dbBackup().deleteBackupRecord(file)) {
            m_messagePopup->showMessage("从数据库中删除备份记录失败。", true);
            return;
        }
        if (QFile::exists(file) && !deleteBackupFile(file))
            m_messagePopup->showMessage("备份记录已删除，但磁盘上的备份文件删除失败。", true);
        delete item;
    });

    // 云端存在性检查结果按条目回填（信号携带文件名，避免多个请求之间状态错乱）
    connect(serverManager, &ServerManager::fileExistsChecked, this,
            [this](const QString &fileName, bool exists) {
        for (int i = 0; i < ui->backupList->count(); ++i) {
            QListWidgetItem *item = ui->backupList->item(i);
            if (QFileInfo(item->data(Qt::UserRole).toString()).fileName() == fileName) {
                item->setData(FileBackupView::CloudStatusRole,
                              exists ? FileBackupView::CloudExists : FileBackupView::CloudMissing);
                break;
            }
        }
    });

    connect(serverManager, &ServerManager::onFilesListUpdated, this, &FileBackupView::updateFileList);

    // 文件系统里新增备份记录后自动刷新，不用手动点刷新按钮
    connect(&DataChangeNotifier::Instance(), &DataChangeNotifier::backupRecordAdded,
            this, &FileBackupView::onBackupRecordAdded);

    loadFileNames();
}

FileBackupView::~FileBackupView()
{
    delete ui;
}

void FileBackupView::loadFileNames()
{
    ui->fileListWidget->clear();
    ui->backupList->clear();
    const QIcon missingIcon = QIcon::fromTheme("dialog-warning", QIcon(":/usedimage/tips.png"));
    QList<QString> fileNames = dbservice.dbBackup().getAllFileNames();
    for (const QString &filePath : fileNames) {
        QListWidgetItem *item = new QListWidgetItem(QFileInfo(filePath).fileName());
        const bool missing = !QFile::exists(filePath);
        if (missing)
            item->setIcon(missingIcon);
        item->setData(Qt::UserRole, filePath);
        item->setData(Qt::UserRole + 1, missing);
        item->setToolTip(filePath);
        ui->fileListWidget->addItem(item);
    }

    applyFileFilter(ui->searchEdit->text());

    // 与原下拉框一致：默认选中第一个（可见的）文件
    if (!ui->fileListWidget->currentItem()) {
        for (int i = 0; i < ui->fileListWidget->count(); ++i) {
            if (!ui->fileListWidget->item(i)->isHidden()) {
                ui->fileListWidget->setCurrentRow(i);
                break;
            }
        }
    }
}

void FileBackupView::updateFileList(const QString filepath)
{
    if (QListWidgetItem *item = findFileItem(filepath))
        ui->fileListWidget->setCurrentItem(item);
    else
        qDebug() << "File not found in the file list.";
}

// 文件列表选中变化：缺失文件给出提示，并刷新右侧备份列表
void FileBackupView::onFileCurrentChanged()
{
    QListWidgetItem *item = ui->fileListWidget->currentItem();
    if (!item) {
        ui->backupList->clear();
        return;
    }
    if (item->data(Qt::UserRole + 1).toBool())
        m_messagePopup->showMessage("您选择的文件已经缺失，请更改源文件路径或选择其他文件。", true);
    fillBackupList(item->data(Qt::UserRole).toString());
}

// 按路径关键字过滤文件列表（不区分大小写），当前选中被过滤掉时改选第一个可见项
void FileBackupView::applyFileFilter(const QString &text)
{
    const QString keyword = text.trimmed();
    for (int i = 0; i < ui->fileListWidget->count(); ++i) {
        QListWidgetItem *item = ui->fileListWidget->item(i);
        item->setHidden(!keyword.isEmpty()
                        && !item->data(Qt::UserRole).toString().contains(keyword, Qt::CaseInsensitive));
    }

    QListWidgetItem *current = ui->fileListWidget->currentItem();
    if (current && current->isHidden()) {
        ui->fileListWidget->setCurrentRow(-1);
        for (int i = 0; i < ui->fileListWidget->count(); ++i) {
            if (!ui->fileListWidget->item(i)->isHidden()) {
                ui->fileListWidget->setCurrentRow(i);
                break;
            }
        }
    }
}

QListWidgetItem *FileBackupView::findFileItem(const QString &filePath) const
{
    for (int i = 0; i < ui->fileListWidget->count(); ++i) {
        QListWidgetItem *item = ui->fileListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == filePath)
            return item;
    }
    return nullptr;
}

void FileBackupView::fillBackupList(const QString &filePath) {
    ui->backupList->clear();

    QList<QString> backupFileNames = dbservice.dbBackup().getBackupFileNames(filePath);

    // 按提交时间倒序展示，最新备份在最上面
    struct VersionInfo { QString path; QDateTime time; };
    QList<VersionInfo> versions;
    for (const QString &backupFileName : backupFileNames)
        versions.append({backupFileName, dbservice.dbBackup().getSubTime(backupFileName)});
    std::sort(versions.begin(), versions.end(),
              [](const VersionInfo &a, const VersionInfo &b) { return a.time > b.time; });

    for (const VersionInfo &version : versions) {
        const QFileInfo fileInfo(version.path);
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());

        const bool fileMissing = !fileInfo.exists();
        if (fileMissing)
            item->setIcon(QIcon::fromTheme("dialog-warning", QIcon(":/usedimage/tips.png")));
        item->setData(Qt::UserRole, version.path);
        item->setData(Qt::UserRole + 1, fileMissing);
        item->setData(FileBackupView::SubmitTimeRole, version.time);
        item->setData(FileBackupView::FileSizeRole, fileMissing ? 0 : fileInfo.size());
        item->setData(FileBackupView::CloudStatusRole, FileBackupView::CloudUnknown);
        item->setToolTip(fileMissing ? "文件已缺失" : version.path);
        ui->backupList->addItem(item);

        // 异步查询该版本是否已上传云端，结果按条目回填
        serverManager->checkFileExists(fileInfo.fileName());
    }
}

// 文件系统新增备份记录后自动刷新：重填文件列表并保持当前选中，
// 若新增记录正好属于当前选中文件，其备份列表会同步显示新备份。
void FileBackupView::onBackupRecordAdded(const QString &filePath)
{
    QListWidgetItem *previous = ui->fileListWidget->currentItem();
    const QString previousFile = previous ? previous->data(Qt::UserRole).toString() : QString();

    {
        // 阻塞信号，避免重填过程中触发选中变化把备份列表换成别的文件
        const QSignalBlocker blocker(ui->fileListWidget);
        loadFileNames();

        QListWidgetItem *target = nullptr;
        if (!previousFile.isEmpty())
            target = findFileItem(previousFile);
        if (!target)
            target = findFileItem(filePath);
        if (!target) {
            for (int i = 0; i < ui->fileListWidget->count(); ++i) {
                if (!ui->fileListWidget->item(i)->isHidden()) {
                    target = ui->fileListWidget->item(i);
                    break;
                }
            }
        }
        if (target)
            ui->fileListWidget->setCurrentItem(target);
    }

    // 信号被阻塞期间选中变化不会触发刷新，这里手动补上
    if (QListWidgetItem *current = ui->fileListWidget->currentItem())
        fillBackupList(current->data(Qt::UserRole).toString());
    else
        ui->backupList->clear();
}

// 双击或点击操作栏"打开"：在编辑器中打开该版本
void FileBackupView::openBackupItem(QListWidgetItem *item)
{
    if (!item)
        return;
    const QString file = item->data(Qt::UserRole).toString();
    if (file.isEmpty())
        return;
    if (!QFile::exists(file)) {
        m_messagePopup->showMessage("文件已缺失，无法打开。", true);
        return;
    }
    emit s_fileopen(file);
}

// 依据当前选中条目刷新底部操作栏的可用状态
void FileBackupView::updateActionStates()
{
    QListWidgetItem *item = ui->backupList->currentItem();
    const QString filePath = item ? item->data(Qt::UserRole).toString() : QString();
    const bool hasLocalFile = !filePath.isEmpty() && QFile::exists(filePath);

    ui->openBtn->setEnabled(hasLocalFile);
    ui->restoreBtn->setEnabled(item != nullptr);
    ui->saveAsBtn->setEnabled(hasLocalFile);
    ui->commitBtn->setEnabled(hasLocalFile);
    ui->deleteBtn->setEnabled(item != nullptr);
}

void FileBackupView::on_backupList_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = ui->backupList->itemAt(pos);
    if (!item)
        return;

    ui->backupList->setCurrentItem(item);  // 右键同步选中，操作栏跟随高亮

    const QString file = item->data(Qt::UserRole).toString();
    const bool hasLocalFile = !file.isEmpty() && QFile::exists(file);
    const bool onCloud = item->data(FileBackupView::CloudStatusRole).toInt()
                         == FileBackupView::CloudExists;

    // 高频操作已上移到底部操作栏，右键菜单只保留低频操作
    QMenu contextMenu(this);
    QAction *actionReplace = contextMenu.addAction("更换路径…");
    QAction *actionUploadDrive = contextMenu.addAction("上传到网盘");
    QAction *actionRecover = contextMenu.addAction("从云端恢复");
    actionUploadDrive->setEnabled(hasLocalFile);
    actionRecover->setEnabled(onCloud);

    connect(actionReplace, &QAction::triggered, this, [this, item, file]() {
        QString newFilePath = QFileDialog::getOpenFileName(this, tr("选择文件"), "", tr("所有文件 (*)"));
        if (newFilePath.isEmpty() || newFilePath == file)
            return;
        if (!dbservice.dbBackup().updateFilePath(file, newFilePath)) {
            m_messagePopup->showMessage("更新备份路径失败。", true);
            return;
        }
        // 换路径后同步刷新条目状态（大小/缺失/tooltip），云端状态按新文件名重查
        const QFileInfo fileInfo(newFilePath);
        const bool missing = !fileInfo.exists();
        item->setData(Qt::UserRole, newFilePath);
        item->setText(fileInfo.fileName());
        item->setData(Qt::UserRole + 1, missing);
        item->setData(FileBackupView::FileSizeRole, missing ? 0 : fileInfo.size());
        item->setData(FileBackupView::CloudStatusRole, FileBackupView::CloudUnknown);
        item->setToolTip(missing ? "文件已缺失" : newFilePath);
        item->setIcon(missing ? QIcon::fromTheme("dialog-warning", QIcon(":/usedimage/tips.png")) : QIcon());
        serverManager->checkFileExists(fileInfo.fileName());
        updateActionStates();  // 本地存在性可能变化，操作栏状态同步
    });

    connect(actionUploadDrive, &QAction::triggered, this, [this, file]() {
        uploadFileToDrive(file);
    });

    connect(actionRecover, &QAction::triggered, this, [this, file]() {
        serverManager->downloadFile(file);
    });

    contextMenu.exec(ui->backupList->mapToGlobal(pos));
}


void FileBackupView::uploadFileToDrive(const QString &filePath)
{
    if (!QFileInfo(filePath).isFile()) {
        m_messagePopup->showMessage("文件不存在或已缺失，无法上传到网盘。", true);
        return;
    }

    QString uploadError;
    if (DriveUploadHelper::uploadToCurrentDirectory(this, filePath, &uploadError)) {
        m_messagePopup->showMessage(QString("已开始上传到网盘（网盘当前位置）：%1").arg(QFileInfo(filePath).fileName()));
    } else if (!uploadError.isEmpty()) {
        m_messagePopup->showMessage(uploadError, true);
    }
}


bool FileBackupView::deleteBackupFile(const QString &backupFilePath) {
    QFile file(backupFilePath);
    if (file.exists()) {
        if (file.remove()) {
            return true;
        } else {
            qDebug() << "Failed to delete file:" << backupFilePath;
            return false;
        }
    } else {
        qDebug() << "File does not exist:" << backupFilePath;
        return false;
    }
}


void FileBackupView::on_refreshBtn_clicked()
{
    loadFileNames();
}
