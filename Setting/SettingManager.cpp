#include "include/SettingManager.h"

SettingManager::SettingManager():settings("settings.ini", QSettings::IniFormat) {}

SettingManager::~SettingManager() {}

int SettingManager::all_setting_font_size() {
    return settings.value("all_setting/font_size", 12).toInt();
}

int SettingManager::all_setting_theme() {
    return settings.value("all_setting/theme", "0").toInt();
}

QString SettingManager::all_setting_icon_color() {
    return settings.value("all_setting/icon_color", "#7598db").toString();
}

QString SettingManager::all_setting_secondary_icon_color() {
    return settings.value("all_setting/secondary_icon_color", "#7598db").toString();
}

bool SettingManager::all_setting_fenable_tray() {
    return settings.value("all_setting/fenableray", true).toBool();
}


int SettingManager::file_see_font_size() {
    return settings.value("file_see/font_size", 12).toInt();

}

bool SettingManager::file_see_txt()
{
    return settings.value("file_see/txt", true).toBool();
}

bool SettingManager::file_see_csv()
{
    return settings.value("file_see/csv", true).toBool();
}

bool SettingManager::file_see_xlsx()
{
    return settings.value("file_see/slsx", true).toBool();
}

bool SettingManager::file_see_img()
{
    return settings.value("file_see/img", true).toBool();
}

QString SettingManager::file_backup_IP()
{
    return settings.value("file_backup/IP", "127.0.0.1").toString();
}




QString SettingManager::file_system_file_system_dir()
{
    return settings.value("file_system/file_system_dir").toString();
}

int SettingManager::tag_schedule_show_time() {
    int timeInSeconds = settings.value("tag_schedule/show_time", 60).toInt();
    return timeInSeconds;
}

int SettingManager::clip_board_hours() {
    int hours = settings.value("clip_board/hours", 24).toInt();
    return hours;
}

QString SettingManager::serverconfig_ip1()
{
    return settings.value("ServerConfig/IP1", "http://43.139.86.56:5002/").toString();
}

QString SettingManager::serverconfig_ip2()
{
    return settings.value("ServerConfig/IP2", "http://43.139.86.56:5000/").toString();
}
QString SettingManager::serverconfig_ip3()
{
    return settings.value("ServerConfig/IP3", "http://43.139.86.56:5003/").toString();
}
QString SettingManager::serverconfig_ip4()
{
    return settings.value("ServerConfig/IP4", "http://43.139.86.56:5001/").toString();
}

QString SettingManager::personal_drive_server_ip()
{
    return settings.value("PersonalDrive/ServerIP", "http://43.139.86.56:5005/").toString();
}

QString SettingManager::personal_drive_download_dir()
{
    return settings.value("PersonalDrive/DefaultDir").toString();
}

void SettingManager::loadHistory()
{
    int size = settings.beginReadArray("fileHistory");
    fileHistory.clear();
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        fileHistory.append(settings.value("path").toString());
    }
    settings.endArray();
}

void SettingManager::saveHistory()
{
    settings.beginWriteArray("fileHistory");
    for (int i = 0; i < fileHistory.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("path", fileHistory[i]);
    }
    settings.endArray();
}

QString SettingManager::getToken() {
    return settings.value("user/token", "").toString();
}

void SettingManager::setToken(const QString &token) {
    settings.setValue("user/token", token);
}

QSize SettingManager::getWindowSize() {
    int width = settings.value("window/width", 1000).toInt();
    int height = settings.value("window/height", 600).toInt();
    return QSize(width, height);
}

void SettingManager::setWindowSize(const QSize &size) {
    settings.setValue("window/width", size.width());
    settings.setValue("window/height", size.height());
}

QPoint SettingManager::getWindowPosition() {
    int x = settings.value("window/x", 100).toInt();
    int y = settings.value("window/y", 100).toInt();
    return QPoint(x, y);
}

void SettingManager::setWindowPosition(const QPoint &position) {
    settings.setValue("window/x", position.x());
    settings.setValue("window/y", position.y());
}

bool SettingManager::getWindowMaximized() {
    return settings.value("window/maximized", false).toBool();
}

void SettingManager::setWindowMaximized(bool maximized) {
    settings.setValue("window/maximized", maximized);
}

