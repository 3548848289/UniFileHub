#ifndef SETTINGMANAGER_H
#define SETTINGMANAGER_H

#include<QSettings>
#include<QSize>
#include<QPoint>

class SettingManager
{
public:
    // 获取单例实例的静态方法
    static SettingManager& Instance()
    {
        static SettingManager instance;
        return instance;
    }

    // 禁止拷贝构造和赋值
    SettingManager(const SettingManager&) = delete;
    SettingManager& operator=(const SettingManager&) = delete;

    int all_setting_font_size();
    bool all_setting_fenable_tray();

    int file_see_font_size();
    bool file_see_txt();
    bool file_see_csv();
    bool file_see_xlsx();
    bool file_see_img();


    QString tag_schedule_reminder_type();

    QString file_system_file_system_dir();

    int tag_schedule_show_time();

    void loadHistory();

    void saveHistory();

    int clip_board_hours();

    QString serverconfig_ip1();
    QString serverconfig_ip2();
    QString serverconfig_ip3();
    QString serverconfig_ip4();
    QString personal_drive_server_ip();
    QString personal_drive_download_dir();
    QString file_backup_IP();
    int all_setting_theme();
    QString all_setting_icon_color();
    QString all_setting_secondary_icon_color();
    
    // Token相关方法
    QString getToken();
    void setToken(const QString &token);
    
    // 窗口大小和位置相关方法
    QSize getWindowSize();
    void setWindowSize(const QSize &size);
    QPoint getWindowPosition();
    void setWindowPosition(const QPoint &position);
private:
    SettingManager();
    ~SettingManager();
    QSettings settings;
    QStringList fileHistory;
};

#endif // SETTINGMANAGER_H
