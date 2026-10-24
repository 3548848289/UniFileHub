#include "./include/TagItemDelegate.h"

#include "../manager/include/FileLocationHelper.h"
#include "../manager/include/DataChangeNotifier.h"

#include <QDir>
#include <QHelpEvent>
#include <QMessageBox>
#include <QToolTip>
#include <QtGlobal>

namespace {
constexpr int kChipHeight = 22;          // 标签 chip 高度(行高 30px)
constexpr int kChipHPadding = 8;         // chip 内左右留白
constexpr int kChipGap = 4;              // 相邻 chip 间距
constexpr int kMaxChipTextWidth = 120;   // 标签文字超宽时省略
constexpr int kSubmissionIconRight = 60; // 备份角标占据 [right-60, right-40],位置与旧版一致
constexpr int kSubmissionReserve = 64;   // 有角标时 chip 至少让出 64px
constexpr int kEdgeMargin = 10;          // 无角标时 chip 距行右缘 10px
}

TagItemDelegate::TagItemDelegate(QObject *parent, ServerManager *serverManager)
    : QStyledItemDelegate(parent),
      serverManager(serverManager),
      dbservice(dbService::instance("./SmartDesk.db"))
{
    // 标签数据在任何地方发生变化(详情窗口保存、标签菜单删除等)后,
    // 清空缓存让文件列表的 chip 立即刷新
    connect(&DataChangeNotifier::Instance(), &DataChangeNotifier::tagChanged, this,
            [this](const QString &) {
                m_infoCache.clear();
                emit TagUpdated();
            });
    // 备份记录变化后同步刷新角标
    connect(&DataChangeNotifier::Instance(), &DataChangeNotifier::backupRecordAdded, this,
            [this](const QString &) {
                m_submissionsCache.clear();
                emit TagUpdated();
            });
}

void TagItemDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    const QString filePath = index.data(QFileSystemModel::FilePathRole).toString();

    const bool hasSubmissionIcon = hasSubmissions(filePath);
    if (hasSubmissionIcon) {
        const QRect submissionIconRect(option.rect.right() - kSubmissionIconRight,
                                       option.rect.top() + 5, 20, 20);
        const QIcon submissionIcon =
#if QT_VERSION_MAJOR >= 6
            QIcon::fromTheme(QIcon::ThemeIcon::EditCopy);
#else
            QIcon::fromTheme(QStringLiteral("edit-copy"));
#endif
        submissionIcon.paint(painter, submissionIconRect, Qt::AlignCenter);
    }

    const FilePathInfo &info = tagInfoFor(filePath);
    const QVector<TagChip> chips = layoutChips(option, info, hasSubmissionIcon);
    if (chips.isEmpty()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    for (const TagChip &chip : chips) {
        QColor background = chip.color;
        background.setAlpha(36);
        QColor border = chip.color;
        border.setAlpha(110);
        painter->setPen(QPen(border, 1));
        painter->setBrush(background);
        painter->drawRoundedRect(chip.rect, 9, 9);
        painter->setPen(chip.color);
        painter->drawText(chip.rect, Qt::AlignCenter, chip.text);
    }
    painter->restore();
}

bool TagItemDelegate::editorEvent(QEvent *event,
                                  QAbstractItemModel *model,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QString filePath = model->data(index, QFileSystemModel::FilePathRole).toString();

        const QRect submissionButtonRect(option.rect.right() - kSubmissionIconRight,
                                         option.rect.top() + 5, 20, 20);
        if (hasSubmissions(filePath) && submissionButtonRect.contains(mouseEvent->pos())) {
            isButtonClicked = true;
            emit subbutClicked(index);
            return true;
        }

        const FilePathInfo &info = tagInfoFor(filePath);
        const QVector<TagChip> chips = layoutChips(option, info, hasSubmissions(filePath));
        for (const TagChip &chip : chips) {
            if (!chip.rect.contains(mouseEvent->pos())) {
                continue;
            }

            isButtonClicked = true;
            emit tagbutClicked(index);
            TagDetail::showForPath(filePath); // 单例:同一文件复用已有详情窗口
            return true;
        }

        if (mouseEvent->button() == Qt::RightButton && option.rect.contains(mouseEvent->pos())) {
#if QT_VERSION_MAJOR >= 6
            showContextMenu(mouseEvent->globalPosition().toPoint(), index, model);
#else
            showContextMenu(mouseEvent->globalPos(), index, model);
#endif
            return true;
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

bool TagItemDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view,
                                const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::ToolTip && index.isValid()) {
        const QString filePath = index.data(QFileSystemModel::FilePathRole).toString();
        const FilePathInfo &info = tagInfoFor(filePath);
        if (!info.tagName.trimmed().isEmpty()) {
            QStringList lines;
            lines << QStringLiteral("标签:%1").arg(info.tagName);
            if (!info.annotation.trimmed().isEmpty()) {
                lines << QStringLiteral("备注:%1").arg(info.annotation);
            }
            if (info.expirationDate.isValid()) {
                lines << QStringLiteral("到期:%1").arg(
                    info.expirationDate.toString(QStringLiteral("yyyy-MM-dd HH:mm")));
            }
            QToolTip::showText(event->globalPos(), lines.join(QLatin1Char('\n')), view);
            return true;
        }
    }

    return QStyledItemDelegate::helpEvent(event, view, option, index);
}

