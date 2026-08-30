#include "include/SettingManager.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QStandardPaths>
#include <QStringList>

namespace {

const char kConfigDirName[] = "UniFileHub";
const char kSettingsFileName[] = "settings.ini";

QString legacySettingsFilePath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(kSettingsFileName);
}

QFile::Permissions userSettingsPermissions()
{
    return QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser;
}

bool canReadWriteFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        return false;
    }

    return file.open(QIODevice::ReadWrite | QIODevice::Text);
}

bool copyFileWithUserPermissions(const QString &sourcePath, const QString &targetPath)
{
    if (!QFile::copy(sourcePath, targetPath)) {
        return false;
    }

    QFile::setPermissions(targetPath, userSettingsPermissions());
    return canReadWriteFile(targetPath);
}

bool createDefaultSettingsFile(const QString &settingsPath)
{
    if (copyFileWithUserPermissions(":/conf/settings.ini", settingsPath)) {
        return true;
    }

    QFile file(settingsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.close();
    QFile::setPermissions(settingsPath, userSettingsPermissions());
    return canReadWriteFile(settingsPath);
}

bool prepareSettingsFile(const QString &settingsPath)
{
    const QFileInfo settingsInfo(settingsPath);
    QDir settingsDir = settingsInfo.dir();
    if (!settingsDir.exists() && !settingsDir.mkpath(".")) {
        return false;
    }

    if (settingsInfo.exists()) {
        QFile::setPermissions(settingsPath, userSettingsPermissions());

        if (settingsInfo.size() == 0 && QFile::remove(settingsPath)) {
            return createDefaultSettingsFile(settingsPath);
        }

        return canReadWriteFile(settingsPath);
    }

    const QString legacyPath = legacySettingsFilePath();
    if (QDir::cleanPath(legacyPath) != QDir::cleanPath(settingsPath) &&
        QFileInfo::exists(legacyPath) &&
        copyFileWithUserPermissions(legacyPath, settingsPath)) {
        return true;
    }

    return createDefaultSettingsFile(settingsPath);
}

QStringList settingsDirectoryCandidates()
{
    QStringList directories;

    auto appendDirectory = [&directories](const QString &directoryPath) {
        if (directoryPath.isEmpty()) {
            return;
        }

        const QString cleanPath = QDir::cleanPath(directoryPath);
        if (!directories.contains(cleanPath)) {
            directories.append(cleanPath);
        }
    };

    const QString genericConfigPath =
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (!genericConfigPath.isEmpty()) {
        appendDirectory(QDir(genericConfigPath).filePath(kConfigDirName));
    }

    const QString appConfigPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!appConfigPath.isEmpty()) {
        const QString configDir = QFileInfo(appConfigPath).fileName().compare(
                                      kConfigDirName, Qt::CaseInsensitive) == 0
                                      ? appConfigPath
                                      : QDir(appConfigPath).filePath(kConfigDirName);
        appendDirectory(configDir);
    }

    const QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (!homePath.isEmpty()) {
        appendDirectory(QDir(homePath).filePath(".unifilehub"));
    }

    appendDirectory(QCoreApplication::applicationDirPath());

    return directories;
}

} // namespace

QString SettingManager::getSettingsFilePath() {
    const QStringList candidateDirs = settingsDirectoryCandidates();
    for (const QString &dirPath : candidateDirs) {
        const QString settingsPath = QDir(dirPath).filePath(kSettingsFileName);
        if (prepareSettingsFile(settingsPath)) {
            return settingsPath;
        }
    }

    return QDir(QCoreApplication::applicationDirPath()).filePath(kSettingsFileName);
}

SettingManager::SettingManager()
    : QObject(nullptr),
      settings(getSettingsFilePath(), QSettings::IniFormat) {
    QFile::setPermissions(settings.fileName(), userSettingsPermissions());
}

SettingManager::~SettingManager() {}

QVariant SettingManager::value(const QString &key, const QVariant &defaultValue) const
{
    return settings.value(key, defaultValue);
}

void SettingManager::setValue(const QString &key, const QVariant &value)
{
    if (settings.value(key) == value) {
        return;
    }

    settings.setValue(key, value);
    settings.sync();
    emit settingChanged(key, value);
}

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
    return settings.value("file_see/xlsx", true).toBool();
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

