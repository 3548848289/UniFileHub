// ServerManager.h
#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QObject>


#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QNetworkRequest>

#include <QModelIndex>
#include <QAbstractItemModel>
#include <QFile>
#include <QStringList>
#include <QDebug>
#include <QFileInfo>
#include <QString>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QRegularExpression>
#include <QPointer>
#include <QMessageBox>
#include "../../Setting/include/SettingManager.h"

static QString friendlyErrorMessage(QNetworkReply::NetworkError code)
{
    switch (code) {
    case QNetworkReply::ConnectionRefusedError:
        return "无法连接服务器，请检查网络或稍后重试。";
    case QNetworkReply::RemoteHostClosedError:
        return "服务器断开了连接，请稍后再试。";
    case QNetworkReply::HostNotFoundError:
        return "服务器地址不存在，请检查设置。";
    case QNetworkReply::TimeoutError:
        return "连接服务器超时，请检查网络后重试。";
    case QNetworkReply::OperationCanceledError:
        return "上传已取消。";
    case QNetworkReply::ContentAccessDenied:
        return "没有权限上传文件，请联系管理员。";
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::UnknownContentError:
    case QNetworkReply::UnknownServerError:
        return "发生未知服务器错误，请稍后重试。";

    default:
        return "上传失败，请检查网络后再试。";
    }
}

class ServerManager : public QObject {
    Q_OBJECT

public:
    static ServerManager* instance() {
        address1 = SettingManager::Instance().serverconfig_ip1();
        address2 = SettingManager::Instance().serverconfig_ip2();
        static ServerManager* instance = new ServerManager();
        return instance;
    }
    ServerManager(const ServerManager&) = delete;
    ServerManager& operator=(const ServerManager&) = delete;

    void test(const QString& filepath);
    void setCurdir(const QString& curdir);
    void sendfilepath(QString filepath);

    void commitFile(const QString& filepath);
    void downloadFile(const QString& filepath);
    void getHistory();
    bool setSharedFile(const QString &filepath, const QString &shareToken);
    void getSharedFile(const QString& shareToken);
    void checkFileExists(const QString &filepath);
signals:
    void commitSuccess();
    void commitFailed();
    void historyReceived(const QStringList& history);
    void onFilesListUpdated(const QString& files);
    void fileListReady(const QStringList &fileNames);
    void returnStatus(bool exists);

private slots:
    void oncommitFin(QPointer<QNetworkReply> reply);
    void ondownloadFin(QNetworkReply* reply, const QString& fileName);
    void onhistoryFin(QNetworkReply* reply);

private:
    explicit ServerManager(QObject *parent = nullptr) : QObject(parent) {}
    ~ServerManager() = default;
    QNetworkAccessManager networkManager;
    QString m_curdir;
    static QString address1;
    static QString address2;

};

#endif // SERVERMANAGER_H
