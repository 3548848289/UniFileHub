#ifndef DRIVEMANAGER_H
#define DRIVEMANAGER_H

#include <QObject>
#include <QList>
#include <QJsonArray>
#include <QMap>
#include <QFileInfo>
#include <QDir>
#include "../../manager/include/dbDriveDownload.h"

class DriveApiClient;
class DriveItem;
class DriveFile;
class DriveFolder;
class dbDriveDownload;

// 前向声明
struct DriveDownloadRecord;

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
    void uploadFile(const QString &filePath, int parentId = 0);
    
    // 创建新文件夹
    void createFolder(const QString &folderName, int parentId = 0);
    
    // 删除文件或文件夹
    void deleteItem(int itemId);
    
    // 重命名文件或文件夹
    void renameItem(int itemId, const QString &newName);
    
    // 移动文件或文件夹
    void moveItem(int itemId, int newParentId);
    
    // 下载文件
    void downloadFile(int fileId, const QString &savePath);
    
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
    void addDownloadRecord(int fileId, const QString &fileName, qint64 fileSize, const QString &savePath);
    
    // 获取下载历史
    QList<DriveDownloadRecord> getDownloadHistory();
    
    // 清空下载历史
    bool clearDownloadHistory();
    
    // 更新下载状态
    bool updateDownloadStatus(int recordId, const QString &status);
    
    // 通过保存路径获取记录ID
    int getRecordIdBySavePath(const QString &savePath);

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
    void uploadProgress(int progress);
    
    // 文件下载进度信号
    void downloadProgress(int progress);
    
    // 文件下载完成信号
    void fileDownloaded(const QString &filePath);
    
    // 文件下载失败信号
    void downloadFailed(const QString &errorMessage);

private slots:
    // 从DriveApiClient接收文件列表
    void onFileListReceived(const QJsonArray &fileList);
    
    // 处理目录路径获取完成
    void onPathReceived(const QJsonArray &path);
    
    // 处理文件上传完成
    void onFileUploaded(const QJsonObject &fileInfo);
    
    // 处理文件夹创建完成
    void onFolderCreated(const QJsonObject &folderInfo);
    
    // 处理文件下载完成
    void onFileDownloaded(const QString &filePath);
    
    // 处理项目删除完成
    void onItemDeleted(int itemId);
    
    // 处理项目重命名完成
    void onItemRenamed(const QJsonObject &itemInfo);
    
    // 处理项目移动完成
    void onItemMoved(const QJsonObject &itemInfo);
    
    // 处理错误信息
    void onError(const QString &errorMessage);

private:
    explicit DriveManager(QObject *parent = nullptr);
    
    // 将JSON转换为DriveItem对象
    DriveItem *jsonToDriveItem(const QJsonObject &json);
    
    // 将DriveItem对象转换为JSON
    QJsonObject driveItemToJson(DriveItem *item);

private:
    DriveApiClient *m_apiClient;
    QList<DriveItem *> m_currentFileList;
    int m_currentDirectoryId;
    bool m_initialized;
    
    // 下载历史管理
    dbDriveDownload *m_dbDriveDownload;
    QMap<int, int> m_downloadRecordMap; // 文件ID -> 记录ID的映射
};

#endif // DRIVEMANAGER_H
