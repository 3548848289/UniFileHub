#include "include/Setting.h"
#include "ui/ui_Setting.h"
#include <QFileDialog>
#include <QCoreApplication>
#include <QSettings>
#include <QColorDialog>
#include "include/IconManager.h"

Setting::Setting(QWidget *parent) : QWidget(parent), ui(new Ui::Setting)
    , settings("settings.ini", QSettings::IniFormat)
{
    ui->setupUi(this);
    is_modified = false;
    loadSettings();
    ui->stackedWidget->setCurrentIndex(0);
    ui->treeWidget->setHeaderHidden(true);

    ui->tag_schedule_timeEdit3->setDisplayFormat("HH:mm");

    if (settings.status() == QSettings::NoError) {
        loadSettings();
    } else {
        qDebug() << "Settings file status: " << settings.status();
    }
    loadSettings();
}


Setting::~Setting() {
    delete ui;
}

void Setting::closeEvent(QCloseEvent *event) {
    saveSettings();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,tr("配置已保存"),
        tr("是否需要重启程序以应用配置？"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QProcess::startDetached(QCoreApplication::applicationFilePath());
        QCoreApplication::exit();
    }

    event->accept();
}


void Setting::loadSettings() {
    ui->all_setting_spinBox->setValue(settings.value("all_setting/font_size", 12).toInt());
    ui->all_setting_comboBox->setCurrentIndex(settings.value("all_setting/theme", 0).toInt());

    ui->all_setting_checkBox->setChecked(settings.value("all_setting/fenableray", true).toBool());
    
    // 加载图标颜色设置
    QString iconColor = settings.value("all_setting/icon_color", "#7598db").toString();
    QString secondaryIconColor = settings.value("all_setting/secondary_icon_color", "#7598db").toString();
    
    // 设置按钮样式以显示当前颜色
    QColor color1(iconColor);
    QColor color2(secondaryIconColor);
    ui->all_setting_iconColorBtn->setStyleSheet(QString("background-color: %1; color: %2;").arg(iconColor).arg(color1.lightness() < 128 ? "white" : "black"));
    ui->all_setting_secondaryIconColorBtn->setStyleSheet(QString("background-color: %1; color: %2;").arg(secondaryIconColor).arg(color2.lightness() < 128 ? "white" : "black"));
    ui->file_system_lineEdit->setText(settings.value("file_system/file_system_dir").toString());
    ui->file_see_spinBox->setValue(settings.value("file_see/font_size", 12).toInt());
    ui->file_see_checkBox1->setChecked(settings.value("file_see/txt", true).toBool());
    ui->file_see_checkBox2->setChecked(settings.value("file_see/csv", true).toBool());
    ui->file_see_checkBox3->setChecked(settings.value("file_see/xlsx", true).toBool());
    ui->file_see_checkBox4->setChecked(settings.value("file_see/img", true).toBool());


    ui->file_backup_lineEdit1->setText(settings.value("file_backup/backup_dir").toString());

    int reminderIndex = ui->tag_schedule_comboBox->findText(
        settings.value("tag_schedule/reminder_type", "Popup").toString());
    if (reminderIndex != -1)
        ui->tag_schedule_comboBox->setCurrentIndex(reminderIndex);

    QTime showtime = QTime(0, 0).addSecs(settings.value("tag_schedule/show_time", 0).toInt());
    ui->tag_schedule_timeEdit3->setTime(showtime);

    ui->email_service_lineEdit_1->setText(settings.value("EmailConfig/host").toString());
    ui->email_service_lineEdit_2->setText(settings.value("EmailConfig/username").toString());
    ui->email_service_lineEdit_3->setText(settings.value("EmailConfig/password").toString());
    ui->email_service_lineEdit_4->setText(settings.value("EmailConfig/port").toString());
    ui->email_service_lineEdit_5->setText(settings.value("EmailConfig/sender").toString());
    ui->email_service_lineEdit_6->setText(settings.value("EmailConfig/received").toString());

    ui->clip_board_spinBox->setValue(settings.value("clip_board/hours", 24).toInt());

    ui->server_config_lineEdit1->setText(settings.value("ServerConfig/IP1", "http://43.139.86.56:5002/").toString());
    ui->server_config_lineEdit2->setText(settings.value("ServerConfig/IP2", "http://43.139.86.56:5000/").toString());
    ui->server_config_lineEdit3->setText(settings.value("ServerConfig/IP3", "http://43.139.86.56:5003/").toString());
    ui->server_config_lineEdit4->setText(settings.value("ServerConfig/IP4", "http://43.139.86.56:5001/").toString());
    ui->server_config_lineEdit5->setText(settings.value("PersonalDrive/ServerIP", "http://127.0.0.1:5005/").toString());
    ui->personal_drive_lineEdit->setText(settings.value("PersonalDrive/DefaultDir").toString());
    
    // 设置标签计划的默认时间值
    // 通知显示时间默认5秒
    int showTimeInSeconds = settings.value("tag_schedule/show_time", 5).toInt();
    QTime showTime(showTimeInSeconds / 3600, (showTimeInSeconds % 3600) / 60, showTimeInSeconds % 60);
    ui->tag_schedule_timeEdit3->setTime(showTime);
}

