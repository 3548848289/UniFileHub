#include "include/ExternalActionDispatcher.h"
#include "include/mainwindow.h"

#include <QDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>

#include "../../FileBackup/include/FileBackupWid.h"
#include "../../TagSchedule/include/AddTag.h"
#include "../../PersonalDrive/include/DriveUploadHelper.h"
#include "../../manager/include/dbService.h"
#include "../../manager/include/DataChangeNotifier.h"

ExternalActionDispatcher::ExternalActionDispatcher(MainWindow *window, QObject *parent)
    : QObject(parent),
      m_window(window)
{
}

void ExternalActionDispatcher::dispatch(const QString &action, const QString &filePath) const
{
    const QString normalized = action.trimmed().toLower();

    if (normalized == QStringLiteral("open")) {
        m_window->openFileFromCommandLine(filePath);
    } else if (normalized == QStringLiteral("backup")) {
        backupFile(filePath);
    } else if (normalized == QStringLiteral("tag")) {
        tagFile(filePath);
    } else if (normalized == QStringLiteral("upload")) {
        uploadFile(filePath);
    } else if (normalized == QStringLiteral("openfolder")) {
        openFolder(filePath);
    } else {
        qDebug() << "Unknown external action:" << action << filePath;
    }
}

void ExternalActionDispatcher::backupFile(const QString &filePath) const
{
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::information(m_window, tr("提示"), tr("文件不存在，无法备份。"));
        return;
    }

    // 与文件系统右键"文件备份"(TagItemDelegate::onCommitTriggered)保持一致：
    // 弹备份对话框，确认后记录备份提交
    FileBackupWid commitDialog(filePath, m_window);
    if (commitDialog.exec() == QDialog::Accepted) {
        const QString backupFilePath = commitDialog.getBackupFilePath();
        dbService::instance("./SmartDesk.db").dbBackup().recordSubmission(filePath, backupFilePath);
        DataChangeNotifier::Instance().notifyBackupRecordAdded(filePath);
    }
}

void ExternalActionDispatcher::tagFile(const QString &filePath) const
{
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::information(m_window, tr("提示"), tr("文件不存在，无法添加标签。"));
        return;
    }

    AddTag tagDialog(m_window);
    tagDialog.setTargetFile(filePath);
    if (tagDialog.exec() != QDialog::Accepted) {
        return;
    }

    // 与文件系统右键"新建标签"(TagItemDelegate::addTag)保持一致的入库逻辑
    const QString tagName = tagDialog.getTagName();
    if (tagName.isEmpty()) {
        return;
    }
    const QString annotation = tagDialog.getAnnotation();
    const QDateTime expirationDate = tagDialog.getExpirationDate();
    const int reminderHours = tagDialog.getReminderTime();
    const int intervalMinutes = tagDialog.getIntervalTime();
    const QString reminderType = tagDialog.getReminderType();

    dbService &db = dbService::instance("./SmartDesk.db");

    FilePathInfo tagInfo;
    tagInfo.filePath = filePath;
    tagInfo.tagName = tagName;
    tagInfo.expirationDate = expirationDate;
    tagInfo.annotation = annotation;
    tagInfo.reminderTime = reminderHours;
    tagInfo.intervalTime = intervalMinutes;
    tagInfo.lastReminderIndex = -1;
    tagInfo.reminderType = reminderType;

    int fileId = -1;
    if (!db.dbTags().getFileId(filePath, fileId)) {
        db.dbTags().addFilePath(filePath, fileId);
        db.dbTags().saveTags(fileId, tagName);
        db.dbTags().saveAnnotation(fileId, annotation);
        db.dbTags().updateFileInfo(tagInfo);
    } else {
        db.dbTags().updateFileInfo(tagInfo);
    }

    DataChangeNotifier::Instance().notifyTagChanged(filePath);
}

void ExternalActionDispatcher::uploadFile(const QString &filePath) const
{
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::information(m_window, tr("提示"), tr("请选择一个本地文件上传到网盘。"));
        return;
    }

    QString uploadError;
    if (!DriveUploadHelper::uploadToCurrentDirectory(m_window, filePath, &uploadError)
        && !uploadError.isEmpty()) {
        QMessageBox::information(m_window, tr("提示"), uploadError);
    }
}

void ExternalActionDispatcher::openFolder(const QString &folderPath) const
{
    // 等价于应用内把文件系统面包屑定位到该目录；校验与提示在
    // MainWindow::openFolderInFileSystem 内完成
    m_window->openFolderInFileSystem(folderPath);
}
