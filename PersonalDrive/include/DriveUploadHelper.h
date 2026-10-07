#ifndef DRIVEUPLOADHELPER_H
#define DRIVEUPLOADHELPER_H

#include <QString>

class QWidget;

// 供文件备份、备忘标签等模块把本地文件上传到网盘的辅助入口。
// 上传目标为网盘当前所在目录（DriveManager 的当前目录），重名冲突沿用设置中的处理策略。
class DriveUploadHelper
{
public:
    // 返回是否已发起上传（不代表上传完成）。用户取消或失败时返回 false，
    // 失败原因写入 errorMessage（可能为空）。
    static bool uploadToCurrentDirectory(QWidget *dialogParent,
                                         const QString &filePath,
                                         QString *errorMessage = nullptr);
};

#endif // DRIVEUPLOADHELPER_H
