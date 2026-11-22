#ifndef DRIVEMANAGER_H
#define DRIVEMANAGER_H

#include <QObject>
#include <QList>
#include <QJsonArray>
#include <QMap>
#include <QFileInfo>
#include <QDir>
#include "../../manager/include/dbDriveDownload.h"
#include "../../manager/include/dbDriveUpload.h"

class DriveApiClient;
class DriveItem;
class DriveFile;
class DriveFolder;
class dbDriveDownload;

// 前向声明
struct DriveDownloadRecord;

// 文件夹上传队列中的一个待传文件
struct DriveFolderUploadFile {
    QString absolutePath;   // 本地绝对路径
    QString fileName;       // 文件名（不含目录）
    QString relativeDir;    // 相对于所传文件夹的目录（"" 表示顶层）
    qint64 size = 0;
};

// 文件夹下载：递归遍历出的一个待下载文件
struct DriveFolderDownloadFile {
    int fileId = 0;
    QString fileName;
    QString relativeDir;    // 相对于所下文件夹的目录（"" 表示顶层）
    qint64 size = 0;
};

class DriveManager : public QObject
{
    Q_OBJECT
public:
    static DriveManager &Instance();
    ~DriveManager() override;

    // 禁止拷贝和赋值
    DriveManager(const DriveManager &) = delete;
    DriveManager &operator=(const DriveManager &) = delete;

    // 初始化方法
    void initialize();

    // 获取当前目录下的文件列表
    void getCurrentDirectoryFiles(int parentId = 0);

    // 上传文件到当前目录
    void uploadFile(const QString &filePath, int parentId = 0, const QString &targetName = QString(), bool overwrite = false);

    // 上传整个文件夹（递归逐文件上传；只记录一条文件夹记录）
    bool uploadFolder(const QString &folderPath, int parentId = 0);

    // 下载整个文件夹（递归逐文件下载到本地镜像目录；只记录一条文件夹记录）
    bool downloadFolder(int folderId, const QString &targetDir, bool overwrite = true);

    // 是否有文件夹上传/下载任务进行中
    bool isFolderTaskActive() const;

    // 创建新文件夹
    void createFolder(const QString &folderName, int parentId = 0);
    
    // 删除文件或文件夹
    void deleteItem(int itemId);
    // 清空指定目录的文件与子文件夹（目录本身保留；0 表示网盘根目录）
    void clearDrive(int dirId = 0);
    
    // 重命名文件或文件夹
    void renameItem(int itemId, const QString &newName);
    
    // 移动文件或文件夹
    void moveItem(int itemId, int newParentId);
    
    // 下载文件
    void downloadFile(int fileId, const QString &savePath, bool overwrite = false);
    
    // 获取当前目录ID
    int getCurrentDirectoryId() const;
    
    // 设置当前目录ID
    void setCurrentDirectoryId(int dirId);
    
    // 获取当前文件列表
    QList<DriveItem *> getCurrentFileList() const;
    
    // 清理当前文件列表
    void clearCurrentFileList();
    
    // 获取DriveApiClient实例（用于测试或特殊需求）
    DriveApiClient *getApiClient() const;
    
    // 获取目录路径（用于面包屑导航）
    void getPath(int dirId);
    
    // ========== 下载历史管理 ==========
    // 添加下载记录
    void addDownloadRecord(int fileId, const QString &localFileName, const QString &cloudFileName, qint64 fileSize, const QString &savePath);
    
    // 获取下载历史
    QList<DriveDownloadRecord> getDownloadHistory();
    
    // 清空下载历史
    bool clearDownloadHistory();
    
    // 更新下载状态
    bool updateDownloadStatus(int recordId, const QString &status);
    
    // 通过保存路径获取记录ID
    int getRecordIdBySavePath(const QString &savePath);
    
    // ========== 上传记录管理 ==========
    // 添加上传记录
    void addUploadRecord(int fileId, const QString &localFileName, const QString &cloudFileName, qint64 fileSize, const QString &localPath, int parentId);
    
    // 获取上传历史
    QList<DriveUploadRecord> getUploadHistory();
    
    // 清空上传历史
    bool clearUploadHistory();
    
    // 更新上传状态
    bool updateUploadStatus(int recordId, const QString &status);

    // 更新上传成功后的云端记录信息
    bool updateUploadResult(int recordId, int fileId, const QString &cloudFileName, const QString &status);
    
    // 通过本地路径获取记录ID
    int getRecordIdByLocalPath(const QString &localPath);

signals:
    // 文件列表更新信号
    void fileListUpdated(const QList<DriveItem *> &fileList);
    void floderListUpdated(const QList<DriveItem *> &fileList);

    // 目录路径获取成功信号
    void pathReceived(const QList<DriveItem *> &path);

    // 操作成功信号
    void operationSuccess(const QString &message);
    
    // 操作失败信号
    void operationFailed(const QString &errorMessage);
    
    // 文件上传进度信号
    void uploadProgress(int recordId, int progress);
    
    // 文件上传失败信号
    void uploadFailed(const QString &errorMessage);
    
    // 文件下载进度信号
    void downloadProgress(int recordId, int progress);
    
    // 文件下载完成信号
    void fileDownloaded(const QString &filePath);
    
