#include "include/Setting.h"
#include "ui/ui_Setting.h"
#include <QFileDialog>
#include <QCoreApplication>
#include <QSettings>
#include <QColorDialog>
#include <QUrl>
#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QSpinBox>
#include <QTime>
#include <QTimeEdit>
#include "include/IconManager.h"
#include "include/SettingManager.h"
#if UNIFILEHUB_ENABLE_TERMINAL
#include "../Resources/ThirdParty/KodoTerm/include/KodoTerm/KodoTermConfig.hpp"
#endif

Setting::Setting(QWidget *parent) : QWidget(parent), ui(new Ui::Setting)
    , settings(SettingManager::getSettingsFilePath(), QSettings::IniFormat)
{
    ui->setupUi(this);
    is_modified = false;
    ui->stackedWidget->setCurrentIndex(0);
    ui->treeWidget->setHeaderHidden(true);

    ui->tag_schedule_timeEdit3->setDisplayFormat("HH:mm");
    ui->personal_drive_nameConflictComboBox->addItem(
        tr("默认覆盖"),
        int(SettingManager::PersonalDriveNameConflictPolicy::Overwrite));
    ui->personal_drive_nameConflictComboBox->addItem(
        tr("默认自动重命名"),
        int(SettingManager::PersonalDriveNameConflictPolicy::AutoRename));
    ui->personal_drive_nameConflictComboBox->addItem(
        tr("默认弹出提示框"),
        int(SettingManager::PersonalDriveNameConflictPolicy::Ask));

    // 初始化终端主题列表
#if UNIFILEHUB_ENABLE_TERMINAL
    initTerminalThemes();
#endif

    if (settings.status() == QSettings::NoError) {
        loadSettings();
        is_modified = false;
    } else {
        qDebug() << "Settings file status: " << settings.status();
    }

    setupRealtimeBindings();
}

#if UNIFILEHUB_ENABLE_TERMINAL
void Setting::initTerminalThemes() {
    // 添加默认主题
    ui->terminal_theme_combo->addItem("Default");
    
    // 只加载 Konsole 主题（.colorscheme 文件）
    auto themes = TerminalTheme::builtInThemes();
    for (const auto &info : themes) {
        // 只添加 Konsole 主题
        if (info.format == TerminalTheme::ThemeFormat::Konsole) {
            ui->terminal_theme_combo->addItem(info.name, info.path);
        }
    }
}
#endif


Setting::~Setting() {
    delete ui;
}

