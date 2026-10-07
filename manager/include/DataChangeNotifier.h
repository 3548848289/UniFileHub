#ifndef DATACHANGENOTIFIER_H
#define DATACHANGENOTIFIER_H

#include <QObject>
#include <QString>

// 跨页面数据变更通知：写入侧（文件系统里的备份/标签操作等）发出信号，
// 展示侧（文件备份、备忘标签等页面）连接后自动刷新列表，免去手动点刷新按钮。
class DataChangeNotifier : public QObject
{
    Q_OBJECT
public:
    static DataChangeNotifier &Instance();

    // 供写入侧发出信号的辅助方法
    void notifyBackupRecordAdded(const QString &filePath);
    void notifyTagChanged(const QString &filePath);

signals:
    // 新增了某文件的备份记录（参数为源文件路径）
    void backupRecordAdded(const QString &filePath);

    // 标签数据发生变化：新增/修改/删除（参数为相关文件路径）
    void tagChanged(const QString &filePath);
};

#endif // DATACHANGENOTIFIER_H
