#ifndef DRIVEAPICLIENT_H
#define DRIVEAPICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

class DriveApiClient : public QObject
{
    Q_OBJECT
public:
    explicit DriveApiClient(QObject *parent = nullptr);
    ~DriveApiClient();

    // 文件列表获取
    void getFileList(int parentId = 0);

    // 文件上传
    void uploadFile(const QString &filePath, int parentId = 0);
    
    // 文件下载
    void downloadFile(int fileId, const QString &savePath);
    
    // 创建文件夹
    void createFolder(const QString &folderName, int parentId = 0);
    
    // 删除文件/文件夹
    void deleteItem(int itemId);
    void clearDrive();
    
    // 重命名文件/文件夹
    void renameItem(int itemId, const QString &newName);
    
    // 移动文件/文件夹
    void moveItem(int itemId, int newParentId);

    // 获取目录路径（用于面包屑导航）
    void getPath(int dirId);
    
    // 更新服务器IP缓存
    void updateServerIpCache();

signals:
    // 信号：文件列表获取成功
    void fileListReceived(const QJsonArray &fileList);


    // 信号：目录路径获取成功
    void pathReceived(const QJsonArray &path);

    // 信号：文件列表获取失败
    void fileListError(const QString &errorMessage);
    
    // 信号：文件上传成功
    void fileUploaded(const QJsonObject &fileInfo);
    
    // 信号：文件上传失败
    void fileUploadError(const QString &errorMessage);
    
    // 信号：文件下载成功
    void fileDownloaded(const QString &filePath);
    
    // 信号：文件下载失败
    void fileDownloadError(const QString &errorMessage);
    
    // 信号：文件夹创建成功
    void folderCreated(const QJsonObject &folderInfo);
    
    // 信号：文件夹创建失败
    void folderCreateError(const QString &errorMessage);
    
    // 信号：文件/文件夹删除成功
    void itemDeleted(int itemId);
    
    // 信号：文件/文件夹删除失败
    void itemDeleteError(const QString &errorMessage);
    void driveCleared(const QJsonObject &result);
    void driveClearError(const QString &errorMessage);
    
    // 信号：文件/文件夹重命名成功
    void itemRenamed(const QJsonObject &itemInfo);
    
    // 信号：文件/文件夹重命名失败
    void itemRenameError(const QString &errorMessage);
    
    // 信号：文件/文件夹移动成功
    void itemMoved(const QJsonObject &itemInfo);
    
    // 信号：文件/文件夹移动失败
    void itemMoveError(const QString &errorMessage);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_serverIp; // 服务器IP缓存
    QString getToken();
    QNetworkRequest createRequest(const QUrl &url);
};

#endif // DRIVEAPICLIENT_H
