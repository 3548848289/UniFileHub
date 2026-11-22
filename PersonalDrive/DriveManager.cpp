#include "include/DriveManager.h"
#include "include/DriveApiClient.h"
#include "include/DriveItem.h"
#include "include/DriveFile.h"
#include "include/DriveFolder.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QDirIterator>
#include <QSet>
#include <algorithm>

DriveManager::DriveManager(QObject *parent): QObject(parent), m_apiClient(nullptr), m_currentDirectoryId(0), m_initialized(false), m_dbDriveDownload(nullptr), m_dbDriveUpload(nullptr)
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
    
    if (m_dbDriveUpload) {
        delete m_dbDriveUpload;
        m_dbDriveUpload = nullptr;
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
    
    // 初始化上传记录数据库
    m_dbDriveUpload = new dbDriveUpload("./SmartDesk.db");
    
    // 连接DriveApiClient的信号
    connect(m_apiClient, &DriveApiClient::fileListReceived,
            this, &DriveManager::onFileListReceived);
    connect(m_apiClient, &DriveApiClient::pathReceived,
            this, &DriveManager::onPathReceived);

    // 文件夹下载：递归遍历云端目录
    connect(m_apiClient, &DriveApiClient::directoryListReceived,
            this, &DriveManager::onFolderDownloadDirListed);
    connect(m_apiClient, &DriveApiClient::directoryListError,
            this, [this](int dirId, const QString &errorMessage) {
                Q_UNUSED(dirId);
                if (m_folderDownloadActive) {
                    failFolderDownload(errorMessage);
                } else {
                    onError(errorMessage);
                }
            });

    connect(m_apiClient, &DriveApiClient::fileUploaded,
            this, &DriveManager::onFileUploaded);

    connect(m_apiClient, &DriveApiClient::uploadProgress,
            this, &DriveManager::onFolderUploadProgress);

    connect(m_apiClient, &DriveApiClient::folderCreated,
            this, &DriveManager::onFolderCreated);

    connect(m_apiClient, &DriveApiClient::fileDownloaded,
            this, &DriveManager::onFileDownloaded);
    connect(m_apiClient, &DriveApiClient::downloadProgress,
            this, &DriveManager::onFolderDownloadProgress);

    connect(m_apiClient, &DriveApiClient::itemDeleted,
            this, &DriveManager::onItemDeleted);

    connect(m_apiClient, &DriveApiClient::driveCleared,
            this, &DriveManager::onDriveCleared);

    connect(m_apiClient, &DriveApiClient::itemRenamed,
            this, &DriveManager::onItemRenamed);

    connect(m_apiClient, &DriveApiClient::itemMoved,
            this, &DriveManager::onItemMoved);

    // 连接错误信号
    connect(m_apiClient, &DriveApiClient::fileListError,
            this, &DriveManager::onError);

    connect(m_apiClient, &DriveApiClient::fileUploadError,
            this, &DriveManager::onFileUploadError);

    connect(m_apiClient, &DriveApiClient::folderCreateError,
            this, [this](const QString &errorMessage) {
                if (m_folderUploadActive) {
                    failFolderUpload(errorMessage);
                } else {
                    onError(errorMessage);
                }
            });

    connect(m_apiClient, &DriveApiClient::fileDownloadError,
            this, &DriveManager::onFileDownloadError);

    connect(m_apiClient, &DriveApiClient::itemDeleteError,
            this, &DriveManager::onError);

    connect(m_apiClient, &DriveApiClient::driveClearError,
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

void DriveManager::uploadFile(const QString &filePath, int parentId, const QString &targetName, bool overwrite)
{
    if (!m_initialized) {
        initialize();
    }
    
    // 获取文件信息
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        // 添加上传记录
        const QString localFileName = fileInfo.fileName();
        const QString cloudFileName = targetName.isEmpty() ? localFileName : targetName;
        addUploadRecord(0, localFileName, cloudFileName, fileInfo.size(), filePath, parentId);

        // 立即通知进度0，让界面显示"上传中"进度条
        const int recordId = m_uploadRecordMap.value(filePath, -1);
        if (recordId > 0) {
            emit uploadProgress(recordId, 0);
        }
    }
    
    m_apiClient->uploadFile(filePath, parentId, targetName, overwrite);
}

void DriveManager::createFolder(const QString &folderName, int parentId)
{
    if (!m_initialized) {
        initialize();
    }

    m_apiClient->createFolder(folderName, parentId);
}

bool DriveManager::isFolderTaskActive() const
{
    return m_folderUploadActive || m_folderDownloadActive;
}

// ========== 文件夹上传（逐文件，单条记录） ==========

bool DriveManager::uploadFolder(const QString &folderPath, int parentId)
{
    if (!m_initialized) {
        initialize();
    }

    if (m_folderUploadActive || m_folderDownloadActive) {
        return false;
    }

    const QFileInfo dirInfo(folderPath);
    if (!dirInfo.isDir() || dirInfo.fileName().isEmpty()) {
        return false;
    }

    QDir rootDir(dirInfo.absoluteFilePath());
    m_folderUploadLocalRoot = dirInfo.absoluteFilePath();
    m_folderUploadFolderName = dirInfo.fileName();
    m_folderUploadParentId = parentId;
    m_folderUploadDirIds.clear();
    m_folderUploadFiles.clear();
    m_folderUploadDirsPending.clear();
    m_folderUploadTotalSize = 0;
    m_folderUploadFileIndex = 0;
    m_folderUploadDoneBytes = 0;
    m_folderUploadCurrentDir.clear();

    // 递归遍历本地文件夹，收集相对目录与文件
    QSet<QString> dirSet;
    dirSet.insert(QString()); // "" 表示顶层（云端父目录下的文件夹本身）

    QDirIterator it(dirInfo.absoluteFilePath(), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        const QString rel = rootDir.relativeFilePath(info.absoluteFilePath());
        if (info.isDir()) {
            dirSet.insert(QDir::fromNativeSeparators(rel));
        } else if (info.isFile()) {
            DriveFolderUploadFile entry;
            entry.absolutePath = QDir::fromNativeSeparators(info.absoluteFilePath());
            entry.fileName = info.fileName();
            entry.relativeDir = QDir::fromNativeSeparators(QFileInfo(rel).path());
            if (entry.relativeDir == QStringLiteral(".")) {
                entry.relativeDir.clear();
            }
            entry.size = info.size();
            m_folderUploadFiles.append(entry);
            m_folderUploadTotalSize += entry.size;
        }
    }

    // 目录按深度排序，保证父目录先于子目录创建
    QStringList dirs = dirSet.values();
    std::sort(dirs.begin(), dirs.end(), [](const QString &a, const QString &b) {
        const int depthA = a.count('/');
        const int depthB = b.count('/');
        if (depthA != depthB) {
            return depthA < depthB;
        }
        return a < b;
    });
    m_folderUploadDirsPending = dirs;

    // 只记录一条文件夹记录
    addUploadRecord(0, m_folderUploadFolderName, m_folderUploadFolderName,
                    m_folderUploadTotalSize, m_folderUploadLocalRoot, parentId);
    m_folderUploadRecordId = getRecordIdByLocalPath(m_folderUploadLocalRoot);
    m_folderUploadActive = true;

    if (m_folderUploadRecordId > 0) {
        emit uploadProgress(m_folderUploadRecordId, 0);
    }

    uploadNextFolderDirectory();
    return true;
}

void DriveManager::uploadNextFolderDirectory()
{
    if (!m_folderUploadActive) {
        return;
    }

    if (m_folderUploadDirsPending.isEmpty()) {
        // 目录链建完，开始逐个上传文件
        uploadNextFolderFile();
        return;
    }

    const QString relDir = m_folderUploadDirsPending.takeFirst();
    m_folderUploadCurrentDir = relDir;

    // 云端父目录ID：顶层目录挂在 m_folderUploadParentId 下，其余挂在已建好的父目录下
    int cloudParentId = m_folderUploadParentId;
    if (!relDir.isEmpty()) {
        const QString parentRel = QFileInfo(relDir).path() == QStringLiteral(".")
            ? QString() : QFileInfo(relDir).path();
        cloudParentId = m_folderUploadDirIds.value(parentRel, -1);
    }

    if (cloudParentId < 0) {
        failFolderUpload(QStringLiteral("云端目录尚未创建：%1").arg(relDir));
        return;
    }

    const QString folderName = relDir.isEmpty()
        ? m_folderUploadFolderName
        : QFileInfo(relDir).fileName();

    m_apiClient->createFolder(folderName, cloudParentId);
}

void DriveManager::uploadNextFolderFile()
{
    if (!m_folderUploadActive) {
        return;
    }

    if (m_folderUploadFileIndex >= m_folderUploadFiles.size()) {
        finishFolderUpload();
        return;
    }

    const DriveFolderUploadFile &entry = m_folderUploadFiles.at(m_folderUploadFileIndex);
    const int cloudParentId = m_folderUploadDirIds.value(entry.relativeDir, -1);
    if (cloudParentId < 0) {
        failFolderUpload(QStringLiteral("云端目录尚未创建：%1").arg(entry.relativeDir));
        return;
    }

    // overwrite=true：文件夹合并语义，云端同名文件直接覆盖
    m_apiClient->uploadFile(entry.absolutePath, cloudParentId, entry.fileName, true);
}

void DriveManager::onFolderUploadProgress(const QString &filePath, int fileProgress)
{
    // 文件夹上传：按所有文件的累计字节数折算总进度
    if (m_folderUploadActive && m_folderUploadFileIndex < m_folderUploadFiles.size()
        && m_folderUploadFiles.at(m_folderUploadFileIndex).absolutePath == filePath) {
        if (m_folderUploadRecordId > 0 && m_folderUploadTotalSize > 0) {
            const DriveFolderUploadFile &entry = m_folderUploadFiles.at(m_folderUploadFileIndex);
            const qint64 sentBytes = m_folderUploadDoneBytes
                + entry.size * qBound(0, fileProgress, 100) / 100;
            const int total = static_cast<int>(qBound<qint64>(
                qint64(0), sentBytes * 100 / m_folderUploadTotalSize, qint64(100)));
            emit uploadProgress(m_folderUploadRecordId, total);
        }
        return;
    }

    // 原单文件上传逻辑
    const int recordId = m_uploadRecordMap.value(filePath, -1);
    if (recordId > 0) {
        emit uploadProgress(recordId, fileProgress);
    }
}

void DriveManager::finishFolderUpload()
{
    if (!m_folderUploadActive) {
        return;
    }

    if (m_folderUploadRecordId > 0) {
        updateUploadStatus(m_folderUploadRecordId, "success");
        emit uploadProgress(m_folderUploadRecordId, 100);
    }

    m_folderUploadActive = false;

    // 刷新当前目录，展示新上传的文件夹
    getCurrentDirectoryFiles(m_currentDirectoryId);
    emit operationSuccess(m_folderUploadFolderName.isEmpty()
        ? QStringLiteral("文件夹上传成功")
        : QStringLiteral("文件夹「%1」上传成功").arg(m_folderUploadFolderName));
}

void DriveManager::failFolderUpload(const QString &errorMessage)
{
    if (m_folderUploadActive) {
        if (m_folderUploadRecordId > 0) {
            updateUploadStatus(m_folderUploadRecordId, "failed");
        }
        m_folderUploadActive = false;
    }

    emit uploadFailed(errorMessage);
    emit operationFailed(errorMessage);
}

void DriveManager::deleteItem(int itemId)
{
    if (!m_initialized) {
        initialize();
    }
    
    m_apiClient->deleteItem(itemId);
}

void DriveManager::clearDrive(int dirId)
{
    if (!m_initialized) {
        initialize();
    }

    m_apiClient->clearDrive(dirId);
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

void DriveManager::downloadFile(int fileId, const QString &savePath, bool overwrite)
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
                while (!overwrite && QFile::exists(finalPath)) {
                    QString newName;
                    if (suffix.isEmpty()) {
                        newName = QString("%1 (%2)").arg(baseName).arg(counter);
                    } else {
                        newName = QString("%1 (%2).%3").arg(baseName).arg(counter).arg(suffix);
                    }
                    finalPath = QDir(path).absoluteFilePath(newName);
                    counter++;
                }

                // 添加下载记录：本地名使用最终保存路径，云端名使用网盘原文件名。
                addDownloadRecord(fileId, QFileInfo(finalPath).fileName(), file->getName(), file->getSize(), finalPath);
                const int recordId = m_downloadRecordMap.value(fileId, -1);
                if (recordId > 0) {
                    emit downloadProgress(recordId, 0);
                }
                m_apiClient->downloadFile(fileId, finalPath, overwrite);
                break;
            }
        }
    }
    
}