bool Setting::commitSettingsAndMaybeRestart()
{
    saveSettings();
    if (!is_modified) {
        return true;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,tr("配置已保存"),
        tr("是否需要重启程序以应用配置？"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QProcess::startDetached(QCoreApplication::applicationFilePath());
        QCoreApplication::exit();
    }

    is_modified = false;
    return true;
}

void Setting::closeEvent(QCloseEvent *event) {
    commitSettingsAndMaybeRestart();
    event->accept();
}


void Setting::loadSettings() {
    ui->all_setting_spinBox->setValue(settings.value("all_setting/font_size", 12).toInt());
    ui->all_setting_comboBox->setCurrentIndex(settings.value("all_setting/theme", 0).toInt());

    ui->all_setting_checkBox->setChecked(settings.value("all_setting/fenableray", true).toBool());
    
#if UNIFILEHUB_ENABLE_TERMINAL
    // 加载终端设置
    ui->terminal_font_combo->setCurrentFont(QFont(settings.value("terminal/font_family", "Consolas").toString()));
    ui->terminal_font_size_spin->setValue(settings.value("terminal/font_size", 14).toInt());
    QString themeName = settings.value("terminal/theme", "Default").toString();
    int themeIndex = ui->terminal_theme_combo->findText(themeName);
    if (themeIndex >= 0) {
        ui->terminal_theme_combo->setCurrentIndex(themeIndex);
    }
    
    // 加载终端类型设置
    QString terminalType = settings.value("terminal/type", "powershell").toString();
    ui->terminal_checkbox_powershell->setChecked(terminalType == "powershell");
    ui->terminal_checkbox_cmd->setChecked(terminalType == "cmd");
#endif
    
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
    ui->file_backup_lineEdit2->setText(settings.value("file_backup/IP", "127.0.0.1").toString());



    QTime showtime = QTime(0, 0).addSecs(settings.value("tag_schedule/show_time", 60).toInt());
    ui->tag_schedule_timeEdit3->setTime(showtime);

    ui->email_service_lineEdit_1->setText(settings.value("EmailConfig/host").toString());
    ui->email_service_lineEdit_2->setText(settings.value("EmailConfig/username").toString());
    ui->email_service_lineEdit_3->setText(settings.value("EmailConfig/password").toString());
    ui->email_service_lineEdit_4->setText(settings.value("EmailConfig/port").toString());
    ui->email_service_lineEdit_5->setText(settings.value("EmailConfig/sender").toString());
    ui->email_service_lineEdit_6->setText(settings.value("EmailConfig/received").toString());

    ui->clip_board_spinBox->setValue(settings.value("clip_board/hours", 24).toInt());
    ui->clip_board_doubleClickMinimizeCheckBox->setChecked(
        settings.value("clip_board/double_click_copy_minimize", true).toBool());
    ui->clip_board_ctrlCMinimizeCheckBox->setChecked(
        settings.value("clip_board/ctrl_c_copy_minimize", true).toBool());
    ui->clip_board_contextMenuMinimizeCheckBox->setChecked(
        settings.value("clip_board/context_menu_copy_minimize", true).toBool());

    ui->server_config_lineEdit1->setText(settings.value("ServerConfig/IP1", "http://43.139.86.56:5002/").toString());
    ui->server_config_lineEdit2->setText(settings.value("ServerConfig/IP2", "http://43.139.86.56:5000/").toString());
    ui->server_config_lineEdit3->setText(settings.value("ServerConfig/IP3", "http://43.139.86.56:5003/").toString());
    ui->server_config_lineEdit4->setText(settings.value("ServerConfig/IP4", "http://43.139.86.56:5001/").toString());
    ui->server_config_lineEdit5->setText(settings.value("PersonalDrive/ServerIP", "http://127.0.0.1:5005/").toString());
    ui->server_config_lineEdit6->setText(settings.value("ClipboardSync/ServerIP", "http://127.0.0.1:5006/").toString());
    ui->personal_drive_lineEdit->setText(settings.value("PersonalDrive/DefaultDir").toString());
    const int nameConflictPolicy = settings.value(
        "PersonalDrive/NameConflictPolicy",
        int(SettingManager::PersonalDriveNameConflictPolicy::Ask)).toInt();
    int nameConflictIndex = ui->personal_drive_nameConflictComboBox->findData(nameConflictPolicy);
    if (nameConflictIndex < 0) {
        nameConflictIndex = ui->personal_drive_nameConflictComboBox->findData(
            int(SettingManager::PersonalDriveNameConflictPolicy::Ask));
    }
    ui->personal_drive_nameConflictComboBox->setCurrentIndex(nameConflictIndex);
    
    // 设置标签计划的默认时间值
    // 通知显示时间默认1分钟
    int showTimeInSeconds = settings.value("tag_schedule/show_time", 60).toInt();
    QTime showTime(showTimeInSeconds / 3600, (showTimeInSeconds % 3600) / 60, showTimeInSeconds % 60);
    ui->tag_schedule_timeEdit3->setTime(showTime);
}

void Setting::saveSettings() {
    auto setRestartValue = [this](const QString &key, const QVariant &value) {
        if (settings.value(key) != value) {
            settings.setValue(key, value);
            is_modified = true;
        }
    };

    setRestartValue("all_setting/font_size", ui->all_setting_spinBox->value());
    setRestartValue("all_setting/theme", ui->all_setting_comboBox->currentIndex());

    setRestartValue("all_setting/fenableray", ui->all_setting_checkBox->isChecked());
    
#if UNIFILEHUB_ENABLE_TERMINAL
    // 保存终端设置
    setRealtimeValue("terminal/font_family", ui->terminal_font_combo->currentFont().family());
    setRealtimeValue("terminal/font_size", ui->terminal_font_size_spin->value());
    setRealtimeValue("terminal/theme", ui->terminal_theme_combo->currentText());
    
    // 保存终端类型设置
    QString terminalType = ui->terminal_checkbox_powershell->isChecked() ? "powershell" : "cmd";
    setRealtimeValue("terminal/type", terminalType);
#endif
    
    // 保存图标颜色设置
    setRestartValue("all_setting/icon_color", ui->all_setting_iconColorBtn->styleSheet().section("background-color: ", 1, 1).section("; color", 0, 0));
    setRestartValue("all_setting/secondary_icon_color", ui->all_setting_secondaryIconColorBtn->styleSheet().section("background-color: ", 1, 1).section("; color", 0, 0));

    QString filesystemDir = ui->file_system_lineEdit->text();
    if (filesystemDir.isEmpty())
        filesystemDir = settings.value("file_system/file_system_dir", QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).toString();
    setRealtimeValue("file_system/file_system_dir", filesystemDir);

    setRealtimeValue("file_see/font_size", ui->file_see_spinBox->value());
    setRealtimeValue("file_see/txt", ui->file_see_checkBox1->isChecked());
    setRealtimeValue("file_see/csv", ui->file_see_checkBox2->isChecked());
    setRealtimeValue("file_see/xlsx", ui->file_see_checkBox3->isChecked());
    setRealtimeValue("file_see/img", ui->file_see_checkBox4->isChecked());

    QString backupDir = ui->file_backup_lineEdit1->text();
    if (backupDir.isEmpty())
        backupDir = settings.value("file_backup/backup_dir", QCoreApplication::applicationDirPath() + "/user").toString();
    setRealtimeValue("file_backup/backup_dir", backupDir);
    setRealtimeValue("file_backup/IP", ui->file_backup_lineEdit2->text().trimmed());




    QTime showTime = ui->tag_schedule_timeEdit3->time();
    int showTimeInSeconds = showTime.hour() * 3600 + showTime.minute() * 60 + showTime.second();
    setRealtimeValue("tag_schedule/show_time", showTimeInSeconds);

    setRealtimeValue("EmailConfig/host", ui->email_service_lineEdit_1->text().trimmed());
    setRealtimeValue("EmailConfig/username", ui->email_service_lineEdit_2->text().trimmed());
    setRealtimeValue("EmailConfig/password", ui->email_service_lineEdit_3->text().trimmed());
    setRealtimeValue("EmailConfig/port", ui->email_service_lineEdit_4->text().trimmed());
    setRealtimeValue("EmailConfig/sender", ui->email_service_lineEdit_5->text().trimmed());
    setRealtimeValue("EmailConfig/received", ui->email_service_lineEdit_6->text().trimmed());

    setRealtimeValue("clip_board/hours", ui->clip_board_spinBox->value());
    setRealtimeValue("clip_board/double_click_copy_minimize",
                     ui->clip_board_doubleClickMinimizeCheckBox->isChecked());
    setRealtimeValue("clip_board/ctrl_c_copy_minimize",
                     ui->clip_board_ctrlCMinimizeCheckBox->isChecked());
    setRealtimeValue("clip_board/context_menu_copy_minimize",
                     ui->clip_board_contextMenuMinimizeCheckBox->isChecked());

    setRealtimeValue("ServerConfig/IP1", ui->server_config_lineEdit1->text().trimmed());
    setRealtimeValue("ServerConfig/IP2", ui->server_config_lineEdit2->text().trimmed());
    setRealtimeValue("ServerConfig/IP3", ui->server_config_lineEdit3->text().trimmed());
    setRealtimeValue("ServerConfig/IP4", ui->server_config_lineEdit4->text().trimmed());
    setRealtimeValue("PersonalDrive/ServerIP", ui->server_config_lineEdit5->text().trimmed());
    setRealtimeValue("ClipboardSync/ServerIP", ui->server_config_lineEdit6->text().trimmed());
    
    QString personalDriveDir = ui->personal_drive_lineEdit->text();
    if (personalDriveDir.isEmpty())
        personalDriveDir = settings.value("PersonalDrive/DefaultDir", QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).toString();
    setRealtimeValue("PersonalDrive/DefaultDir", personalDriveDir);
    setRealtimeValue("PersonalDrive/NameConflictPolicy",
                     ui->personal_drive_nameConflictComboBox->currentData().toInt());
    
    settings.sync();
}

void Setting::setRealtimeValue(const QString &key, const QVariant &value)
{
    SettingManager::Instance().setValue(key, value);
    settings.setValue(key, value);
    settings.sync();
}

void Setting::setupRealtimeBindings()
{
    auto bindLineEdit = [this](QLineEdit *lineEdit, const QString &key) {
        connect(lineEdit, &QLineEdit::editingFinished, this, [this, lineEdit, key]() {
            setRealtimeValue(key, lineEdit->text().trimmed());
        });
    };

    bindLineEdit(ui->file_system_lineEdit, "file_system/file_system_dir");
    bindLineEdit(ui->file_backup_lineEdit1, "file_backup/backup_dir");
    bindLineEdit(ui->file_backup_lineEdit2, "file_backup/IP");
    bindLineEdit(ui->email_service_lineEdit_1, "EmailConfig/host");
    bindLineEdit(ui->email_service_lineEdit_2, "EmailConfig/username");
    bindLineEdit(ui->email_service_lineEdit_3, "EmailConfig/password");
    bindLineEdit(ui->email_service_lineEdit_4, "EmailConfig/port");
    bindLineEdit(ui->email_service_lineEdit_5, "EmailConfig/sender");
    bindLineEdit(ui->email_service_lineEdit_6, "EmailConfig/received");
    bindLineEdit(ui->server_config_lineEdit1, "ServerConfig/IP1");
    bindLineEdit(ui->server_config_lineEdit2, "ServerConfig/IP2");
    bindLineEdit(ui->server_config_lineEdit3, "ServerConfig/IP3");
    bindLineEdit(ui->server_config_lineEdit4, "ServerConfig/IP4");
    bindLineEdit(ui->server_config_lineEdit5, "PersonalDrive/ServerIP");
    bindLineEdit(ui->server_config_lineEdit6, "ClipboardSync/ServerIP");
    bindLineEdit(ui->personal_drive_lineEdit, "PersonalDrive/DefaultDir");
    connect(ui->personal_drive_nameConflictComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this]() {
                setRealtimeValue("PersonalDrive/NameConflictPolicy",
                                 ui->personal_drive_nameConflictComboBox->currentData().toInt());
            });

    connect(ui->file_see_spinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        setRealtimeValue("file_see/font_size", value);
    });
    connect(ui->file_see_checkBox1, &QCheckBox::toggled, this, [this](bool checked) {
        setRealtimeValue("file_see/txt", checked);
    });
    connect(ui->file_see_checkBox2, &QCheckBox::toggled, this, [this](bool checked) {
        setRealtimeValue("file_see/csv", checked);
    });
    connect(ui->file_see_checkBox3, &QCheckBox::toggled, this, [this](bool checked) {
        setRealtimeValue("file_see/xlsx", checked);
    });
    connect(ui->file_see_checkBox4, &QCheckBox::toggled, this, [this](bool checked) {
        setRealtimeValue("file_see/img", checked);
    });

    connect(ui->tag_schedule_timeEdit3, &QTimeEdit::timeChanged, this, [this](const QTime &time) {
        setRealtimeValue("tag_schedule/show_time", time.hour() * 3600 + time.minute() * 60 + time.second());
    });
    connect(ui->clip_board_spinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        setRealtimeValue("clip_board/hours", value);
    });
    connect(ui->clip_board_doubleClickMinimizeCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        setRealtimeValue("clip_board/double_click_copy_minimize", checked);
    });
    connect(ui->clip_board_ctrlCMinimizeCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        setRealtimeValue("clip_board/ctrl_c_copy_minimize", checked);
    });
    connect(ui->clip_board_contextMenuMinimizeCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        setRealtimeValue("clip_board/context_menu_copy_minimize", checked);
    });

