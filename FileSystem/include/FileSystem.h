#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QWidget>
#include <QFileSystemModel>
#include <QModelIndex>
#include <QListWidgetItem>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDirIterator>
#include <QDropEvent>
#include "TagItemDelegate.h"
#include "../../manager/include/dbService.h"
#include "../../manager/include/ServerManager.h"
#include "../../Setting/include/SettingManager.h"
#include "../../Resources/ThirdParty/QFileSystemBreadcrumbBar/QFileSystemBreadcrumbBar.h"
namespace Ui {
class FileSystem;
}

class FileSystem : public QWidget
{
    Q_OBJECT

public:
    explicit FileSystem(QWidget *parent = nullptr);
    void changePath(const QString& path);
    QString currentDirectory() const { return currentDir; }  // 供退出时记录上次目录

    ~FileSystem();

public slots:
    void refreshTagCache(); // 标签数据变化后清空 delegate 缓存并刷新列表

private slots:
    void onItemClicked(const QModelIndex &index);
    void onItemDoubleClicked(const QModelIndex &index);
    // void on_goButton_clicked();
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

signals:
    void fileOpened(const QString &filePath);
    void deleteFileRequested(const QString &filePath);
    void filebackuplistOpened();
    void tagopened();
    void fileSelectedByKeyboard(const QString &filePath);
    void uploadToDriveRequested(const QString &filePath);
    void openTerminalRequested(const QString &directoryPath);
    void driveFileDropped(int fileId, const QString &fileName, const QString &targetDirectory);
    // 通过面包屑输入路径导航到某个文件夹时发出
    void folderNavigated(const QString &folderPath);
private:
    void loadFileMetadata(const QString &filePath);
    void saveExpirationDate(const QString &filePath, const QDate &expirationDate);
    bool eventFilter(QObject *watched, QEvent *event);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    ServerManager* serverManager;

    Ui::FileSystem *ui;
    QFileSystemModel *fileSystemModel;
    TagItemDelegate *tagItemdelegate;
    QFileSystemBreadcrumbBar* breadcrumb;

    QString curfilePath;
    QString currentDir;
    bool isMouseClick;
    bool isKeyboardSelection;

};

#endif // FILESYSTEM_H