void DriveManager::getPath(int dirId)
{
    if (!m_initialized) {
        initialize();
    }

    m_apiClient->getPath(dirId);
}

// ========== 文件夹下载（递归遍历 + 逐文件，单条记录） ==========

bool DriveManager::downloadFolder(int folderId, const QString &targetDir, bool overwrite)
{
    if (!m_initialized) {
        initialize();
    }

    if (m_folderUploadActive || m_folderDownloadActive) {
        return false;
    }

    // 从当前文件列表中找到文件夹信息
    QString folderName;
    for (DriveItem *item : m_currentFileList) {
        if (item->getId() == folderId && item->isFolder()) {
            folderName = item->getName();
            break;
        }
    }
    if (folderName.isEmpty()) {
        return false;
    }

    m_folderDownloadActive = true;
    // targetDir 即为最终保存目录（调用方已按文件夹名/重命名处理过路径）
    m_folderDownloadTargetRoot = QDir(targetDir).absolutePath();
    m_folderDownloadFolderName = folderName;
    m_folderDownloadOverwrite = overwrite;
    m_folderDownloadDirsPending.clear();
    m_folderDownloadFiles.clear();
    m_folderDownloadFileIndex = 0;
    m_folderDownloadTotalSize = 0;
    m_folderDownloadDoneBytes = 0;
    m_folderDownloadCurrentFileId = -1;
    m_folderDownloadCurrentSize = 0;
    m_folderDownloadCurrentSavePath.clear();
    m_folderDownloadDirsPending.append({folderId, QString()});

    // 只记录一条文件夹记录（大小等遍历完成后回填）
    addDownloadRecord(folderId, folderName, folderName, 0, m_folderDownloadTargetRoot);
    m_folderDownloadRecordId = getRecordIdBySavePath(m_folderDownloadTargetRoot);

    // 创建本地根目录
    QDir(m_folderDownloadTargetRoot).mkpath(QStringLiteral("."));

    if (m_folderDownloadRecordId > 0) {
        emit downloadProgress(m_folderDownloadRecordId, 0);
    }

    listNextDownloadDirectory();
    return true;
}