    // 文件下载失败信号
    void downloadFailed(const QString &errorMessage);

private slots:
    // 从DriveApiClient接收文件列表
    void onFileListReceived(const QJsonArray &fileList);

    // 文件夹下载：接收递归遍历的目录内容
    void onFolderDownloadDirListed(int dirId, const QJsonArray &items);

    // 处理目录路径获取完成
    void onPathReceived(const QJsonArray &path);

    // 处理文件上传完成（filePath 用于区分是文件夹队列还是单文件上传）
    void onFileUploaded(const QString &filePath, const QJsonObject &fileInfo);

    // 处理文件夹创建完成
    void onFolderCreated(const QJsonObject &folderInfo);

    // 处理文件下载完成
    void onFileDownloaded(const QString &filePath);

    // 处理项目删除完成
    void onItemDeleted(int itemId);
    void onDriveCleared(const QJsonObject &result);

    // 处理项目重命名完成
    void onItemRenamed(const QJsonObject &itemInfo);

    // 处理项目移动完成
    void onItemMoved(const QJsonObject &itemInfo);

    // 处理错误信息
    void onError(const QString &errorMessage);

    // 上传失败（带文件路径，用于区分文件夹队列内的失败）
    void onFileUploadError(const QString &filePath, const QString &errorMessage);

    // 下载失败（带保存路径，用于区分文件夹队列内的失败）
    void onFileDownloadError(const QString &savePath, const QString &errorMessage);

private:
    explicit DriveManager(QObject *parent = nullptr);

    // 将JSON转换为DriveItem对象
    DriveItem *jsonToDriveItem(const QJsonObject &json);

    // 将DriveItem对象转换为JSON
    QJsonObject driveItemToJson(DriveItem *item);

    // ========== 文件夹上传（逐文件，单条记录） ==========
    void uploadNextFolderDirectory();           // 创建下一个云端子目录
    void uploadNextFolderFile();                // 上传下一个文件
    void onFolderUploadProgress(const QString &filePath, int fileProgress);
    void finishFolderUpload();                  // 全部完成
    void failFolderUpload(const QString &errorMessage);

    // ========== 文件夹下载（递归遍历 + 逐文件，单条记录） ==========
    void listNextDownloadDirectory();           // 列举下一个云端目录
    void downloadNextFolderFile();              // 下载下一个文件
    void onFolderDownloadProgress(int fileId, int fileProgress);
    void finishFolderDownload();                // 全部完成
    void failFolderDownload(const QString &errorMessage);

private:
    DriveApiClient *m_apiClient;
    QList<DriveItem *> m_currentFileList;
    int m_currentDirectoryId;
    bool m_initialized;

    // 下载历史管理
    dbDriveDownload *m_dbDriveDownload;
    QMap<int, int> m_downloadRecordMap; // 文件ID -> 记录ID的映射

    // 上传记录管理
    dbDriveUpload *m_dbDriveUpload;
    QMap<QString, int> m_uploadRecordMap; // 本地路径 -> 记录ID的映射

    // ========== 文件夹上传状态 ==========
    bool m_folderUploadActive = false;
    int m_folderUploadRecordId = -1;
    int m_folderUploadParentId = 0;                 // 上传到的云端父目录
    QString m_folderUploadLocalRoot;                // 本地文件夹绝对路径
    QString m_folderUploadFolderName;               // 文件夹名（云端顶层目录名）
    QString m_folderUploadCurrentDir;               // 正在创建的相对目录
    qint64 m_folderUploadTotalSize = 0;             // 所有文件总字节数
    QStringList m_folderUploadDirsPending;          // 待创建的相对目录（父目录在前）
    QMap<QString, int> m_folderUploadDirIds;        // 相对目录 -> 云端目录ID（"" 表示顶层云端父目录本身）
    QList<DriveFolderUploadFile> m_folderUploadFiles;
    int m_folderUploadFileIndex = 0;
    qint64 m_folderUploadDoneBytes = 0;             // 已完成文件的累计字节数

    // ========== 文件夹下载状态 ==========
    struct DriveFolderDownloadDir {
        int cloudId = 0;
        QString relativeDir;                        // 该目录相对顶层的位置
    };
    bool m_folderDownloadActive = false;
    int m_folderDownloadRecordId = -1;
    QString m_folderDownloadTargetRoot;             // 本地目标根目录
    QString m_folderDownloadFolderName;             // 网盘文件夹名
    bool m_folderDownloadOverwrite = true;          // 同名文件是否覆盖（合并语义）
    QList<DriveFolderDownloadDir> m_folderDownloadDirsPending; // 待遍历的云端目录
    DriveFolderDownloadDir m_folderDownloadListingDir;         // 正在列举的云端目录
    QList<DriveFolderDownloadFile> m_folderDownloadFiles;
    int m_folderDownloadFileIndex = 0;
    qint64 m_folderDownloadTotalSize = 0;
    qint64 m_folderDownloadDoneBytes = 0;
    int m_folderDownloadCurrentFileId = -1;         // 正在下载的云端文件ID
    qint64 m_folderDownloadCurrentSize = 0;         // 正在下载的文件大小
    QString m_folderDownloadCurrentSavePath;        // 正在下载的文件本地保存路径
};

#endif // DRIVEMANAGER_H
