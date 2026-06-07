#ifndef CLIPBOARDCLOUDCLIENT_H
#define CLIPBOARDCLOUDCLIENT_H

#include <QObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

class ClipboardCloudClient : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardCloudClient(QObject *parent = nullptr);

    void uploadTextItem(const QString &content);
    void fetchItems();
    void deleteItem(int cloudItemId);

signals:
    void uploadSucceeded();
    void uploadFailed(const QString &message);
    void deleteSucceeded();
    void deleteFailed(const QString &message);
    void itemsFetched(const QJsonArray &items);
    void fetchFailed(const QString &message);

private:
    QString serviceAddress() const;
    QString token() const;
    void handleJsonReply(QNetworkReply *reply,
                         const std::function<void (const QJsonDocument &doc)> &onSuccess,
                         const std::function<void (const QString &message)> &onFailure);

    QNetworkAccessManager *m_networkManager;
};

#endif // CLIPBOARDCLOUDCLIENT_H
