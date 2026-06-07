#include "include/FlaskInfo.h"
#include <QTimer>

FlaskInfo::FlaskInfo(QObject *parent) : QObject(parent)
{
    address = SettingManager::Instance().serverconfig_ip4();
    networkManager = new QNetworkAccessManager(this);
}

void FlaskInfo::route_loginUser(const QString &username, const QString &password)
{
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;

    sendRequest(QUrl(address + "/user/login"), json, "login");
}

void FlaskInfo::route_registerUser(const QString &username, const QString &password, const QByteArray &avatarData)
{
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    json["avatar"] = QString(avatarData.toBase64());

    sendRequest(QUrl(address + "/user/register"), json, "register");
}

void FlaskInfo::route_loadUserInfo(const QString &username)
{
    QJsonObject json;
    json["username"] = username; // 传递用户名作为查询参数

    sendRequest(QUrl(address + "/info/get_user_info"), json, "load_user_info");
}

void FlaskInfo::route_validateSession()
{
    sendGetRequest(QUrl(address + "/user/me"), "validate_session",
                   SettingManager::Instance().getToken().trimmed());
}

void FlaskInfo::route_refreshSession()
{
    sendGetRequest(QUrl(address + "/user/refresh"), "refresh_session",
                   SettingManager::Instance().getRefreshToken().trimmed());
}

void FlaskInfo::route_updateUserInfo(const QString &username, const QMap<QString, QVariant> &userInfo)
{
    QJsonObject json;
    json["username"] = username;
    json["name"] = userInfo["name"].toString();
    json["motto"] = userInfo["motto"].toString();
    json["gender"] = userInfo["gender"].toString();
    json["birthday"] = userInfo["birthday"].toDate().toString("yyyy-MM-dd");
    json["location"] = userInfo["location"].toString();
    json["company"] = userInfo["company"].toString();

    sendRequest(QUrl(address + "/info/update_user_info"), json, "update_user_info");
}


void FlaskInfo::sendRequest(const QUrl &url, const QJsonObject &json, const QString &action)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(kRequestTimeoutMs);

    QByteArray data = QJsonDocument(json).toJson();
    emit requestStarted(action);
    QNetworkReply *reply = networkManager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, reply, action]() {
        handleResponse(reply, action);
    });
}

void FlaskInfo::sendGetRequest(const QUrl &url, const QString &action, const QString &bearerToken)
{
    QNetworkRequest request(url);
    request.setTransferTimeout(kRequestTimeoutMs);
    if (!bearerToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(bearerToken).toUtf8());
    }

    emit requestStarted(action);
    QNetworkReply *reply = (action == "refresh_session")
        ? networkManager->post(request, QByteArray())
        : networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, action]() {
        handleResponse(reply, action);
    });
}

void FlaskInfo::handleResponse(QNetworkReply *reply, const QString &action)
{
    emit requestFinished(action);
    // 先读取响应数据
    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonRes = jsonDoc.isObject() ? jsonDoc.object() : QJsonObject();

    // 获取 HTTP 状态码
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // 1️⃣ 网络层错误（连接失败、超时等），才用 reply->error()
    if (reply->error() != QNetworkReply::NoError && statusCode == 0) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    // 2️⃣ 业务层 HTTP 错误（4xx/5xx）通过 JSON 处理，不触发 errorOccurred
    if (statusCode >= 400 && statusCode < 600) {
        QString msg;
        if (jsonRes.contains("error"))
            msg = jsonRes["error"].toString();
        else if (jsonRes.contains("message"))
            msg = jsonRes["message"].toString();
        else
            msg = QString("HTTP Error %1").arg(statusCode);

        // 对注册操作特殊处理
        if (action == "login") {
            H_LoginAct(jsonRes);  // 登录失败 JSON 包含错误信息
        } else if (action == "register") {
            H_RegisterAct(jsonRes); // 让注册函数自己解析 error/message
        } else if (action == "validate_session" || action == "refresh_session") {
            emit errorOccurred(msg);
        } else {
            emit errorOccurred(msg);
        }

        reply->deleteLater();
        return;
    }

    // 3️⃣ JSON解析失败
    if (!jsonDoc.isObject()) {
        emit errorOccurred("Invalid JSON response.");
        reply->deleteLater();
        return;
    }

    // 4️⃣ 正常业务逻辑处理
    if (action == "login") {
        H_LoginAct(jsonRes);
    } else if (action == "register") {
        H_RegisterAct(jsonRes);
    } else if (action == "update_user_info") {
        H_UpdateAct(jsonRes);
    } else if (action == "load_user_info") {
        H_LoadAct(jsonRes);
    } else if (action == "validate_session") {
        emit s_sessionValidated(jsonRes);
    } else if (action == "refresh_session") {
        emit s_sessionRefreshed(jsonRes);
    }

    reply->deleteLater();
}

void FlaskInfo::H_LoginAct(const QJsonObject &jsonRes)
{

    if (jsonRes.contains("avatar_url") && !jsonRes["avatar_url"].toString().isEmpty()) {
        QString baseUrl = address;
        QString avatarUrl = baseUrl + jsonRes["avatar_url"].toString();
        fetchAvatarImage(avatarUrl, "login_avatar");
    } else {
        qDebug() << "avatar_url not found or is empty!";
    }
    emit s_loginRec(jsonRes);
}

void FlaskInfo::H_RegisterAct(const QJsonObject &jsonRes)
{
    emit s_registerRec(jsonRes);
}

void FlaskInfo::H_UpdateAct(const QJsonObject &jsonRes)
{
    if (jsonRes.contains("status") && jsonRes["status"].toString() == "success") {
        emit s_updateRec(jsonRes);
    } else {
        emit errorOccurred("Update failed.");
    }

}

void FlaskInfo::H_LoadAct(const QJsonObject &jsonRes)
{
    if (jsonRes.contains("avatar_url") && !jsonRes["avatar_url"].toString().isEmpty()) {
        fetchAvatarImage(jsonRes["avatar_url"].toString(), "load_user_avatar");
    }
    emit s_loadRec(jsonRes);
}

void FlaskInfo::fetchAvatarImage(const QString &url, const QString &action)
{
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, action]() {
        QByteArray imageData;
        if (reply->error() == QNetworkReply::NoError) {
            imageData = reply->readAll();
        } else {
            qDebug() << "Failed to download avatar:" << reply->errorString();
        }
        emit avatarDownloaded(imageData, action);
        reply->deleteLater();
    });
}
