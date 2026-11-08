#ifndef EXTERNALACTIONDISPATCHER_H
#define EXTERNALACTIONDISPATCHER_H

#include <QObject>
#include <QString>

class MainWindow;

// 统一处理来自资源管理器右键菜单（经命令行/单实例转发）的动作请求。
// 每个动作以文件/文件夹绝对路径为入参，等价于在应用内对对应对象执行操作：
//   open       - 打开文件（转 TabManager）
//   backup     - 文件备份（FileBackupWid 对话框 + 记录备份提交）
//   tag        - 备忘标签（AddTag 对话框 + 写标签数据库）
//   upload     - 上传网盘（DriveUploadHelper）
//   openfolder - 打开文件夹（文件系统面包屑定位到该目录）
class ExternalActionDispatcher : public QObject
{
    Q_OBJECT

public:
    explicit ExternalActionDispatcher(MainWindow *window, QObject *parent = nullptr);

    // action 不区分大小写，未知动作仅记录日志
    void dispatch(const QString &action, const QString &filePath) const;

private:
    void backupFile(const QString &filePath) const;
    void tagFile(const QString &filePath) const;
    void uploadFile(const QString &filePath) const;
    void openFolder(const QString &folderPath) const;

    MainWindow *m_window;
};

#endif // EXTERNALACTIONDISPATCHER_H