void DriveManager::listNextDownloadDirectory()
{
    if (!m_folderDownloadActive) {
        return;
    }

    if (m_folderDownloadDirsPending.isEmpty()) {
        // 遍历完成：回填记录大小，开始逐个下载文件
        if (m_folderDownloadRecordId > 0) {
            m_dbDriveDownload->updateDownloadFileSize(m_folderDownloadRecordId, m_folderDownloadTotalSize);
        }
        if (m_folderDownloadFiles.isEmpty()) {
            finishFolderDownload();
            return;
        }
        downloadNextFolderFile();
        return;
    }

    m_folderDownloadListingDir = m_folderDownloadDirsPending.takeFirst();

    // 本地目录立即创建（保证空文件夹也存在）
    QDir(QDir(m_folderDownloadTargetRoot).absoluteFilePath(
        m_folderDownloadListingDir.relativeDir.isEmpty()
            ? QStringLiteral(".") : m_folderDownloadListingDir.relativeDir)).mkpath(QStringLiteral("."));

    m_apiClient->listDirectory(m_folderDownloadListingDir.cloudId);
}

void DriveManager::onFolderDownloadDirListed(int dirId, const QJsonArray &items)
{
    Q_UNUSED(dirId);

    if (!m_folderDownloadActive) {
        return;
    }

    const QString parentRel = m_folderDownloadListingDir.relativeDir;

    for (const QJsonValue &value : items) {
        const QJsonObject obj = value.toObject();
        const bool isDir = obj[QStringLiteral("is_dir")].toBool();
        const QString name = obj[QStringLiteral("name")].toString();
        if (name.isEmpty()) {
            continue;
        }

        if (isDir) {
            DriveFolderDownloadDir child;
            child.cloudId = obj[QStringLiteral("id")].toInt();
            child.relativeDir = parentRel.isEmpty() ? name : parentRel + QStringLiteral("/") + name;
            if (child.cloudId > 0) {
                m_folderDownloadDirsPending.append(child);
            }
        } else {
            DriveFolderDownloadFile entry;
            entry.fileId = obj[QStringLiteral("id")].toInt();
            entry.fileName = name;
            entry.relativeDir = parentRel;
            entry.size = static_cast<qint64>(obj[QStringLiteral("size")].toDouble());
            if (entry.fileId > 0) {
                m_folderDownloadFiles.append(entry);
                m_folderDownloadTotalSize += entry.size;
            }
        }
    }

    listNextDownloadDirectory();
}

