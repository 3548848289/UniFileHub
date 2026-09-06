// ServerDiscovery.h
#ifndef SERVERDISCOVERY_H
#define SERVERDISCOVERY_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include "../../Setting/include/SettingManager.h"

// 局域网服务发现：启动时广播探测，服务端 Discovery 服务应答 IP + 端口，
// 验证连通后自动写入 ServerConfig/IPx、ClipboardSync/ServerIP、PersonalDrive/ServerIP。
class ServerDiscovery : public QObject {
    Q_OBJECT

public:
    static constexpr quint16 DISCOVERY_PORT = 15000;
    static const char* REQUEST_MAGIC;
    static const char* RESPONSE_MAGIC;

    // timeoutMs 为整个扫描的时长上限
    explicit ServerDiscovery(int timeoutMs = 2000, QObject *parent = nullptr);
    ~ServerDiscovery() override;

    void start();

signals:
    void discovered(const QString& serverIp);
    void finished(bool found);

private slots:
    void onReadyRead();
    void onTimeout();
    void onVerifyFinished();

private:
    void sendProbe();
    void verifyAndApply(const QString& ip, const QJsonObject& ports);
    void finish(bool found);

    QUdpSocket* udpSocket = nullptr;
    QTimer* timeoutTimer = nullptr;
    QNetworkAccessManager* networkManager = nullptr;
    QNetworkReply* verifyReply = nullptr;
    int timeoutMs;
    bool done = false;
    QString pendingIp;
    QJsonObject pendingPorts;
};

#endif // SERVERDISCOVERY_H
