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

namespace Ui {
class Setting;
}

class Setting : public QWidget
{
    Q_OBJECT

public:
    explicit Setting(QWidget *parent = nullptr);
    ~Setting();

private slots:
    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);
    void on_file_system_Btn_clicked();
    void on_file_backup_Btn_clicked();
    void on_personal_drive_Btn_clicked();
    void on_server_config_replaceBtn_clicked();
    void on_all_setting_iconColorBtn_clicked();
    void on_all_setting_secondaryIconColorBtn_clicked();

    void on_all_setting_comboBox_currentIndexChanged(int index);

private:
    Ui::Setting *ui;
    QSettings settings;
    bool is_modified;
    void loadSettings();
    void saveSettings();

    void closeEvent(QCloseEvent *event);
};



#endif // SETTING_H