const FilePathInfo &TagItemDelegate::tagInfoFor(const QString &filePath) const
{
    auto it = m_infoCache.find(filePath);
    if (it != m_infoCache.end()) {
        return it.value();
    }

    FilePathInfo info{};
    // 查询失败也可能是旧库缺备注行导致的"部分成功",保留已取到的标签字段
    dbservice.dbTags().getFileInfoByFilePath(filePath, info);
    info.filePath = filePath;

    return const_cast<TagItemDelegate *>(this)->m_infoCache.insert(filePath, info).value();
}

bool TagItemDelegate::hasSubmissions(const QString &filePath) const
{
    auto it = m_submissionsCache.find(filePath);
    if (it != m_submissionsCache.end()) {
        return it.value();
    }

    const bool has = dbservice.dbBackup().hasSubmissions(filePath);
    return const_cast<TagItemDelegate *>(this)->m_submissionsCache.insert(filePath, has).value();
}

QColor TagItemDelegate::tagColor(const QString &tagName)
{
    static const QColor palette[] = {
        QColor(37, 99, 235),   QColor(5, 150, 105),  QColor(217, 119, 6),
        QColor(220, 38, 38),   QColor(124, 58, 237), QColor(8, 145, 178),
        QColor(219, 39, 119),  QColor(101, 163, 13),
    };
    constexpr int paletteSize = sizeof(palette) / sizeof(palette[0]);
    if (tagName.isEmpty()) {
        return palette[0];
    }

    return palette[qHash(tagName) % paletteSize];
}

QVector<TagItemDelegate::TagChip> TagItemDelegate::layoutChips(const QStyleOptionViewItem &option,
                                                               const FilePathInfo &info,
                                                               bool hasSubmissionIcon) const
{
    QVector<TagChip> chips;

    const QString tagName = info.tagName.trimmed();
    if (tagName.isEmpty()) {
        return chips; // 没标签的文件不显示任何内容,打标走右键菜单"新建标签"
    }

    // 当前数据模型下一个文件对应一条标签;按列表渲染,后续支持多标签时无需改动绘制
    int right = option.rect.right() - (hasSubmissionIcon ? kSubmissionReserve : kEdgeMargin);
    const int top = option.rect.top() + (option.rect.height() - kChipHeight) / 2;
    const QFontMetrics fm(option.font);
    const QStringList tagNames = { tagName };
    for (const QString &raw : tagNames) {
        const QString text = fm.elidedText(raw, Qt::ElideRight, kMaxChipTextWidth);
        const int width = fm.horizontalAdvance(text) + kChipHPadding * 2;
        chips.append(TagChip{ text, tagColor(raw), QRect(right - width, top, width, kChipHeight) });
        right -= width + kChipGap;
    }

    return chips;
}

void TagItemDelegate::showContextMenu(const QPoint &pos,
                                      const QModelIndex &index,
                                      QAbstractItemModel *model)
{
    QMenu contextMenu;
    auto *openAction = new QAction(QStringLiteral("打开文件"), &contextMenu);
    auto *renameAction = new QAction(QStringLiteral("重命名"), &contextMenu);
    auto *deleteAction = new QAction(QStringLiteral("删除文件"), &contextMenu);
    auto *newTagAction = new QAction(QStringLiteral("新建标签"), &contextMenu);
    auto *commitAction = new QAction(QStringLiteral("文件备份"), &contextMenu);
    auto *uploadToDriveAction = new QAction(QStringLiteral("上传到网盘"), &contextMenu);
    auto *copyPathAction = new QAction(QStringLiteral("复制路径"), &contextMenu);
    auto *openInExplorerAction = new QAction(QStringLiteral("在文件夹中打开"), &contextMenu);
#if UNIFILEHUB_ENABLE_TERMINAL
    auto *openInTerminalAction = new QAction(QStringLiteral("在终端打开"), &contextMenu);
#endif

    connect(openAction, &QAction::triggered, [this, model, index]() { onOpenFileTriggered(model, index); });
    connect(renameAction, &QAction::triggered, [this, model, index]() { onRenameTriggered(model, index); });
    connect(deleteAction, &QAction::triggered, [this, model, index]() { onDeleteFileTriggered(model, index); });
    connect(newTagAction, &QAction::triggered, [this, model, index]() { onNewTagTriggered(model, index); });
    connect(commitAction, &QAction::triggered, [this, model, index]() { onCommitTriggered(model, index); });
    connect(uploadToDriveAction, &QAction::triggered, [this, model, index]() { onUploadToDriveTriggered(model, index); });
    connect(copyPathAction, &QAction::triggered, [this, model, index]() { onCopyPathTriggered(model, index); });
    connect(openInExplorerAction, &QAction::triggered, [this, model, index]() { onOpenInExplorer(model, index); });
#if UNIFILEHUB_ENABLE_TERMINAL
    connect(openInTerminalAction, &QAction::triggered, [this, model, index]() { onOpenInTerminal(model, index); });
#endif

    contextMenu.addAction(openAction);
    contextMenu.addAction(renameAction);
    contextMenu.addAction(deleteAction);
    contextMenu.addAction(copyPathAction);
    contextMenu.addAction(openInExplorerAction);
    contextMenu.addAction(newTagAction);
    contextMenu.addAction(commitAction);
    contextMenu.addAction(uploadToDriveAction);
#if UNIFILEHUB_ENABLE_TERMINAL
    contextMenu.addAction(openInTerminalAction);
#endif
    contextMenu.exec(pos);
}

