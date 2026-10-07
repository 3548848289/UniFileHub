#include "include/DriveUploadHelper.h"
#include "include/DriveManager.h"
#include "include/DriveItem.h"
#include "include/NameConflictDialog.h"
#include "../Setting/include/SettingManager.h"
#include <QFileInfo>

namespace {

bool cloudNameExists(const QString &fileName)
{
    const QList<DriveItem *> currentFiles = DriveManager::Instance().getCurrentFileList();
    for (DriveItem *item : currentFiles) {
        if (item && item->getName().compare(fileName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

}

bool DriveUploadHelper::uploadToCurrentDirectory(QWidget *dialogParent,
                                                 const QString &filePath,
                                                 QString *errorMessage)
{
    const auto fail = [errorMessage](const QString &message) {
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    };

    QFileInfo fileInfo(filePath);
    if (!fileInfo.isFile()) {
        return fail(QStringLiteral("文件不存在或无法访问"));
    }

    if (SettingManager::Instance().personal_drive_server_ip().trimmed().isEmpty()) {
        return fail(QStringLiteral("网盘服务器地址为空，请先在设置中配置"));
    }

    DriveManager &manager = DriveManager::Instance();
    const QString fileName = fileInfo.fileName();
    const int parentId = manager.getCurrentDirectoryId();

    if (!cloudNameExists(fileName)) {
        manager.uploadFile(filePath, parentId);
        return true;
    }

    switch (SettingManager::Instance().personal_drive_name_conflict_policy()) {
    case SettingManager::PersonalDriveNameConflictPolicy::Overwrite:
        manager.uploadFile(filePath, parentId, fileName, true);
        return true;
    case SettingManager::PersonalDriveNameConflictPolicy::AutoRename:
        manager.uploadFile(filePath, parentId);
        return true;
    case SettingManager::PersonalDriveNameConflictPolicy::Ask:
        break;
    }

    const NameConflictChoice choice = NameConflictDialog::getChoice(
        dialogParent,
        fileName,
        QStringLiteral("当前云盘目录中已存在同名文件，请选择处理方式。"),
        QStringLiteral("覆盖云盘文件"),
        QStringLiteral("自动重命名上传"),
        QStringLiteral("以该命名上传"));

    switch (choice.action) {
    case NameConflictAction::Overwrite:
        manager.uploadFile(filePath, parentId, fileName, true);
        return true;
    case NameConflictAction::AutoRename:
        manager.uploadFile(filePath, parentId);
        return true;
    case NameConflictAction::CustomName:
        if (cloudNameExists(choice.customName)) {
            return fail(QStringLiteral("自定义名称仍与云盘文件重复，请换一个名称"));
        }
        manager.uploadFile(filePath, parentId, choice.customName, false);
        return true;
    case NameConflictAction::Cancel:
        return false;
    }

    return false;
}
