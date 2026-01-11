#include "include/DriveManager.h"
#include "include/DriveApiClient.h"
#include "include/DriveItem.h"
#include "include/DriveFile.h"
#include "include/DriveFolder.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>

DriveManager::DriveManager(QObject *parent): QObject(parent), m_apiClient(nullptr), m_currentDirectoryId(0), m_initialized(false)
{

}

DriveManager::~DriveManager()
{
    clearCurrentFileList();
    
    if (m_apiClient) {
        delete m_apiClient;
        m_apiClient = nullptr;
    }
}

DriveManager &DriveManager::Instance()
{
    static DriveManager instance;
    return instance;
}

void DriveManager::initialize()
{
    if (m_initialized) {
        return;
    }
    
    m_apiClient = new DriveApiClient(this);
    
    // 连接DriveApiClient的信号
    connect(m_apiClient, &DriveApiClient::fileListReceived,
            this, &DriveManager::onFileListReceived);
    connect(m_apiClient, &DriveApiClient::pathReceived,
            this, &DriveManager::onPathReceived);

    connect(m_apiClient, &DriveApiClient::fileUploaded,
            this, &DriveManager::onFileUploaded);
    
    connect(m_apiClient, &DriveApiClient::folderCreated,
            this, &DriveManager::onFolderCreated);
    
    connect(m_apiClient, &DriveApiClient::fileDownloaded,
            this, &DriveManager::onFileDownloaded);
    
    connect(m_apiClient, &DriveApiClient::itemDeleted,
            this, &DriveManager::onItemDeleted);
    
    connect(m_apiClient, &DriveApiClient::itemRenamed,
            this, &DriveManager::onItemRenamed);
    
    connect(m_apiClient, &DriveApiClient::itemMoved,
            this, &DriveManager::onItemMoved);
    
    // 连接错误信号
    connect(m_apiClient, &DriveApiClient::fileListError,
            this, &DriveManager::onError);
    
    connect(m_apiClient, &DriveApiClient::fileUploadError,
            this, &DriveManager::onError);
    
    connect(m_apiClient, &DriveApiClient::folderCreateError,
            this, &DriveManager::onError);
    
    connect(m_apiClient, &DriveApiClient::fileDownloadError,
            this, &DriveManager::onError);
    
    connect(m_apiClient, &DriveApiClient::itemDeleteError,
            this, &DriveManager::onError);
    
    connect(m_apiClient, &DriveApiClient::itemRenameError,
            this, &DriveManager::onError);
    
    connect(m_apiClient, &DriveApiClient::itemMoveError,
            this, &DriveManager::onError);
    
    m_initialized = true;
}

void DriveManager::getCurrentDirectoryFiles(int parentId)
{
    if (!m_initialized) {
        initialize();
    }
    
    m_currentDirectoryId = parentId;
    m_apiClient->getFileList(parentId);
}

void DriveManager::uploadFile(const QString &filePath, int parentId)
{
    if (!m_initialized) {
        initialize();
    }
    
    m_apiClient->uploadFile(filePath, parentId);
}

void DriveManager::createFolder(const QString &folderName, int parentId)
{
    if (!m_initialized) {
        initialize();
    }
    
    m_apiClient->createFolder(folderName, parentId);
}

void DriveManager::deleteItem(int itemId)
{
    if (!m_initialized) {
        initialize();
    }
    
    m_apiClient->deleteItem(itemId);
}

void DriveManager::renameItem(int itemId, const QString &newName)
{
    if (!m_initialized) {
        initialize();
    }
    
    m_apiClient->renameItem(itemId, newName);
}

void DriveManager::moveItem(int itemId, int newParentId)
{
    if (!m_initialized) {
        initialize();
    }
    
    m_apiClient->moveItem(itemId, newParentId);
}

void DriveManager::downloadFile(int fileId, const QString &savePath)
{
    if (!m_initialized) {
        initialize();
    }
    
    m_apiClient->downloadFile(fileId, savePath);
}

void DriveManager::getPath(int dirId)
{
    if (!m_initialized) {
        initialize();
    }
    
    m_apiClient->getPath(dirId);
}

