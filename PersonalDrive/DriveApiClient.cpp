#include "include/DriveApiClient.h"
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDebug>
#include "../Setting/include/SettingManager.h"

DriveApiClient::DriveApiClient(QObject *parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

DriveApiClient::~DriveApiClient()
{
    // 清理资源
}

QString DriveApiClient::getToken()
{
    return SettingManager::Instance().getToken();
}

QNetworkRequest DriveApiClient::createRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + getToken().toUtf8());
    return request;
}

void DriveApiClient::getFileList(int parentId)
{
    QUrl url(QString("http://127.0.0.1:5005/api/drive/list?parent_id=%1").arg(parentId));
    QNetworkRequest request = createRequest(url);
    
    QNetworkReply *reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit fileListError(reply->errorString());
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            emit fileListError("Invalid response format");
            return;
        }
        
        emit fileListReceived(doc.array());
    });
}


void DriveApiClient::uploadFile(const QString &filePath, int parentId)
{
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        emit fileUploadError("无法打开文件");
        delete file;
        return;
    }
    
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // 文件部分
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"" + QFileInfo(filePath).fileName() + "\""));
    filePart.setBodyDevice(file);
    file->setParent(multiPart); // 让 multiPart 管理 file 生命周期
    multiPart->append(filePart);
    
    // 父目录ID部分
    QHttpPart parentPart;
    parentPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"parent_id\""));
    parentPart.setBody(QString::number(parentId).toUtf8());
    multiPart->append(parentPart);
    
    QUrl url("http://127.0.0.1:5005/api/drive/upload");
    QNetworkRequest request = createRequest(url);
    
    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply); // 让 reply 管理 multiPart 生命周期
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit fileUploadError(reply->errorString());
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit fileUploadError("Invalid response format");
            return;
        }
        
        emit fileUploaded(doc.object());
    });
}

void DriveApiClient::downloadFile(int fileId, const QString &savePath)
{
    QUrl url(QString("http://127.0.0.1:5005/api/drive/download?file_id=%1").arg(fileId));
    QNetworkRequest request = createRequest(url);
    
    QNetworkReply *reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit fileDownloadError(reply->errorString());
            return;
        }
        
        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit fileDownloadError("无法保存文件");
            return;
        }
        
        file.write(reply->readAll());
        file.close();
        
        emit fileDownloaded(savePath);
    });
}

void DriveApiClient::createFolder(const QString &folderName, int parentId)
{
    QUrl url("http://127.0.0.1:5005/api/drive/create_folder");
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["name"] = folderName;
    json["parent_id"] = parentId;
    
    QByteArray data = QJsonDocument(json).toJson();
    
    QNetworkReply *reply = m_networkManager->post(request, data);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit folderCreateError(reply->errorString());
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit folderCreateError("Invalid response format");
            return;
        }
        
        emit folderCreated(doc.object());
    });
}

void DriveApiClient::deleteItem(int itemId)
{
    QUrl url(QString("http://127.0.0.1:5005/api/drive/delete?item_id=%1").arg(itemId));
    QNetworkRequest request = createRequest(url);
    
    QNetworkReply *reply = m_networkManager->deleteResource(request);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit itemDeleteError(reply->errorString());
            return;
        }
        
        emit itemDeleted(itemId);
    });
}

void DriveApiClient::getPath(int dirId)
{
    QUrl url(QString("http://127.0.0.1:5005/api/drive/path?dir_id=%1").arg(dirId));
    QNetworkRequest request = createRequest(url);
    
    QNetworkReply *reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit fileListError(reply->errorString());
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            emit fileListError("Invalid response format");
            return;
        }
        
        emit pathReceived(doc.array());
    });
}

void DriveApiClient::renameItem(int itemId, const QString &newName)
{
    QUrl url("http://127.0.0.1:5005/api/drive/rename");
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["item_id"] = itemId;
    json["new_name"] = newName;
    
    QByteArray data = QJsonDocument(json).toJson();
    
    QNetworkReply *reply = m_networkManager->post(request, data);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit itemRenameError(reply->errorString());
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit itemRenameError("Invalid response format");
            return;
        }
        
        emit itemRenamed(doc.object());
    });
}

void DriveApiClient::moveItem(int itemId, int newParentId)
{
    QUrl url("http://127.0.0.1:5005/api/drive/move");
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["item_id"] = itemId;
    json["new_parent_id"] = newParentId;
    
    QByteArray data = QJsonDocument(json).toJson();
    
    QNetworkReply *reply = m_networkManager->post(request, data);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit itemMoveError(reply->errorString());
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit itemMoveError("Invalid response format");
            return;
        }
        
        emit itemMoved(doc.object());
    });
}