namespace {
// 生成不与现有文件冲突的路径（与 DriveView::uniqueLocalPath 同规则）
QString uniqueFilePath(const QString &path)
{
    QString finalPath = path;
    const QFileInfo fileInfo(path);
    const QString baseName = fileInfo.baseName();
    const QString suffix = fileInfo.completeSuffix();
    const QString dirPath = fileInfo.path();
    int counter = 1;

    while (QFile::exists(finalPath)) {
        const QString newName = suffix.isEmpty()
            ? QStringLiteral("%1 (%2)").arg(baseName).arg(counter)
            : QStringLiteral("%1 (%2).%3").arg(baseName).arg(counter).arg(suffix);
        finalPath = QDir(dirPath).absoluteFilePath(newName);
        ++counter;
    }

    return finalPath;
}
}

void DriveManager::downloadNextFolderFile()
{
    if (!m_folderDownloadActive) {
        return;
    }

    if (m_folderDownloadFileIndex >= m_folderDownloadFiles.size()) {
        finishFolderDownload();
        return;
    }

    const DriveFolderDownloadFile &entry = m_folderDownloadFiles.at(m_folderDownloadFileIndex);
    QDir targetDir(QDir(m_folderDownloadTargetRoot).absoluteFilePath(
        entry.relativeDir.isEmpty() ? QStringLiteral(".") : entry.relativeDir));
    targetDir.mkpath(QStringLiteral("."));

    // 非覆盖模式下先本地去重，再以覆盖方式交给API层，
    // 保证完成回调回报的路径与预期一致
    QString savePath = targetDir.absoluteFilePath(entry.fileName);
    if (!m_folderDownloadOverwrite && QFile::exists(savePath)) {
        savePath = uniqueFilePath(savePath);
    }

    m_folderDownloadCurrentFileId = entry.fileId;
    m_folderDownloadCurrentSize = entry.size;
    m_folderDownloadCurrentSavePath = savePath;

    m_apiClient->downloadFile(entry.fileId, savePath, true);
}

