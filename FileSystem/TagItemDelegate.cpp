#include "./include/TagItemDelegate.h"

#include "../manager/include/FileLocationHelper.h"

#include <QDir>
#include <QMessageBox>

TagItemDelegate::TagItemDelegate(QObject *parent, ServerManager *serverManager)
    : QStyledItemDelegate(parent),
      serverManager(serverManager),
      dbservice(dbService::instance("./SmartDesk.db"))
{
}

void TagItemDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    const QString filePath = index.data(QFileSystemModel::FilePathRole).toString();

    if (hasTags(filePath)) {
        const QRect iconRect(option.rect.right() - 30, option.rect.top() + 5, 20, 20);
        const QIcon tagIcon = QIcon::fromTheme(QIcon::ThemeIcon::MailMessageNew);
        tagIcon.paint(painter, iconRect, Qt::AlignCenter);
    }

    if (dbservice.dbBackup().hasSubmissions(filePath)) {
        const QRect submissionIconRect(option.rect.right() - 60, option.rect.top() + 5, 20, 20);
        const QIcon submissionIcon = QIcon::fromTheme(QIcon::ThemeIcon::EditCopy);
        submissionIcon.paint(painter, submissionIconRect, Qt::AlignCenter);
    }
}

bool TagItemDelegate::editorEvent(QEvent *event,
                                  QAbstractItemModel *model,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);

        const QRect tagIconRect(option.rect.right() - 30, option.rect.top() + 5, 20, 20);
        if (tagIconRect.contains(mouseEvent->pos())) {
            isButtonClicked = true;
            emit tagbutClicked(index);

            const QString filePath = model->data(index, QFileSystemModel::FilePathRole).toString();
            auto *tagDetail = new TagDetail(nullptr, filePath);
            connect(tagDetail, &QObject::destroyed, tagDetail, &QObject::deleteLater);
            tagDetail->show();
            return true;
        }

        const QRect submissionButtonRect(option.rect.right() - 60, option.rect.top() + 5, 20, 20);
        if (submissionButtonRect.contains(mouseEvent->pos())) {
            isButtonClicked = true;
            emit subbutClicked(index);
            return true;
        }

        if (mouseEvent->button() == Qt::RightButton && option.rect.contains(mouseEvent->pos())) {
            showContextMenu(mouseEvent->globalPosition().toPoint(), index, model);
            return true;
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

bool TagItemDelegate::hasTags(const QString &filePath) const
{
    auto it = m_tagsCache.find(filePath);
    if (it != m_tagsCache.end()) {
        return it.value();
    }

    const bool hasTagsValue = dbservice.dbTags().hasTagsForFile(filePath);
    const_cast<TagItemDelegate *>(this)->m_tagsCache[filePath] = hasTagsValue;
    return hasTagsValue;
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
    auto *copyPathAction = new QAction(QStringLiteral("复制路径"), &contextMenu);
    auto *openInExplorerAction = new QAction(QStringLiteral("在文件夹中打开"), &contextMenu);

    connect(openAction, &QAction::triggered, [this, model, index]() { onOpenFileTriggered(model, index); });
    connect(renameAction, &QAction::triggered, [this, model, index]() { onRenameTriggered(model, index); });
    connect(deleteAction, &QAction::triggered, [this, model, index]() { onDeleteFileTriggered(model, index); });
    connect(newTagAction, &QAction::triggered, [this, model, index]() { onNewTagTriggered(model, index); });
    connect(commitAction, &QAction::triggered, [this, model, index]() { onCommitTriggered(model, index); });
    connect(copyPathAction, &QAction::triggered, [this, model, index]() { onCopyPathTriggered(model, index); });
    connect(openInExplorerAction, &QAction::triggered, [this, model, index]() { onOpenInExplorer(model, index); });

    contextMenu.addAction(openAction);
    contextMenu.addAction(renameAction);
    contextMenu.addAction(deleteAction);
    contextMenu.addAction(newTagAction);
    contextMenu.addAction(commitAction);
    contextMenu.addAction(copyPathAction);
    contextMenu.addAction(openInExplorerAction);
    contextMenu.exec(pos);
}

void TagItemDelegate::onNewTagTriggered(QAbstractItemModel *model, const QModelIndex &index)
{
    AddTag tagDialog;
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
    } else {
        QMessageBox::warning(nullptr,
                             QStringLiteral("删除失败"),
                             QStringLiteral("无法删除: %1\n可能没有删除权限").arg(filePath));
    }

    emit deleteFileRequested(filePath);
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
    }
    commitDialog->deleteLater();
}

void TagItemDelegate::addTag(const QAbstractItemModel *model, const QModelIndex &index, AddTag &tagDialog)
{
    if (tagDialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString tagName = tagDialog.getTagName();
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

    emit TagUpdated();
    m_tagsCache[filePath] = true;
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

void TagItemDelegate::onTagDeleted(const QString &filePath)
{
    Q_UNUSED(filePath);
    m_tagsCache.clear();
    emit TagUpdated();
}
