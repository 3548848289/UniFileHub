#ifndef CLIPBOARDCLOUDCLIENT_H
#define CLIPBOARDCLOUDCLIENT_H

#include <QObject>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class ClipboardCloudClient : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardCloudClient(QObject *parent = nullptr);

    void uploadTextItem(const QString &content);
    void fetchItems();
    void deleteItem(int cloudItemId);
    void startEventStream();
    void stopEventStream();

signals:
    void uploadSucceeded();
    void uploadFailed(const QString &message);
    void deleteSucceeded();
    void deleteFailed(const QString &message);
    void itemsFetched(const QJsonArray &items);
    void fetchFailed(const QString &message);
    void cloudItemsChanged();

private:
    QString serviceAddress() const;
    QString token() const;
    void handleJsonReply(QNetworkReply *reply,
                         const std::function<void (const QJsonDocument &doc)> &onSuccess,
                         const std::function<void (const QString &message)> &onFailure);
    void restartEventStream();
    void scheduleEventStreamReconnect();
    void processEventStreamBytes(const QByteArray &bytes);
    void processEventStreamBlock(const QByteArray &block);

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_eventReply = nullptr;
    QTimer *m_eventReconnectTimer = nullptr;
    QByteArray m_eventBuffer;
    bool m_eventStreamEnabled = false;
};

#endif // CLIPBOARDCLOUDCLIENT_H
