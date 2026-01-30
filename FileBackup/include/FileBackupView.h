#ifndef FILEBACKUPVIEW_H
#define FILEBACKUPVIEW_H

#include <QWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QToolTip>
#include <QListView>

#include "FileRestoreWid.h"
#include "../../manager/include/dbService.h"
#include "../../manager/include/ServerManager.h"

namespace Ui {
class FileBackupView;
}

class FileBackupView : public QWidget
{
    Q_OBJECT

public:
    explicit FileBackupView(QWidget *parent = nullptr);
    ~FileBackupView();

    bool deleteBackupFile(const QString &backupFilePath);
public slots:
    void updateFileList(const QString filePath);

private slots:
    void on_backupList_itemClicked(QListWidgetItem *item);
    void on_fileListComboBox_currentIndexChanged(int index);


    void on_pushButton_clicked();

    void on_backupList_customContextMenuRequested(const QPoint &pos);

    void on_refreshBtn_clicked();

    void on_openInFileSystemBtn_clicked();

signals:
    void s_fileopen(const QString &filePath);
    void openInFileSystemRequested(const QString &filePath);

private:
    ServerManager* serverManager;
    dbService& dbservice;
    FileRestoreWid * file_restore_wid;
    Ui::FileBackupView *ui;
    QList<QString> missingFiles;
    bool exists = false;
    QString choosed_file;

    void loadFileNames();
};

#endif // FILEBACKUPVIEW_H
