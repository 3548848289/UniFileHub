#ifndef DRIVEVIEW_H
#define DRIVEVIEW_H

#include <QWidget>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QMap>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTableView>
#include "../../Resources/ThirdParty/QFileSystemBreadcrumbBar/QFileSystemBreadcrumbBar.h"

namespace Ui {
class DriveView;
}

class DriveManager;
class DriveItem;
class QLabel;

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
    void on_ClearDriveBtn_clicked();

    void on_NewFloderBtn_clicked();
    
    // 历史记录相关
    void onClearHistoryBtnClicked();
    void onDownloadSuccess(const QString &filePath);
    void onDownloadFailed(const QString &errorMessage);
    void onDownloadProgress(int recordId, int progress);
    void onUploadSuccess(const QString &message);
    void onUploadFailed(const QString &errorMessage);
    
    // 下载历史 Delegate 按钮点击
    void onDownloadHistoryOpenLocationClicked(int row);
    void onDownloadHistoryPreviewClicked(int row);
    
    // 上传历史 Delegate 按钮点击
    void onUploadHistoryOpenLocationClicked(int row);
    void onUploadHistoryPreviewClicked(int row);

private:
    QFileSystemBreadcrumbBar* breadcrumb;
    QList<BreadcrumbNode*> m_breadcrumbPath;

    Ui::DriveView *ui;
    QStandardItemModel * m_model;
    QStandardItemModel * m_downloadHistoryModel;
    QStandardItemModel * m_uploadHistoryModel;
    QWidget *m_statusPopup;
    QLabel *m_statusLabel;
    DriveManager *m_driveManager;
    int m_currentDirId;
    bool m_isClearDriveRequesting = false;
    bool m_layoutUpdatePending = false;
    int m_statusMessageSerial = 0;

    void loadFileList(int parentId);
    void updateFileList(const QList<DriveItem *> &fileList);
    void buildBreadcrumbPath();
    void loadDownloadHistory();
    void loadUploadHistory();
    void showInlineMessage(const QString &message, bool isError = false);
    void updateDownloadHistoryProgress(int recordId, int progress);
    QString ensureDownloadDirectory();
    void polishTableView(QTableView *tableView);
    void scheduleTableLayoutUpdate();
    void applyDriveTableLayout();
    void applyHistoryTableLayout(QTableView *tableView);
    void positionStatusPopup();

    QString formatFileSize(qint64 bytes);
protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // DRIVEVIEW_H
