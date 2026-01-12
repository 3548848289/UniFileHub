#include "include/DriveApiClient.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDebug>
#include "../Setting/include/SettingManager.h"

DriveApiClient::DriveApiClient(QObject *parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    // 初始化服务器IP缓存
    updateServerIpCache();
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

void DriveApiClient::updateServerIpCache()
{
    m_serverIp = SettingManager::Instance().personal_drive_server_ip();
}

void DriveApiClient::getFileList(int parentId)
{
    QUrl url(QString("%1/api/drive/list?parent_id=%2").arg(m_serverIp).arg(parentId));
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

    QUrl url(QString("%1/api/drive/upload").arg(m_serverIp));
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
    qDebug() << "Downloading file with ID: " << fileId;  // 调试打印 fileId

    QUrl url(QString("%1/api/drive/download/%2").arg(m_serverIp).arg(fileId));
    QNetworkRequest request = createRequest(url);

    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit fileDownloadError(reply->errorString());
            return;
        }

        // 处理重名文件，自动生成新文件名
        QString finalPath = savePath;
        QFileInfo fileInfo(savePath);
        QString baseName = fileInfo.baseName();
        QString suffix = fileInfo.completeSuffix();
        QString path = fileInfo.path();
        int counter = 1;

        // 如果文件已存在，生成新的文件名
        while (QFile::exists(finalPath)) {
            QString newName;
            if (suffix.isEmpty()) {
                newName = QString("%1 (%2)").arg(baseName).arg(counter);
            } else {
                newName = QString("%1 (%2).%3").arg(baseName).arg(counter).arg(suffix);
            }
            finalPath = QDir(path).absoluteFilePath(newName);
            counter++;
        }

        QFile file(finalPath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit fileDownloadError("无法保存文件");
            return;
        }

        file.write(reply->readAll());
        file.close();

        emit fileDownloaded(finalPath);
    });
}

void DriveApiClient::createFolder(const QString &folderName, int parentId)
{
    // 后端接口格式: /new_folder/<int:parent_id>/<folder_name>
    QUrl url(QString("%1/api/drive/new_folder/%2/%3").arg(m_serverIp).arg(parentId).arg(folderName));
    QNetworkRequest request = createRequest(url);
    
    // 接口不需要JSON数据，直接发送POST请求
    QNetworkReply *reply = m_networkManager->post(request, QByteArray());
    
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
        
        QJsonObject response = doc.object();
        if (response["msg"].toString() == "ok") {
            emit folderCreated(response);
        } else {
            emit folderCreateError(response["msg"].toString());
        }
    });
}

void DriveApiClient::deleteItem(int itemId)
{
    QUrl url(QString("%1/api/drive/delete/%2").arg(m_serverIp).arg(itemId));
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
    QUrl url(QString("%1/api/drive/path?dir_id=%2").arg(m_serverIp).arg(dirId));
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
    QUrl url(QString("%1/api/drive/rename").arg(m_serverIp));
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
    QUrl url(QString("%1/api/drive/move").arg(m_serverIp));
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
