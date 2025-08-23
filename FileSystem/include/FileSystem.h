#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QWidget>
#include <QFileSystemModel>
#include <QModelIndex>
#include <QListWidgetItem>
#include <QFileDialog>
#include <QStandardPaths>
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

    ~FileSystem();

private slots:
    void onItemClicked(const QModelIndex &index);
    // void on_goButton_clicked();

signals:
    void fileOpened(const QString &filePath);
    void filebackuplistOpened();
    void tagopened();
private:
    void loadFileMetadata(const QString &filePath);
    void saveExpirationDate(const QString &filePath, const QDate &expirationDate);

    ServerManager* serverManager;

    Ui::FileSystem *ui;
    QFileSystemModel *fileSystemModel;
    TagItemDelegate *tagItemdelegate;
    QFileSystemBreadcrumbBar* breadcrumb;

    QString curfilePath;
    QString currentDir;

    void changePath(const QString& path);
};

#endif // FILESYSTEM_H