void DriveManager::onFolderDownloadProgress(int fileId, int fileProgress)
{
    // 文件夹下载：按累计字节数折算总进度
    if (m_folderDownloadActive && fileId == m_folderDownloadCurrentFileId) {
        if (m_folderDownloadRecordId > 0 && m_folderDownloadTotalSize > 0) {
            const qint64 receivedBytes = m_folderDownloadDoneBytes
                + m_folderDownloadCurrentSize * qBound(0, fileProgress, 100) / 100;
            const int total = static_cast<int>(qBound<qint64>(
                qint64(0), receivedBytes * 100 / m_folderDownloadTotalSize, qint64(100)));
            emit downloadProgress(m_folderDownloadRecordId, total);
        }
        return;
    }

    // 原单文件下载逻辑
    const int recordId = m_downloadRecordMap.value(fileId, -1);
    if (recordId > 0) {
        emit downloadProgress(recordId, fileProgress);
    }
}

void DriveManager::finishFolderDownload()
{
    if (!m_folderDownloadActive) {
        return;
    }

    if (m_folderDownloadRecordId > 0) {
        updateDownloadStatus(m_folderDownloadRecordId, "success");
        emit downloadProgress(m_folderDownloadRecordId, 100);
    }

    m_folderDownloadActive = false;

    const QString downloadedRoot = m_folderDownloadTargetRoot;
    emit fileDownloaded(downloadedRoot);
    emit operationSuccess(QStringLiteral("文件夹「%1」下载成功").arg(m_folderDownloadFolderName));
}