bool SettingManager::clip_board_double_click_copy_minimize()
{
    return settings.value("clip_board/double_click_copy_minimize", true).toBool();
}

bool SettingManager::clip_board_ctrl_c_copy_minimize()
{
    return settings.value("clip_board/ctrl_c_copy_minimize", true).toBool();
}

bool SettingManager::clip_board_context_menu_copy_minimize()
{
    return settings.value("clip_board/context_menu_copy_minimize", true).toBool();
}

bool SettingManager::clip_board_copy_auto_sync()
{
    return settings.value("clip_board/copy_auto_sync", false).toBool();
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

void SettingManager::set_personal_drive_download_dir(const QString &dir)
{
    setValue("PersonalDrive/DefaultDir", dir);
}

SettingManager::PersonalDriveNameConflictPolicy SettingManager::personal_drive_name_conflict_policy()
{
    const int policy = settings.value("PersonalDrive/NameConflictPolicy",
                                      int(PersonalDriveNameConflictPolicy::Ask)).toInt();

    switch (policy) {
    case int(PersonalDriveNameConflictPolicy::Overwrite):
        return PersonalDriveNameConflictPolicy::Overwrite;
    case int(PersonalDriveNameConflictPolicy::AutoRename):
        return PersonalDriveNameConflictPolicy::AutoRename;
    case int(PersonalDriveNameConflictPolicy::Ask):
    default:
        return PersonalDriveNameConflictPolicy::Ask;
    }
}

void SettingManager::set_personal_drive_name_conflict_policy(PersonalDriveNameConflictPolicy policy)
{
    setValue("PersonalDrive/NameConflictPolicy", int(policy));
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
    settings.sync();
}

QString SettingManager::getToken() {
    return settings.value("user/token", "").toString();
}

void SettingManager::setToken(const QString &token) {
    setValue("user/token", token);
}

QString SettingManager::getRefreshToken() {
    return settings.value("user/refresh_token", "").toString();
}

void SettingManager::setRefreshToken(const QString &token) {
    setValue("user/refresh_token", token);
}

QString SettingManager::getLoginUsername() {
    return settings.value("user/username", "").toString();
}

void SettingManager::setLoginUsername(const QString &username) {
    setValue("user/username", username);
}

void SettingManager::clearLoginSession() {
    settings.remove("user/token");
    settings.remove("user/refresh_token");
    settings.remove("user/username");
    settings.sync();
}

QSize SettingManager::getWindowSize() {
    int width = settings.value("window/width", 1000).toInt();
    int height = settings.value("window/height", 600).toInt();
    return QSize(width, height);
}

void SettingManager::setWindowSize(const QSize &size) {
    settings.setValue("window/width", size.width());
    settings.setValue("window/height", size.height());
    settings.sync();
}

QPoint SettingManager::getWindowPosition() {
    int x = settings.value("window/x", 100).toInt();
    int y = settings.value("window/y", 100).toInt();
    return QPoint(x, y);
}

void SettingManager::setWindowPosition(const QPoint &position) {
    settings.setValue("window/x", position.x());
    settings.setValue("window/y", position.y());
    settings.sync();
}

bool SettingManager::getWindowMaximized() {
    return settings.value("window/maximized", false).toBool();
}

void SettingManager::setWindowMaximized(bool maximized) {
    settings.setValue("window/maximized", maximized);
    settings.sync();
}

// 终端配置相关方法
QString SettingManager::terminal_font_family() {
    return settings.value("terminal/font_family", "Consolas").toString();
}

void SettingManager::set_terminal_font_family(const QString &family) {
    setValue("terminal/font_family", family);
}

int SettingManager::terminal_font_size() {
    return settings.value("terminal/font_size", 14).toInt();
}

void SettingManager::set_terminal_font_size(int size) {
    setValue("terminal/font_size", size);
}

QString SettingManager::terminal_theme() {
    return settings.value("terminal/theme", "Default").toString();
}

void SettingManager::set_terminal_theme(const QString &theme) {
    setValue("terminal/theme", theme);
}

QString SettingManager::terminal_type() {
    return settings.value("terminal/type", "powershell").toString();
}

void SettingManager::set_terminal_type(const QString &type) {
    setValue("terminal/type", type);
}

