#include "include/DriveManager.h"
#include "include/DriveApiClient.h"
#include "include/DriveItem.h"
#include "include/DriveFile.h"
#include "include/DriveFolder.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>

DriveManager::DriveManager(QObject *parent): QObject(parent), m_apiClient(nullptr), m_currentDirectoryId(0), m_initialized(false), m_dbDriveDownload(nullptr)
{

}

DriveManager::~DriveManager()
{
    clearCurrentFileList();
    
    if (m_apiClient) {
        delete m_apiClient;
        m_apiClient = nullptr;
    }
    
    if (m_dbDriveDownload) {
        delete m_dbDriveDownload;
        m_dbDriveDownload = nullptr;
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
    
    // 初始化下载历史数据库
    m_dbDriveDownload = new dbDriveDownload("./SmartDesk.db");
    
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
    
    // 从文件列表中查找文件信息
    for (DriveItem *item : m_currentFileList) {
        if (item->getId() == fileId && !item->isFolder()) {
            DriveFile *file = dynamic_cast<DriveFile*>(item);
            if (file) {
                // 处理重名文件，使用与DriveApiClient相同的命名规则
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

                // 添加下载记录（使用处理后的实际保存路径）
                addDownloadRecord(fileId, file->getName(), file->getSize(), finalPath);
                break;
            }
        }
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
    // 更新下载状态为成功
    int recordId = getRecordIdBySavePath(filePath);
    if (recordId > 0) {
        updateDownloadStatus(recordId, "success");
    }
    
    emit fileDownloaded(filePath);
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
    // 检查是否是下载错误
    if (errorMessage.contains("下载") || errorMessage.contains("download", Qt::CaseInsensitive)) {
        emit downloadFailed(errorMessage);
    }
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

// ========== 下载历史管理 ==========

void DriveManager::addDownloadRecord(int fileId, const QString &fileName, qint64 fileSize, const QString &savePath)
{
    if (!m_dbDriveDownload) {
        return;
    }
    
    DriveDownloadRecord record;
    record.fileId = fileId;
    record.fileName = fileName;
    record.fileSize = fileSize;
    record.savePath = savePath;
    record.downloadTime = QDateTime::currentDateTime();
    record.downloadStatus = "downloading"; // 初始状态为下载中
    record.fileType = fileName.section('.', -1);
    
    if (m_dbDriveDownload->addDownloadRecord(record)) {
        // 获取刚插入的记录ID
        int recordId = m_dbDriveDownload->getRecordIdBySavePath(savePath);
        if (recordId > 0) {
            m_downloadRecordMap[fileId] = recordId;
        }
    }
}

QList<DriveDownloadRecord> DriveManager::getDownloadHistory()
{
    if (!m_dbDriveDownload) {
        return QList<DriveDownloadRecord>();
    }
    
    return m_dbDriveDownload->getDownloadHistory();
}

bool DriveManager::clearDownloadHistory()
{
    if (!m_dbDriveDownload) {
        return false;
    }
    
    return m_dbDriveDownload->clearDownloadHistory();
}

bool DriveManager::updateDownloadStatus(int recordId, const QString &status)
{
    if (!m_dbDriveDownload) {
        return false;
    }
    
    return m_dbDriveDownload->updateDownloadStatus(recordId, status);
}

int DriveManager::getRecordIdBySavePath(const QString &savePath)
{
    if (!m_dbDriveDownload) {
        return -1;
    }
    
    return m_dbDriveDownload->getRecordIdBySavePath(savePath);
}