void Setting::saveSettings() {
    settings.setValue("all_setting/font_size", ui->all_setting_spinBox->value());
    settings.setValue("all_setting/theme", ui->all_setting_comboBox->currentIndex());

    settings.setValue("all_setting/fenableray", ui->all_setting_checkBox->isChecked());
    
    // 保存图标颜色设置
    settings.setValue("all_setting/icon_color", ui->all_setting_iconColorBtn->styleSheet().section("background-color: ", 1, 1).section("; color", 0, 0));
    settings.setValue("all_setting/secondary_icon_color", ui->all_setting_secondaryIconColorBtn->styleSheet().section("background-color: ", 1, 1).section("; color", 0, 0));

    QString filesystemDir = ui->file_system_lineEdit->text();
    if (filesystemDir.isEmpty())
        filesystemDir = settings.value("file_system/file_system_dir", QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).toString();
    else
        settings.setValue("file_system/file_system_dir", filesystemDir);

    settings.setValue("file_see/font_size", ui->file_see_spinBox->value());
    settings.setValue("file_see/txt", ui->file_see_checkBox1->isChecked());
    settings.setValue("file_see/csv", ui->file_see_checkBox2->isChecked());
    settings.setValue("file_see/xlsx", ui->file_see_checkBox3->isChecked());
    settings.setValue("file_see/img", ui->file_see_checkBox4->isChecked());

    QString backupDir = ui->file_backup_lineEdit1->text();
    if (backupDir.isEmpty())
        backupDir = settings.value("file_backup/backup_dir", QCoreApplication::applicationDirPath() + "/user").toString();
    else
        settings.setValue("file_backup/backup_dir", backupDir);


    settings.setValue("tag_schedule/reminder_type", ui->tag_schedule_comboBox->currentText());

    QTime showTime = ui->tag_schedule_timeEdit3->time();
    int showTimeInSeconds = showTime.hour() * 3600 + showTime.minute() * 60 + showTime.second();
    settings.setValue("tag_schedule/show_time", showTimeInSeconds);

    settings.setValue("EmailConfig/host", ui->email_service_lineEdit_1->text());
    settings.setValue("EmailConfig/username", ui->email_service_lineEdit_2->text());
    settings.setValue("EmailConfig/password", ui->email_service_lineEdit_3->text());
    settings.setValue("EmailConfig/port", ui->email_service_lineEdit_4->text());
    settings.setValue("EmailConfig/sender", ui->email_service_lineEdit_5->text());
    settings.setValue("EmailConfig/received", ui->email_service_lineEdit_6->text());

    settings.setValue("clip_board/hours", ui->clip_board_spinBox->text());

    settings.setValue("ServerConfig/IP1", ui->server_config_lineEdit1->text());
    settings.setValue("ServerConfig/IP2", ui->server_config_lineEdit2->text());
    settings.setValue("ServerConfig/IP3", ui->server_config_lineEdit3->text());
    settings.setValue("ServerConfig/IP4", ui->server_config_lineEdit4->text());
    settings.setValue("PersonalDrive/ServerIP", ui->server_config_lineEdit5->text());
    
    QString personalDriveDir = ui->personal_drive_lineEdit->text();
    if (personalDriveDir.isEmpty())
        personalDriveDir = settings.value("PersonalDrive/DefaultDir", QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).toString();
    else
        settings.setValue("PersonalDrive/DefaultDir", personalDriveDir);
    
    settings.sync();
}


void Setting::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    QTreeWidgetItem *rootItem = item;
    while (rootItem->parent()) {
        rootItem = rootItem->parent();
    }
    int index = rootItem->treeWidget()->indexOfTopLevelItem(rootItem);
    ui->stackedWidget->setCurrentIndex(index);
}





void Setting::on_file_system_Btn_clicked()
{
    QString selectedDir = QFileDialog::getExistingDirectory(this, "Select Filesystem Directory", ui->file_system_lineEdit->text());
    if (!selectedDir.isEmpty()) {
        selectedDir.replace("\\", "/");
        ui->file_system_lineEdit->setText(selectedDir);
    }
}


void Setting::on_file_backup_Btn_clicked()
{
    QString selectedDir = QFileDialog::getExistingDirectory(this, "Select Backup Directory", ui->file_backup_lineEdit1->text());
    if (!selectedDir.isEmpty()) {
        selectedDir.replace("\\", "/");
        ui->file_backup_lineEdit1->setText(selectedDir);
    }
}


void Setting::on_all_setting_comboBox_currentIndexChanged(int index)
{
    settings.setValue("all_setting/theme", index);
    is_modified = true;
}

void Setting::on_all_setting_iconColorBtn_clicked() {
    QColor color = QColorDialog::getColor(Qt::blue, this, "选择图标颜色");
    if (color.isValid()) {
        QString colorStr = color.name();
        ui->all_setting_iconColorBtn->setStyleSheet(QString("background-color: %1; color: %2;").arg(colorStr).arg(color.lightness() < 128 ? "white" : "black"));
        settings.setValue("all_setting/icon_color", colorStr);
        is_modified = true;
    }
}

void Setting::on_all_setting_secondaryIconColorBtn_clicked() {
    QColor color = QColorDialog::getColor(Qt::blue, this, "选择辅助图标颜色");
    if (color.isValid()) {
        QString colorStr = color.name();
        ui->all_setting_secondaryIconColorBtn->setStyleSheet(QString("background-color: %1; color: %2;").arg(colorStr).arg(color.lightness() < 128 ? "white" : "black"));
        settings.setValue("all_setting/secondary_icon_color", colorStr);
        is_modified = true;
    }
}


void Setting::on_personal_drive_Btn_clicked()
{
    QString selectedDir = QFileDialog::getExistingDirectory(this, "Select Personal Drive Directory", ui->personal_drive_lineEdit->text());
    if (!selectedDir.isEmpty()) {
        selectedDir.replace("\\", "/");
        ui->personal_drive_lineEdit->setText(selectedDir);
    }
}