void DriveManager::failFolderDownload(const QString &errorMessage)
{
    if (m_folderDownloadActive) {
        if (m_folderDownloadRecordId > 0) {
            updateDownloadStatus(m_folderDownloadRecordId, "failed");
        }
        m_folderDownloadActive = false;
    }

    emit downloadFailed(errorMessage);
    emit operationFailed(errorMessage);
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


void DriveManager::onFileUploaded(const QString &filePath, const QJsonObject &fileInfo)
{
    // 文件夹上传队列：推进到下一个文件
    if (m_folderUploadActive && m_folderUploadFileIndex < m_folderUploadFiles.size()
        && m_folderUploadFiles.at(m_folderUploadFileIndex).absolutePath == filePath) {
        m_folderUploadDoneBytes += m_folderUploadFiles.at(m_folderUploadFileIndex).size;
        ++m_folderUploadFileIndex;

        if (m_folderUploadRecordId > 0 && m_folderUploadTotalSize > 0) {
            const int total = static_cast<int>(qBound<qint64>(
                qint64(0), m_folderUploadDoneBytes * 100 / m_folderUploadTotalSize, qint64(100)));
            emit uploadProgress(m_folderUploadRecordId, total);
        }

        uploadNextFolderFile();
        return;
    }

    // 原单文件上传逻辑：按本地路径精确匹配记录并更新为成功
    int fileId = fileInfo["id"].toInt();
    QString fileName = fileInfo["name"].toString();

    const int recordId = m_uploadRecordMap.value(filePath, -1);
    if (recordId > 0) {
        updateUploadResult(recordId, fileId, fileName, "success");
    }

    // 文件上传成功，刷新当前目录
    getCurrentDirectoryFiles(m_currentDirectoryId);
    emit operationSuccess("文件上传成功");
}

void DriveManager::onFolderCreated(const QJsonObject &folderInfo)
{
    // 文件夹上传队列：记录新目录ID，创建下一个目录
    if (m_folderUploadActive) {
        const int cloudId = folderInfo[QStringLiteral("id")].toInt();
        if (cloudId > 0) {
            m_folderUploadDirIds[m_folderUploadCurrentDir] = cloudId;
        } else {
            failFolderUpload(QStringLiteral("创建云端目录失败：%1").arg(m_folderUploadCurrentDir));
            return;
        }
        uploadNextFolderDirectory();
        return;
    }

    // 文件夹创建成功，刷新当前目录
    getCurrentDirectoryFiles(m_currentDirectoryId);
    emit operationSuccess("文件夹创建成功");
}

void DriveManager::onFileDownloaded(const QString &filePath)
{
    // 文件夹下载队列：推进到下一个文件
    if (m_folderDownloadActive && filePath == m_folderDownloadCurrentSavePath) {
        if (m_folderDownloadFileIndex < m_folderDownloadFiles.size()) {
            m_folderDownloadDoneBytes += m_folderDownloadFiles.at(m_folderDownloadFileIndex).size;
        }
        ++m_folderDownloadFileIndex;

        if (m_folderDownloadRecordId > 0 && m_folderDownloadTotalSize > 0) {
            const int total = static_cast<int>(qBound<qint64>(
                qint64(0), m_folderDownloadDoneBytes * 100 / m_folderDownloadTotalSize, qint64(100)));
            emit downloadProgress(m_folderDownloadRecordId, total);
        }

        downloadNextFolderFile();
        return;
    }

    // 原单文件下载逻辑：更新下载状态为成功
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

void DriveManager::onDriveCleared(const QJsonObject &result)
{
    const int deletedCount = result[QStringLiteral("deleted_count")].toInt();

    // 目录本身保留，原地刷新当前目录展示
    getCurrentDirectoryFiles(m_currentDirectoryId);
    emit operationSuccess(deletedCount > 0
        ? QStringLiteral("目录已清空（共删除 %1 项）").arg(deletedCount)
        : QStringLiteral("目录已清空"));
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
    // 检查是否是上传错误
    else if (errorMessage.contains("上传") || errorMessage.contains("upload", Qt::CaseInsensitive)) {
        // 尝试从错误消息中提取文件路径
        QString filePath;
        for (auto it = m_uploadRecordMap.begin(); it != m_uploadRecordMap.end(); ++it) {
            // 文件夹任务进行中时，不要把文件夹的记录标成失败（由文件夹流程自己管理）
            if (m_folderUploadActive && it.value() == m_folderUploadRecordId) {
                continue;
            }
            int recordId = it.value();
            // 更新所有uploading状态的上传记录为failed
            updateUploadStatus(recordId, "failed");
        }
        emit uploadFailed(errorMessage);
    }
    emit operationFailed(errorMessage);
}

// 上传失败（带文件路径）：文件夹队列内失败则中止整个文件夹任务，否则走原单文件逻辑
void DriveManager::onFileUploadError(const QString &filePath, const QString &errorMessage)
{
    if (m_folderUploadActive && m_folderUploadFileIndex < m_folderUploadFiles.size()
        && m_folderUploadFiles.at(m_folderUploadFileIndex).absolutePath == filePath) {
        failFolderUpload(QStringLiteral("上传文件「%1」失败：%2")
                             .arg(QFileInfo(filePath).fileName(), errorMessage));
        return;
    }

    // 原单文件上传失败逻辑：按本地路径精确匹配记录并标为失败
    const int recordId = m_uploadRecordMap.value(filePath, -1);
    if (recordId > 0) {
        updateUploadStatus(recordId, "failed");
    }
    emit uploadFailed(errorMessage);
    emit operationFailed(errorMessage);
}

// 下载失败（带保存路径）：文件夹队列内失败则中止整个文件夹任务，否则走原单文件逻辑
void DriveManager::onFileDownloadError(const QString &savePath, const QString &errorMessage)
{
    if (m_folderDownloadActive && savePath == m_folderDownloadCurrentSavePath) {
        failFolderDownload(QStringLiteral("下载文件「%1」失败：%2")
                               .arg(QFileInfo(savePath).fileName(), errorMessage));
        return;
    }

    // 原单文件下载失败逻辑
    emit downloadFailed(errorMessage);
    emit operationFailed(errorMessage);
}

DriveItem *DriveManager::jsonToDriveItem(const QJsonObject &json)
{
    int id = json["id"].toInt();
    QString name = json["name"].toString();
    int parentId = json["parent_id"].toInt();
    bool isDir = json["is_dir"].toBool();
    QDateTime createdAt = QDateTime::fromString(json["created_at"].toString(), Qt::ISODate).toLocalTime();
    
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

void DriveManager::addDownloadRecord(int fileId, const QString &localFileName, const QString &cloudFileName, qint64 fileSize, const QString &savePath)
{
    if (!m_dbDriveDownload) {
        return;
    }
    
    DriveDownloadRecord record;
    record.fileId = fileId;
    record.fileName = localFileName;
    record.cloudFileName = cloudFileName;
    record.fileSize = fileSize;
    record.savePath = savePath;
    record.downloadTime = QDateTime::currentDateTime();
    record.downloadStatus = "downloading"; // 初始状态为下载中
    record.fileType = localFileName.section('.', -1);
    
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
    if (!m_initialized) {
        initialize();
    }

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

// ========== 上传记录管理 ==========

void DriveManager::addUploadRecord(int fileId, const QString &localFileName, const QString &cloudFileName, qint64 fileSize, const QString &localPath, int parentId)
{
    if (!m_dbDriveUpload) {
        return;
    }
    
    DriveUploadRecord record;
    record.fileId = fileId;
    record.fileName = localFileName;
    record.cloudFileName = cloudFileName;
    record.fileSize = fileSize;
    record.localPath = localPath;
    record.uploadTime = QDateTime::currentDateTime();
    record.uploadStatus = "uploading"; // 初始状态为上传中
    record.fileType = localFileName.section('.', -1);
    record.parentId = parentId;
    
    if (m_dbDriveUpload->addUploadRecord(record)) {
        // 获取刚插入的记录ID
        int recordId = m_dbDriveUpload->getRecordIdByLocalPath(localPath);
        if (recordId > 0) {
            m_uploadRecordMap[localPath] = recordId;
        }
    }
}

QList<DriveUploadRecord> DriveManager::getUploadHistory()
{
    if (!m_initialized) {
        initialize();
    }

    if (!m_dbDriveUpload) {
        return QList<DriveUploadRecord>();
    }
    
    return m_dbDriveUpload->getUploadHistory();
}

bool DriveManager::clearUploadHistory()
{
    if (!m_dbDriveUpload) {
        return false;
    }
    
    return m_dbDriveUpload->clearUploadHistory();
}

bool DriveManager::updateUploadStatus(int recordId, const QString &status)
{
    if (!m_dbDriveUpload) {
        return false;
    }
    
    return m_dbDriveUpload->updateUploadStatus(recordId, status);
}

bool DriveManager::updateUploadResult(int recordId, int fileId, const QString &cloudFileName, const QString &status)
{
    if (!m_dbDriveUpload) {
        return false;
    }

    return m_dbDriveUpload->updateUploadResult(recordId, fileId, cloudFileName, status);
}

int DriveManager::getRecordIdByLocalPath(const QString &localPath)
{
    if (!m_dbDriveUpload) {
        return -1;
    }
    
    return m_dbDriveUpload->getRecordIdByLocalPath(localPath);
}