void TagItemDelegate::onNewTagTriggered(QAbstractItemModel *model, const QModelIndex &index)
{
    const QString filePath = model->data(index, QFileSystemModel::FilePathRole).toString();
    AddTag tagDialog;
    tagDialog.setTargetFile(filePath);
    addTag(model, index, tagDialog);
}

void TagItemDelegate::onOpenFileTriggered(QAbstractItemModel *model, const QModelIndex &index)
{
    const QString filePath = model->data(index, QFileSystemModel::FilePathRole).toString();
    emit openFileRequested(filePath);
}

void TagItemDelegate::onDeleteFileTriggered(QAbstractItemModel *model, const QModelIndex &index)
{
    const QString filePath = model->data(index, QFileSystemModel::FilePathRole).toString();

    QMessageBox msgBox;
    msgBox.setWindowTitle(QStringLiteral("删除文件/文件夹"));
    msgBox.setText(QStringLiteral("你确定要删除这个文件或文件夹吗？"));
    msgBox.setWindowIcon(QIcon::fromTheme("utilities-system-monitor"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    const int ret = msgBox.exec();
    if (ret != QMessageBox::Yes) {
        return;
    }

    const QFileInfo fileInfo(filePath);
    bool success = false;

    if (fileInfo.isDir()) {
        QDir dir(filePath);
        success = dir.removeRecursively();
    } else {
        QFile file(filePath);
        success = file.remove();
    }

    if (success) {
        model->removeRow(index.row());
        // 磁盘文件已删除,同步清理标签/备注记录,避免管理页残留死卡片
        cleanupTagRecordsForRemovedPath(filePath);
        DataChangeNotifier::Instance().notifyTagChanged(filePath);
    } else {
        QMessageBox::warning(nullptr,
                             QStringLiteral("删除失败"),
                             QStringLiteral("无法删除: %1\n可能没有删除权限").arg(filePath));
    }

    emit deleteFileRequested(filePath);
}

void TagItemDelegate::cleanupTagRecordsForRemovedPath(const QString &removedPath)
{
    const QString normalized = QDir::fromNativeSeparators(removedPath);
    const QString childPrefix = normalized + QLatin1Char('/');

    // 删除的可能是目录:其下所有带标签的子文件记录一并清理
    const QStringList allPaths = dbservice.dbTags().getAllFilePaths();
    for (const QString &path : allPaths) {
        const QString candidate = QDir::fromNativeSeparators(path);
        if (candidate.compare(normalized, Qt::CaseInsensitive) != 0
            && !candidate.startsWith(childPrefix, Qt::CaseInsensitive)) {
            continue;
        }

        int fileId = -1;
        if (dbservice.dbTags().getFileId(path, fileId)) {
            dbservice.dbTags().deleteTag(fileId);
        }
        m_infoCache.remove(path);
        m_submissionsCache.remove(path);
    }
}

void TagItemDelegate::onRenameTriggered(QAbstractItemModel *model, const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    auto *fileSystemModel = qobject_cast<QFileSystemModel *>(model);
    if (!fileSystemModel) {
        return;
    }

    const QString filePath = fileSystemModel->filePath(index);
    const QFileInfo fileInfo(filePath);
    const QString currentName = fileInfo.fileName();

    bool ok = false;
    const QString newName = QInputDialog::getText(nullptr,
                                                  QStringLiteral("重命名"),
                                                  QStringLiteral("输入新名称："),
                                                  QLineEdit::Normal,
                                                  currentName,
                                                  &ok)
                                .trimmed();
    if (!ok || newName.isEmpty() || newName == currentName) {
        return;
    }

    if (newName.contains('/') || newName.contains('\\')) {
        QMessageBox::warning(nullptr,
                             QStringLiteral("重命名失败"),
                             QStringLiteral("名称不能包含路径分隔符。"));
        return;
    }

    const QString targetPath = fileInfo.dir().absoluteFilePath(newName);
    if (QFileInfo::exists(targetPath)) {
        QMessageBox::warning(nullptr,
                             QStringLiteral("重命名失败"),
                             QStringLiteral("目标名称已存在。"));
        return;
    }

    if (!fileSystemModel->setData(index, newName, Qt::EditRole)) {
        QMessageBox::warning(nullptr,
                             QStringLiteral("重命名失败"),
                             QStringLiteral("无法将 %1 重命名为 %2").arg(currentName, newName));
    }
}

void TagItemDelegate::onCommitTriggered(QAbstractItemModel *model, const QModelIndex &index)
{
    const QString filePath = model->data(index, QFileSystemModel::FilePathRole).toString();
    auto *commitDialog = new FileBackupWid(filePath, nullptr);
    if (commitDialog->exec() == QDialog::Accepted) {
        const QString backupFilePath = commitDialog->getBackupFilePath();
        dbservice.dbBackup().recordSubmission(filePath, backupFilePath);
        DataChangeNotifier::Instance().notifyBackupRecordAdded(filePath);
    }
    commitDialog->deleteLater();
}

void TagItemDelegate::onUploadToDriveTriggered(QAbstractItemModel *model, const QModelIndex &index)
{
    const QString filePath = model->data(index, QFileSystemModel::FilePathRole).toString();
    emit uploadToDriveRequested(filePath);
}

void TagItemDelegate::addTag(const QAbstractItemModel *model, const QModelIndex &index, AddTag &tagDialog)
{
    if (tagDialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString tagName = tagDialog.getTagName();
    if (tagName.isEmpty()) {
        return;
    }
    const QString annotation = tagDialog.getAnnotation();
    const QDateTime expirationDate = tagDialog.getExpirationDate();
    const int reminderHours = tagDialog.getReminderTime();
    const int intervalMinutes = tagDialog.getIntervalTime();
    const QString reminderType = tagDialog.getReminderType();
    const QString filePath = model->data(index, QFileSystemModel::FilePathRole).toString();

    int fileId = -1;

    FilePathInfo fileInfo;
    fileInfo.filePath = filePath;
    fileInfo.tagName = tagName;
    fileInfo.expirationDate = expirationDate;
    fileInfo.annotation = annotation;
    fileInfo.reminderTime = reminderHours;
    fileInfo.intervalTime = intervalMinutes;
    fileInfo.lastReminderIndex = -1;
    fileInfo.reminderType = reminderType;

    if (!dbservice.dbTags().getFileId(filePath, fileId)) {
        dbservice.dbTags().addFilePath(filePath, fileId);
        dbservice.dbTags().saveTags(fileId, tagName);
        dbservice.dbTags().saveAnnotation(fileId, annotation);
        dbservice.dbTags().updateFileInfo(fileInfo);
    } else {
        dbservice.dbTags().updateFileInfo(fileInfo);
    }

    m_infoCache.remove(filePath);
    emit TagUpdated();
    DataChangeNotifier::Instance().notifyTagChanged(filePath);
}

void TagItemDelegate::onCopyPathTriggered(QAbstractItemModel *model, const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    auto *fileSystemModel = qobject_cast<QFileSystemModel *>(model);
    if (!fileSystemModel) {
        return;
    }

    const QString filePath = fileSystemModel->filePath(index);
    if (filePath.isEmpty()) {
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(filePath);
}

void TagItemDelegate::onOpenInExplorer(QAbstractItemModel *model, const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    auto *fileSystemModel = qobject_cast<QFileSystemModel *>(model);
    if (!fileSystemModel) {
        return;
    }

    const QString filePath = fileSystemModel->filePath(index);
    FileLocationHelper::openFileLocationWithSelection(filePath);
}

void TagItemDelegate::onOpenInTerminal(QAbstractItemModel *model, const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    auto *fileSystemModel = qobject_cast<QFileSystemModel *>(model);
    if (!fileSystemModel) {
        return;
    }

    const QString filePath = fileSystemModel->filePath(index);
    const QString directoryPath = fileSystemModel->isDir(index)
                                      ? filePath
                                      : QFileInfo(filePath).absolutePath();
    if (!directoryPath.isEmpty()) {
        emit openTerminalRequested(directoryPath);
    }
}

void TagItemDelegate::onTagDeleted(const QString &filePath)
{
    Q_UNUSED(filePath);
    m_infoCache.clear();
    m_submissionsCache.clear();
    emit TagUpdated();
}
