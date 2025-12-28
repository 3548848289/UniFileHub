#ifndef PERSONALDRIVEVIEW_H
#define PERSONALDRIVEVIEW_H

#include <QWidget>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include "../../Resources/ThirdParty/QFileSystemBreadcrumbBar/QFileSystemBreadcrumbBar.h"

namespace Ui {
class PersonalDriveView;
}

class DriveManager;
class DriveItem;

class PersonalDriveView : public QWidget
{
    Q_OBJECT

public:
    explicit PersonalDriveView(QWidget *parent = nullptr);
    ~PersonalDriveView();

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

    void on_pushButton_clicked();

private:
    QFileSystemBreadcrumbBar* breadcrumb;
    QList<BreadcrumbNode*> m_breadcrumbPath;

    Ui::PersonalDriveView *ui;
    QStandardItemModel * m_model;
    DriveManager *m_driveManager;
    int m_currentDirId;

    void loadFileList(int parentId);
    void updateFileList(const QList<DriveItem *> &fileList);
    void buildBreadcrumbPath();
};

#endif // PERSONALDRIVEVIEW_H
