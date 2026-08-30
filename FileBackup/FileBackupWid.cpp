#include "include/FileBackupWid.h"
#include "ui/ui_FileBackupWid.h"
#include <QTimer>
#include "../Setting/include/SettingManager.h"

FileBackupWid::FileBackupWid(const QString &filePath, QWidget *parent) :QDialog(parent),ui(new Ui::FileBackupWid), m_filePath(filePath)
{
    ui->setupUi(this);
    m_messagePopup = new InlineMessagePopup(this);
    m_messagePopup->setPanelWidget(this); // 弹窗显示在面板内部顶部居中

    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
    QString backupFileName = QFileInfo(filePath).baseName() + timeStamp + "." + QFileInfo(filePath).suffix();


    QSettings settings(SettingManager::getSettingsFilePath(), QSettings::IniFormat);
    QString backupDir = settings.value("file_backup/backup_dir", QCoreApplication::applicationDirPath() + "/user").toString();

    ui->edit_path->setText(backupDir);
    ui->edit_name->setText(backupFileName);
}

QString FileBackupWid::getBackupFilePath() const {
    return m_backupFilePath;
}

FileBackupWid::~FileBackupWid() {
    delete ui;
}

bool FileBackupWid::backupFile(const QString &filePath, const QString &fileName) {
    QString backupDir = ui->edit_path->text();
    QDir dir(backupDir);
    if (!dir.exists())
        if (!dir.mkpath(".")) {
            m_messagePopup->showMessage("无法创建备份目录！", true);
            return false;
        }

    QString backupFilePath = backupDir + "/" + ui->edit_name->text();

    if (QFile::copy(filePath, backupFilePath)) {
        m_backupFilePath = backupFilePath;
        return true;
    } else {
        return false;
    }
}

void FileBackupWid::on_save_clicked()
{
    QString fileName = ui->edit_name->text();
    if (backupFile(m_filePath, fileName)) {
        m_messagePopup->showMessage("文件已成功备份！");
        // 延迟关闭，让用户看到成功提示
        QTimer::singleShot(1200, this, &QDialog::accept);
    } else {
        m_messagePopup->showMessage("备份文件失败！", true);
    }
}

void FileBackupWid::on_canle_clicked()
{
    this->close();
}

