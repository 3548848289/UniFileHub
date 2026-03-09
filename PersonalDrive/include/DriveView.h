#ifndef DRIVEVIEW_H
#define DRIVEVIEW_H

#include <QWidget>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QMap>
#include "../../Resources/ThirdParty/QFileSystemBreadcrumbBar/QFileSystemBreadcrumbBar.h"

namespace Ui {
class DriveView;
}

class DriveManager;
class DriveItem;

class DriveView : public QWidget
{
    Q_OBJECT

public:
    explicit DriveView(QWidget *parent = nullptr);
    ~DriveView();

private slots:
    void on_PushFileBtn_clicked();

    void onItemDoubleClicked(const QModelIndex &index);

    // 从DriveManager接收文件列表
    void onFileListUpdated(const QList<DriveItem *> &fileList);

    // 从DriveManager接收目录路径（用于面包屑）
    void onPathReceived(const QList<DriveItem *> &path);

    // 处理操作成功
    void onOperationSuccess(const QString &message);

    // 处理操作失败
    void onOperationFailed(const QString &errorMessage);

    // 处理委托中的按钮点击
    void onActionClicked(int row, int action);

    void on_RefreshBtn_clicked();

    void on_NewFloderBtn_clicked();
    
    // 历史记录相关
    void onClearHistoryBtnClicked();
    void onDownloadSuccess(const QString &filePath);
    void onDownloadFailed(const QString &errorMessage);
    void onUploadSuccess(const QString &message);
    void onUploadFailed(const QString &errorMessage);

private:
    QFileSystemBreadcrumbBar* breadcrumb;
    QList<BreadcrumbNode*> m_breadcrumbPath;

    Ui::DriveView *ui;
    QStandardItemModel * m_model;
    QStandardItemModel * m_downloadHistoryModel;
    QStandardItemModel * m_uploadHistoryModel;
    DriveManager *m_driveManager;
    int m_currentDirId;

    void loadFileList(int parentId);
    void updateFileList(const QList<DriveItem *> &fileList);
    void buildBreadcrumbPath();
    void loadDownloadHistory();
    void loadUploadHistory();
};

#endif // DRIVEVIEW_H