#if UNIFILEHUB_ENABLE_TERMINAL
    connect(ui->terminal_font_combo, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        setRealtimeValue("terminal/font_family", font.family());
    });
    connect(ui->terminal_font_size_spin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        setRealtimeValue("terminal/font_size", value);
    });
    connect(ui->terminal_theme_combo, &QComboBox::currentTextChanged, this, [this](const QString &theme) {
        setRealtimeValue("terminal/theme", theme);
    });

#endif
    connect(ui->all_setting_spinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        is_modified = true;
    });
    connect(ui->all_setting_checkBox, &QCheckBox::toggled, this, [this](bool) {
        is_modified = true;
    });
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
        setRealtimeValue("file_system/file_system_dir", selectedDir);
    }
}


void Setting::on_file_backup_Btn_clicked()
{
    QString selectedDir = QFileDialog::getExistingDirectory(this, "Select Backup Directory", ui->file_backup_lineEdit1->text());
    if (!selectedDir.isEmpty()) {
        selectedDir.replace("\\", "/");
        ui->file_backup_lineEdit1->setText(selectedDir);
        setRealtimeValue("file_backup/backup_dir", selectedDir);
    }
}


void Setting::on_all_setting_comboBox_currentIndexChanged(int index)
{
    settings.setValue("all_setting/theme", index);
    is_modified = true;
}

