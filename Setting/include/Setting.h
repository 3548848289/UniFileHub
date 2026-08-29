#ifndef SETTING_H
#define SETTING_H

#include <QWidget>
#include <QSlider>
#include <QLineEdit>
#include <QSettings>
#include <QDebug>
#include <QTreeWidgetItem>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QMessageBox>
#include <QProcess>
#include <QStyleHints>
#include <QVariant>
#if UNIFILEHUB_ENABLE_TERMINAL
#include "../../Resources/ThirdParty/KodoTerm/include/KodoTerm/KodoTermConfig.hpp"
#endif
namespace Ui {
class Setting;
}

class Setting : public QWidget
{
    Q_OBJECT

public:
    explicit Setting(QWidget *parent = nullptr);
    ~Setting();
    bool commitSettingsAndMaybeRestart();

private slots:
    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);
    void on_file_system_Btn_clicked();
    void on_file_backup_Btn_clicked();
    void on_personal_drive_Btn_clicked();
    void on_server_config_replaceBtn_clicked();
    void on_all_setting_iconColorBtn_clicked();
    void on_all_setting_secondaryIconColorBtn_clicked();

    void on_all_setting_comboBox_currentIndexChanged(int index);
    
    void on_terminal_checkbox_powershell_stateChanged(int state);
    void on_terminal_checkbox_cmd_stateChanged(int state);

private:
    Ui::Setting *ui;
    QSettings settings;
    bool is_modified;
    void loadSettings();
    void saveSettings();
    void initTerminalThemes();
    void setupRealtimeBindings();
    void setRealtimeValue(const QString &key, const QVariant &value);

    void closeEvent(QCloseEvent *event) override;
};



#endif // SETTING_H
