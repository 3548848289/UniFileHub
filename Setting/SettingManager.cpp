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
    return settings.value("ServerConfig/IP1", "http://127.0.0.1:5002/").toString();
}

QString SettingManager::serverconfig_ip2()
{
    return settings.value("ServerConfig/IP2", "http://127.0.0.1:5000/").toString();
}
QString SettingManager::serverconfig_ip3()
{
    return settings.value("ServerConfig/IP3", "http://127.0.0.1:5003/").toString();
}
QString SettingManager::serverconfig_ip4()
{
    return settings.value("ServerConfig/IP4", "http://127.0.0.1:5001/").toString();
}

QString SettingManager::clipboard_sync_server_ip()
{
    return settings.value("ClipboardSync/ServerIP", "http://127.0.0.1:5006/").toString();
}

QString SettingManager::personal_drive_server_ip()
{
    return settings.value("PersonalDrive/ServerIP", "http://127.0.0.1:5005/").toString();
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

QString SettingManager::getRefreshToken() {
    return settings.value("user/refresh_token", "").toString();
}

void SettingManager::setRefreshToken(const QString &token) {
    settings.setValue("user/refresh_token", token);
}

QString SettingManager::getLoginUsername() {
    return settings.value("user/username", "").toString();
}

void SettingManager::setLoginUsername(const QString &username) {
    settings.setValue("user/username", username);
}

void SettingManager::clearLoginSession() {
    settings.remove("user/token");
    settings.remove("user/refresh_token");
    settings.remove("user/username");
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

// 终端配置相关方法
QString SettingManager::terminal_font_family() {
    return settings.value("terminal/font_family", "Consolas").toString();
}

void SettingManager::set_terminal_font_family(const QString &family) {
    settings.setValue("terminal/font_family", family);
}

int SettingManager::terminal_font_size() {
    return settings.value("terminal/font_size", 14).toInt();
}

void SettingManager::set_terminal_font_size(int size) {
    settings.setValue("terminal/font_size", size);
}

QString SettingManager::terminal_theme() {
    return settings.value("terminal/theme", "Default").toString();
}

void SettingManager::set_terminal_theme(const QString &theme) {
    settings.setValue("terminal/theme", theme);
}

QString SettingManager::terminal_type() {
    return settings.value("terminal/type", "powershell").toString();
}

void SettingManager::set_terminal_type(const QString &type) {
    settings.setValue("terminal/type", type);
}