void Setting::on_all_setting_iconColorBtn_clicked() {
    QColor color = QColorDialog::getColor(Qt::blue, this, "閫夋嫨鍥炬爣棰滆壊");
    if (color.isValid()) {
        QString colorStr = color.name();
        ui->all_setting_iconColorBtn->setStyleSheet(QString("background-color: %1; color: %2;").arg(colorStr).arg(color.lightness() < 128 ? "white" : "black"));
        settings.setValue("all_setting/icon_color", colorStr);
        is_modified = true;
    }
}

void Setting::on_all_setting_secondaryIconColorBtn_clicked() {
    QColor color = QColorDialog::getColor(Qt::blue, this, "閫夋嫨杈呭姪鍥炬爣棰滆壊");
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
        setRealtimeValue("PersonalDrive/DefaultDir", selectedDir);
    }
}

void Setting::on_server_config_replaceBtn_clicked()
{
    const QString inputIp = ui->server_config_replaceLineEdit->text().trimmed();
    if (inputIp.isEmpty()) {
        return;
    }

    auto replaceHost = [&inputIp](QLineEdit *lineEdit) {
        QUrl url(lineEdit->text().trimmed());
        if (url.isValid() && !url.scheme().isEmpty()) {
            url.setHost(inputIp);
            lineEdit->setText(url.toString());
            return;
        }

        QString text = lineEdit->text().trimmed();
        int schemePos = text.indexOf("://");
        if (schemePos >= 0) {
            int hostStart = schemePos + 3;
            int hostEnd = text.indexOf(':', hostStart);
            if (hostEnd < 0) {
                hostEnd = text.indexOf('/', hostStart);
            }
            if (hostEnd < 0) {
                hostEnd = text.size();
            }
            text.replace(hostStart, hostEnd - hostStart, inputIp);
            lineEdit->setText(text);
            return;
        }

        int hostEnd = text.indexOf(':');
        if (hostEnd < 0) {
            hostEnd = text.indexOf('/');
        }
        if (hostEnd < 0) {
            lineEdit->setText(inputIp);
            return;
        }
        text.replace(0, hostEnd, inputIp);
        lineEdit->setText(text);
    };

    replaceHost(ui->server_config_lineEdit1);
    replaceHost(ui->server_config_lineEdit2);
    replaceHost(ui->server_config_lineEdit3);
    replaceHost(ui->server_config_lineEdit4);
    replaceHost(ui->server_config_lineEdit5);
    replaceHost(ui->server_config_lineEdit6);

    setRealtimeValue("ServerConfig/IP1", ui->server_config_lineEdit1->text().trimmed());
    setRealtimeValue("ServerConfig/IP2", ui->server_config_lineEdit2->text().trimmed());
    setRealtimeValue("ServerConfig/IP3", ui->server_config_lineEdit3->text().trimmed());
    setRealtimeValue("ServerConfig/IP4", ui->server_config_lineEdit4->text().trimmed());
    setRealtimeValue("PersonalDrive/ServerIP", ui->server_config_lineEdit5->text().trimmed());
    setRealtimeValue("ClipboardSync/ServerIP", ui->server_config_lineEdit6->text().trimmed());
}

void Setting::on_terminal_checkbox_powershell_stateChanged(int state) {
    if (state == Qt::Checked) {
        ui->terminal_checkbox_cmd->setChecked(false);
        setRealtimeValue("terminal/type", "powershell");
    }
}

void Setting::on_terminal_checkbox_cmd_stateChanged(int state) {
    if (state == Qt::Checked) {
        ui->terminal_checkbox_powershell->setChecked(false);
        setRealtimeValue("terminal/type", "cmd");
    }
}