int DriveManager::getCurrentDirectoryId() const
{
    return m_currentDirectoryId;
}

void DriveManager::setCurrentDirectoryId(int dirId)
{
    m_currentDirectoryId = dirId;
}

QList<DriveItem *> DriveManager::getCurrentFileList() const
{
    return m_currentFileList;
}

void DriveManager::clearCurrentFileList()
{
    qDeleteAll(m_currentFileList);
    m_currentFileList.clear();
}

DriveApiClient *DriveManager::getApiClient() const
{
    return m_apiClient;
}

void DriveManager::onFileListReceived(const QJsonArray &fileList)
{
    // 清理当前文件列表
    clearCurrentFileList();
    
    // 将JSON转换为DriveItem对象
    for (const auto &item : fileList) {
        QJsonObject obj = item.toObject();
        DriveItem *driveItem = jsonToDriveItem(obj);
        if (driveItem) {
            m_currentFileList.append(driveItem);
        }
    }
    
    // 发送文件列表更新信号
    emit fileListUpdated(m_currentFileList);
}


void DriveManager::onFileUploaded(const QJsonObject &fileInfo)
{
    // 文件上传成功，刷新当前目录
    getCurrentDirectoryFiles(m_currentDirectoryId);
    emit operationSuccess("文件上传成功");
}

void DriveManager::onFolderCreated(const QJsonObject &folderInfo)
{
    // 文件夹创建成功，刷新当前目录
    getCurrentDirectoryFiles(m_currentDirectoryId);
    emit operationSuccess("文件夹创建成功");
}

void DriveManager::onFileDownloaded(const QString &filePath)
{
    emit operationSuccess("文件下载成功");
}

void DriveManager::onItemDeleted(int itemId)
{
    // 项目删除成功，刷新当前目录
    getCurrentDirectoryFiles(m_currentDirectoryId);
    emit operationSuccess("删除成功");
}

void DriveManager::onItemRenamed(const QJsonObject &itemInfo)
{
    // 项目重命名成功，刷新当前目录
    getCurrentDirectoryFiles(m_currentDirectoryId);
    emit operationSuccess("重命名成功");
}

void DriveManager::onItemMoved(const QJsonObject &itemInfo)
{
    // 项目移动成功，刷新当前目录
    getCurrentDirectoryFiles(m_currentDirectoryId);
    emit operationSuccess("移动成功");
}

void DriveManager::onError(const QString &errorMessage)
{
    emit operationFailed(errorMessage);
}

DriveItem *DriveManager::jsonToDriveItem(const QJsonObject &json)
{
    int id = json["id"].toInt();
    QString name = json["name"].toString();
    int parentId = json["parent_id"].toInt();
    bool isDir = json["is_dir"].toBool();
    QDateTime createdAt = QDateTime::fromString(json["created_at"].toString(), Qt::ISODate);
    
    if (isDir) {
        return new DriveFolder(id, name, parentId, createdAt);
    } else {
        qint64 size = json["size"].toInt();
        QString mimeType = json["mimetype"].toString();
        return new DriveFile(id, name, parentId, size, mimeType, createdAt);
    }
}

QJsonObject DriveManager::driveItemToJson(DriveItem *item)
{
    QJsonObject json;
    json["id"] = item->getId();
    json["name"] = item->getName();
    json["parent_id"] = item->getParentId();
    json["is_dir"] = item->isFolder();
    json["created_at"] = item->getCreatedAt().toString(Qt::ISODate);
    
    if (item->isFolder()) {
        // 文件夹不需要额外属性
    } else {
        DriveFile *file = dynamic_cast<DriveFile *>(item);
        if (file) {
            json["size"] = file->getSize();
            json["mimetype"] = file->getMimeType();
        }
    }
    
    return json;
}

void DriveManager::onPathReceived(const QJsonArray &path)
{
    QList<DriveItem *> pathItems;
    
    for (const QJsonValue &value : path) {
        if (value.isObject()) {
            QJsonObject json = value.toObject();
            DriveItem *item = jsonToDriveItem(json);
            pathItems.append(item);
        }
    }
    
    emit pathReceived(pathItems);
}
