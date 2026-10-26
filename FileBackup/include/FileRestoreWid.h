#ifndef FILERESTOREWID_H
#define FILERESTOREWID_H

#include <QDialog>
#include <QString>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include "../../manager/include/dbService.h"
#include "../../manager/include/InlineMessagePopup.h"

namespace Ui {
class FileRestoreWid;
}

class FileRestoreWid : public QDialog
{
    Q_OBJECT

public:
    explicit FileRestoreWid(QString fileName, bool isMissing, QWidget *parent = nullptr);
    ~FileRestoreWid();

    // 安全复制：覆盖前先处理目标冲突，失败时给出可提示的错误信息
    static bool copyFileSafely(const QString &source, const QString &destination, QString *errorMsg);

private slots:
    void on_restoreBtn_clicked();

private:
    Ui::FileRestoreWid *ui;
    QString file_name;          //备份文件全文件路径
    QString backup_filepath;    //备份文件路径
    QDateTime submit_time;
    QString initial_file_name;  //原文件全文件路径
    bool isMissing;
    dbService& dbservice;
    InlineMessagePopup *m_messagePopup = nullptr;
};

#endif // FILERESTOREWID_H



// void FileRestoreWid::on_pathBtn_clicked()
// {
//     QString dir = QFileDialog::getExistingDirectory(this, tr("选择保存目录"),
//          QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
//     if (!dir.isEmpty()) {
//         ui->backupPathRdit->setText(dir);
//     }
// }
