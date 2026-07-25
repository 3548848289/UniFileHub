#include "include/ClipboardCloudClient.h"
#include "../Setting/include/SettingManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QVariant>

ClipboardCloudClient::ClipboardCloudClient(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_eventReconnectTimer(new QTimer(this))
{
    m_eventReconnectTimer->setSingleShot(true);
    m_eventReconnectTimer->setInterval(3000);
    connect(m_eventReconnectTimer, &QTimer::timeout,
            this, &ClipboardCloudClient::restartEventStream);

    connect(&SettingManager::Instance(), &SettingManager::settingChanged,
            this, [this](const QString &key, const QVariant &) {
                if (key == QStringLiteral("ClipboardSync/ServerIP") ||
                    key == QStringLiteral("user/token")) {
                    restartEventStream();
                }
            });
}

QString ClipboardCloudClient::serviceAddress() const
{
    QString address = SettingManager::Instance().clipboard_sync_server_ip().trimmed();
    if (address.endsWith('/')) {
        address.chop(1);
    }
    return address;
}

QString ClipboardCloudClient::token() const
{
    return SettingManager::Instance().getToken().trimmed();
}

void ClipboardCloudClient::uploadTextItem(const QString &content)
{
    if (serviceAddress().isEmpty() || token().isEmpty()) {
        emit uploadFailed(QStringLiteral("未登录或剪切板服务地址未配置"));
        return;
    }

    QNetworkRequest request(QUrl(serviceAddress() + "/clipboard/items"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + token().toUtf8());
    request.setTransferTimeout(15000);

    QJsonObject payload;
    payload["content"] = content;

    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleJsonReply(reply,
                        [this](const QJsonDocument &) { emit uploadSucceeded(); },
                        [this](const QString &message) { emit uploadFailed(message); });
    });
}

void ClipboardCloudClient::fetchItems()
{
    if (serviceAddress().isEmpty() || token().isEmpty()) {
        emit fetchFailed(QStringLiteral("未登录或剪切板服务地址未配置"));
        return;
    }

    QNetworkRequest request(QUrl(serviceAddress() + "/clipboard/items"));
    request.setRawHeader("Authorization", "Bearer " + token().toUtf8());
    request.setTransferTimeout(15000);

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleJsonReply(reply,
                        [this](const QJsonDocument &doc) {
                            if (!doc.isObject()) {
                                emit fetchFailed(QStringLiteral("服务端返回格式不正确"));
                                return;
                            }
                            emit itemsFetched(doc.object().value("items").toArray());
                        },
                        [this](const QString &message) { emit fetchFailed(message); });
    });
}

void ClipboardCloudClient::deleteItem(int cloudItemId)
{
    if (serviceAddress().isEmpty() || token().isEmpty()) {
        emit deleteFailed(QStringLiteral("未登录或剪切板服务地址未配置"));
        return;
    }
    if (cloudItemId <= 0) {
        emit deleteFailed(QStringLiteral("未找到云端记录"));
        return;
    }

    QNetworkRequest request(QUrl(serviceAddress() + QString("/clipboard/items/%1").arg(cloudItemId)));
    request.setRawHeader("Authorization", "Bearer " + token().toUtf8());
    request.setTransferTimeout(15000);

    QNetworkReply *reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleJsonReply(reply,
                        [this](const QJsonDocument &) { emit deleteSucceeded(); },
                        [this](const QString &message) { emit deleteFailed(message); });
    });
}

void ClipboardCloudClient::startEventStream()
{
    m_eventStreamEnabled = true;
    restartEventStream();
}

void ClipboardCloudClient::stopEventStream()
{
    m_eventStreamEnabled = false;
    if (m_eventReconnectTimer) {
        m_eventReconnectTimer->stop();
    }
    if (m_eventReply) {
        QNetworkReply *reply = m_eventReply;
        m_eventReply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    m_eventBuffer.clear();
}

void ClipboardCloudClient::restartEventStream()
{
    if (!m_eventStreamEnabled) {
        return;
    }

    if (m_eventReconnectTimer) {
        m_eventReconnectTimer->stop();
    }
    if (m_eventReply) {
        QNetworkReply *reply = m_eventReply;
        m_eventReply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    m_eventBuffer.clear();

    if (serviceAddress().isEmpty() || token().isEmpty()) {
        scheduleEventStreamReconnect();
        return;
    }

    QNetworkRequest request(QUrl(serviceAddress() + "/clipboard/events"));
    request.setRawHeader("Authorization", "Bearer " + token().toUtf8());
    request.setRawHeader("Accept", "text/event-stream");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_eventReply = m_networkManager->get(request);
    connect(m_eventReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_eventReply) {
            processEventStreamBytes(m_eventReply->readAll());
        }
    });
    connect(m_eventReply, &QNetworkReply::finished, this, [this]() {
        if (!m_eventReply) {
            return;
        }
        QNetworkReply *reply = m_eventReply;
        m_eventReply = nullptr;
        reply->deleteLater();
        scheduleEventStreamReconnect();
    });
}

void ClipboardCloudClient::scheduleEventStreamReconnect()
{
    if (!m_eventStreamEnabled || !m_eventReconnectTimer) {
        return;
    }
    if (!m_eventReconnectTimer->isActive()) {
        m_eventReconnectTimer->start();
    }
}

void ClipboardCloudClient::processEventStreamBytes(const QByteArray &bytes)
{
    m_eventBuffer.append(bytes);
    m_eventBuffer.replace("\r\n", "\n");

    int separatorIndex = m_eventBuffer.indexOf("\n\n");
    while (separatorIndex >= 0) {
        const QByteArray block = m_eventBuffer.left(separatorIndex);
        m_eventBuffer.remove(0, separatorIndex + 2);
        processEventStreamBlock(block);
        separatorIndex = m_eventBuffer.indexOf("\n\n");
    }
}

void ClipboardCloudClient::processEventStreamBlock(const QByteArray &block)
{
    if (block.trimmed().isEmpty() || block.trimmed().startsWith(':')) {
        return;
    }

    bool isClipboardChanged = false;
    const QList<QByteArray> lines = block.split('\n');
    for (const QByteArray &line : lines) {
        if (line.trimmed() == QByteArrayLiteral("event: clipboard_changed")) {
            isClipboardChanged = true;
            break;
        }
    }

    if (isClipboardChanged) {
        emit cloudItemsChanged();
    }
}

void ClipboardCloudClient::handleJsonReply(
    QNetworkReply *reply,
    const std::function<void (const QJsonDocument &doc)> &onSuccess,
    const std::function<void (const QString &message)> &onFailure)
{
    const QByteArray responseBytes = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QJsonDocument doc = QJsonDocument::fromJson(responseBytes);

    if (reply->error() != QNetworkReply::NoError && statusCode == 0) {
        onFailure(reply->errorString());
        reply->deleteLater();
        return;
    }

    if (statusCode >= 400) {
        QString message = QStringLiteral("请求失败");
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            if (obj.contains("message")) {
                message = obj.value("message").toString();
            } else if (obj.contains("error")) {
                message = obj.value("error").toString();
            }
        }
        onFailure(message);
        reply->deleteLater();
        return;
    }

    onSuccess(doc);
    reply->deleteLater();
}
