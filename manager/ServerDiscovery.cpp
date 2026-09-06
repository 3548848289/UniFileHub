#include "ServerDiscovery.h"
#include <QNetworkInterface>
#include <QHostAddress>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

const char* ServerDiscovery::REQUEST_MAGIC = "UniFileHub-DISCOVERY-v1";
const char* ServerDiscovery::RESPONSE_MAGIC = "UniFileHub-SERVER-v1";

ServerDiscovery::ServerDiscovery(int timeoutMs, QObject *parent)
    : QObject(parent), timeoutMs(timeoutMs)
{
    udpSocket = new QUdpSocket(this);
    connect(udpSocket, &QUdpSocket::readyRead, this, &ServerDiscovery::onReadyRead);

    timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, &ServerDiscovery::onTimeout);
}

ServerDiscovery::~ServerDiscovery() = default;

void ServerDiscovery::start()
{
    // ShareAddress：Windows 上允许多个套接字共存；绑定后再发广播才能收到应答
    if (!udpSocket->bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "[discovery] UDP bind failed:" << udpSocket->errorString();
        finish(false);
        return;
    }

    sendProbe();
    timeoutTimer->start(timeoutMs);
}

void ServerDiscovery::sendProbe()
{
    const QByteArray probe = QByteArray(REQUEST_MAGIC);

    quint64 sent = udpSocket->writeDatagram(probe, QHostAddress::Broadcast, DISCOVERY_PORT);

    // 多网卡场景（VPN、虚拟机网卡）下定向广播更可靠：逐个物理网卡发一遍
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::CanBroadcast))
            continue;
        if (iface.flags() & QNetworkInterface::IsLoopBack)
            continue;

        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
                continue;
            if (entry.broadcast().isNull())
                continue;
            udpSocket->writeDatagram(probe, entry.broadcast(), DISCOVERY_PORT);
        }
    }

    if (sent < 0 && udpSocket->error() != QAbstractSocket::UnknownSocketError) {
        qWarning() << "[discovery] broadcast send failed:" << udpSocket->errorString();
    }
}

void ServerDiscovery::onReadyRead()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        if (udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort) < 0)
            continue;

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(datagram, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject())
            continue;

        const QJsonObject obj = doc.object();
        if (obj.value("magic").toString() != QLatin1String(RESPONSE_MAGIC))
            continue;

        QString ip = obj.value("ip").toString();
        if (ip.isEmpty() || ip == QLatin1String("127.0.0.1"))
            ip = sender.toString();

        const QJsonObject ports = obj.value("ports").toObject();
        if (ports.isEmpty())
            continue;

        timeoutTimer->stop();
        verifyAndApply(ip, ports);
        return;
    }
}

void ServerDiscovery::verifyAndApply(const QString& ip, const QJsonObject& ports)
{
    // 用一个真实服务端口做连通性验证，确认这台机器确实是 UniFileHub 服务端
    const QString verifyPortKey = ports.contains("ip4") ? QStringLiteral("ip4") : QStringLiteral("ip2");
    const int verifyPort = ports.value(verifyPortKey).toInt();
    if (verifyPort <= 0) {
        finish(false);
        return;
    }

    pendingIp = ip;
    pendingPorts = ports;

    if (!networkManager)
        networkManager = new QNetworkAccessManager(this);

    QNetworkRequest request{QUrl(QString("http://%1:%2/").arg(ip).arg(verifyPort))};
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(3000);
#endif
    verifyReply = networkManager->get(request);
    connect(verifyReply, &QNetworkReply::finished, this, &ServerDiscovery::onVerifyFinished);
}

void ServerDiscovery::onVerifyFinished()
{
    QNetworkReply* reply = verifyReply;
    verifyReply = nullptr;
    if (!reply) {
        finish(false);
        return;
    }

    reply->deleteLater();

    // 只要有 HTTP 层的应答（哪怕 404）就说明服务端在线；
    // 连接被拒/超时/找不到主机才视为失败
    switch (reply->error()) {
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::NetworkSessionFailedError:
        qWarning() << "[discovery] verify failed:" << reply->errorString();
        finish(false);
        return;
    default:
        break;
    }

    // 写入全部服务地址。setValue 会触发 settingChanged，
    // ServerManager 等已监听该信号的模块会自动更新地址
    const QJsonObject& ports = pendingPorts;
    const QString ip = pendingIp;

    struct Mapping { const char* key; const char* portKey; };
    static const Mapping mappings[] = {
        {"ServerConfig/IP1", "ip1"},
        {"ServerConfig/IP2", "ip2"},
        {"ServerConfig/IP3", "ip3"},
        {"ServerConfig/IP4", "ip4"},
        {"ClipboardSync/ServerIP", "clipboard"},
        {"PersonalDrive/ServerIP", "drive"},
    };

    SettingManager& settings = SettingManager::Instance();
    for (const Mapping& m : mappings) {
        const int port = ports.value(QLatin1String(m.portKey)).toInt(-1);
        if (port <= 0)
            continue;
        const QString url = QString("http://%1:%2/").arg(ip).arg(port);
        if (settings.value(QLatin1String(m.key)).toString() != url)
            settings.setValue(QLatin1String(m.key), url);
    }

    qDebug() << "[discovery] server found at" << ip << "- settings updated";
    emit discovered(ip);
    finish(true);
}

void ServerDiscovery::onTimeout()
{
    finish(false);
}

void ServerDiscovery::finish(bool found)
{
    if (done)
        return;
    done = true;
    if (verifyReply) {
        verifyReply->abort();
        verifyReply->deleteLater();
        verifyReply = nullptr;
    }
    emit finished(found);
    deleteLater();
}
