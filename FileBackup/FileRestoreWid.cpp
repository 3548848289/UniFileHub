#include "include/FileRestoreWid.h"
#include "ui/ui_FileRestoreWid.h"

FileRestoreWid::FileRestoreWid(QString fileName, bool isMissing, QWidget *parent) :QDialog(parent),
    ui(new Ui::FileRestoreWid), file_name(fileName), isMissing(isMissing),
    dbservice(dbService::instance("./SmartDesk.db")) {
    ui->setupUi(this);
    m_messagePopup = new InlineMessagePopup(this);
    m_messagePopup->setPanelWidget(this); // 弹窗显示在面板内部顶部居中
    QFileInfo tmp(fileName);
    backup_filepath = tmp.path();

    submit_time = dbservice.dbBackup().getSubTime(fileName);
    initial_file_name = dbservice.dbBackup().getInitPath(fileName);

    ui->dateTimeEdit->setDateTime(submit_time);
    ui->filenameEdit->setText(file_name);
    ui->initpathEdit->setText(initial_file_name);
    QPixmap pixmap(":/usedimage/tips.svg");

    QSize labelSize = ui->label_4->size();
    QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    ui->label_4->setPixmap(scaledPixmap);
    if(isMissing) {
        ui->label_4->show();
        ui->label_4->setToolTip("文件已经缺失，可在上一级窗口右键文件更改路径");
    }
    else
        ui->label_4->hide();
}

FileRestoreWid::~FileRestoreWid()
{
    delete ui;
}

bool FileRestoreWid::copyFileSafely(const QString &source, const QString &destination, QString *errorMsg)
{
    QFileInfo sourceInfo(source);
    if (!sourceInfo.exists()) {
        if (errorMsg) *errorMsg = "源文件不存在!";
        return false;
    }

    QFileInfo destInfo(destination);
    QDir destDir = destInfo.absoluteDir();

    if (!destDir.exists()) {
        if (!destDir.mkpath(".")) {
            if (errorMsg) *errorMsg = "无法创建目标目录!";
            return false;
        }
    }

    if (destInfo.exists() && !QFile::remove(destination)) {
        if (errorMsg) *errorMsg = "无法删除已有目标文件!";
        return false;
    }

    if (!QFile::copy(source, destination)) {
        if (errorMsg) *errorMsg = "文件复制失败!";
        return false;
    }

    return true;
}

void FileRestoreWid::on_restoreBtn_clicked()
{
    QString error;
    QString targetFilePath = ui->initpathEdit->text();

    // 还原会覆盖原位置的现文件，覆盖前必须确认
    if (QFile::exists(targetFilePath)) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, "还原文件",
            QString("原位置已存在同名文件，还原将覆盖它，且无法恢复。确定继续吗？\n%1").arg(targetFilePath));
        if (answer != QMessageBox::Yes)
            return;
    }

    if (!copyFileSafely(file_name, targetFilePath, &error)) {
        m_messagePopup->showMessage(error, true);
        return;
    }

    m_messagePopup->showMessage("文件恢复成功!");
}
