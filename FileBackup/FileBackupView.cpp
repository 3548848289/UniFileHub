#include "include/FileBackupView.h"
#include "ui/ui_FileBackupView.h"
#include "../PersonalDrive/include/DriveUploadHelper.h"
#include "../manager/include/DataChangeNotifier.h"

FileBackupView::FileBackupView(QWidget *parent) : QWidget(parent), ui(new Ui::FileBackupView)
    , serverManager(ServerManager::instance()) , dbservice(dbService::instance("./SmartDesk.db")) {
    ui->setupUi(this);
    m_messagePopup = new InlineMessagePopup(this);
    m_messagePopup->setPanelWidget(this); // 弹窗显示在面板内部顶部居中
    ui->backupList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->fileListComboBox->setMinimumContentsLength(20);
    ui->fileListComboBox->setContextMenuPolicy(Qt::CustomContextMenu);

    // 为fileListComboBox添加右键菜单
    connect(ui->fileListComboBox, &QWidget::customContextMenuRequested, this, [=](const QPoint &pos) {
        QMenu contextMenu(this);

        // 添加菜单项（调整顺序）
        QAction *changePathAction = contextMenu.addAction("更换文件路径");
        QAction *deleteRecordAction = contextMenu.addAction("删除文件记录");
        QAction *openInFileSystemAction = contextMenu.addAction("在文件系统打开");
        QAction *openFileLocationAction = contextMenu.addAction("打开文件所在位置");
        QAction *uploadToDriveAction = contextMenu.addAction("上传到网盘");

        // 处理菜单项点击
        connect(changePathAction, &QAction::triggered, this, [=]() {
            int index = ui->fileListComboBox->currentIndex();
            if (index != -1) {
                choosed_file = ui->fileListComboBox->currentText();
                QString newFilePath = QFileDialog::getOpenFileName(this, tr("选择新的文件路径"), "", tr("所有文件 (*)"));
                if (!newFilePath.isEmpty() && newFilePath != choosed_file) {
                    if (dbservice.dbBackup().updateSubmissions(choosed_file, newFilePath)) {
                        ui->fileListComboBox->setItemData(index, newFilePath);
                        ui->fileListComboBox->setItemText(index, QFileInfo(newFilePath).fileName());
                    } else
                        m_messagePopup->showMessage(tr("更新文件路径失败。"), true);
                }
            }
        });

        connect(deleteRecordAction, &QAction::triggered, this, [=]() {
            int index = ui->fileListComboBox->currentIndex();
            if (index != -1) {
                QString currentFilePath = ui->fileListComboBox->currentData().toString();
                if (!currentFilePath.isEmpty()) {
                    QList<QString> backupFileNames = dbservice.dbBackup().getBackupFileList(currentFilePath);
                    if (dbservice.dbBackup().deleteAll(currentFilePath)) {
                        // 删除磁盘上的所有备份文件
                        for (const QString &backupFile : backupFileNames) {
                            deleteBackupFile(backupFile);  // 删除实际文件
                        }

                        ui->fileListComboBox->removeItem(index);
                    } else {
                        m_messagePopup->showMessage(tr("从数据库中删除文件记录失败。"), true);
                    }

                    if (ui->fileListComboBox->count() == 0)
                        ui->backupList->clear();
                }
            }
        });

        connect(openInFileSystemAction, &QAction::triggered, this, [=]() {
            int index = ui->fileListComboBox->currentIndex();
            if (index != -1) {
                QString filePath = ui->fileListComboBox->currentData().toString();
                if (!filePath.isEmpty()) {
                    emit openInFileSystemRequested(filePath);
                }
            }
        });

        connect(openFileLocationAction, &QAction::triggered, this, [=]() {
            int index = ui->fileListComboBox->currentIndex();
            if (index != -1) {
                QString filePath = ui->fileListComboBox->currentData().toString();
                if (!filePath.isEmpty()) {
                    QFileInfo fileInfo(filePath);
                    QString folderPath = fileInfo.absolutePath();
                    QUrl folderUrl = QUrl::fromLocalFile(folderPath);
                    QDesktopServices::openUrl(folderUrl);
                }
            }
        });

        connect(uploadToDriveAction, &QAction::triggered, this, [=]() {
            int index = ui->fileListComboBox->currentIndex();
            if (index != -1) {
                uploadFileToDrive(ui->fileListComboBox->currentData().toString());
            }
        });

        // 显示菜单
        contextMenu.exec(ui->fileListComboBox->mapToGlobal(pos));
    });

    connect(serverManager, &ServerManager::onFilesListUpdated, this, &FileBackupView::updateFileList);

    // 文件系统里新增备份记录后自动刷新，不用手动点刷新按钮
    connect(&DataChangeNotifier::Instance(), &DataChangeNotifier::backupRecordAdded,
            this, &FileBackupView::onBackupRecordAdded);

    connect(serverManager, &ServerManager::returnStatus, this, [this](bool exists) {
        this->exists = exists;
    });
    loadFileNames();
}

FileBackupView::~FileBackupView()
{
    delete ui;
}

void FileBackupView::loadFileNames()
{
    ui->fileListComboBox->clear();
    ui->backupList->clear();
    QList<QString> fileNames = dbservice.dbBackup().getAllFileNames();
    for (const QString &filePath : fileNames) {
        if (QFile::exists(filePath)) {
            ui->fileListComboBox->addItem(filePath, filePath);
        } else {
            QIcon missingIcon = QIcon::fromTheme(
                "dialog-warning", QIcon(":/usedimage/tips.png"));
            ui->fileListComboBox->addItem(missingIcon, filePath, filePath);
            int itemIndex = ui->fileListComboBox->count() - 1;
            ui->fileListComboBox->setItemData(itemIndex, "missing", Qt::UserRole);
        }
    }
}

void FileBackupView::updateFileList(const QString filepath)
{
    int index = ui->fileListComboBox->findText(filepath);
    if (index != -1)
        ui->fileListComboBox->setCurrentIndex(index);
    else
        qDebug() << "File not found in the combobox.";
}

void FileBackupView::on_fileListComboBox_currentIndexChanged(int index) {
    if (index == -1)
        return;

    if (ui->fileListComboBox->itemData(index) == "missing") {
        m_messagePopup->showMessage("您选择的文件已经缺失，请更改源文件路径或选择其他文件。", true);
    }
    choosed_file = ui->fileListComboBox->currentText();
    ui->fileListComboBox->setToolTip(choosed_file);
    fillBackupList(choosed_file);
}

void FileBackupView::fillBackupList(const QString &filePath) {
    QList<QString> backupFileNames = dbservice.dbBackup().getBackupFileNames(filePath);
    ui->backupList->clear();

    QIcon missingIcon = QIcon::fromTheme("dialog-warning", QIcon(":/usedimage/tips.png"));

    for (QString &backupFileName : backupFileNames) {

        QFileInfo fileInfo(backupFileName);
        QString fileNameOnly = fileInfo.fileName();
        serverManager->checkFileExists(fileNameOnly);
        QListWidgetItem *item = new QListWidgetItem(fileNameOnly);

        bool fileMissing = false;

        if (!fileInfo.exists()) {
            item->setIcon(missingIcon);
            fileMissing = true;
        }
        item->setData(Qt::UserRole, backupFileName);
        item->setData(Qt::UserRole + 1, fileMissing);
        item->setToolTip(fileMissing ? "文件已缺失" : backupFileName);
        ui->backupList->addItem(item);
    }
}

// 文件系统新增备份记录后自动刷新：重填文件下拉框并保持当前选中，
// 若新增记录正好属于当前选中文件，其备份列表会同步显示新备份。
void FileBackupView::onBackupRecordAdded(const QString &filePath)
{
    const QString previousFile = ui->fileListComboBox->currentText();
    const bool hadSelection = ui->fileListComboBox->currentIndex() != -1;

    {
        // 阻塞信号，避免重填过程中触发 currentIndexChanged 把备份列表换成别的文件
        const QSignalBlocker blocker(ui->fileListComboBox);
        loadFileNames();

        int index = -1;
        if (hadSelection) {
            index = ui->fileListComboBox->findText(previousFile);
        }
        if (index < 0) {
            index = ui->fileListComboBox->findText(filePath);
        }
        if (index < 0 && ui->fileListComboBox->count() > 0) {
            index = 0;
        }
        if (index >= 0) {
            ui->fileListComboBox->setCurrentIndex(index);
        }
    }

    choosed_file = ui->fileListComboBox->currentText();
    ui->fileListComboBox->setToolTip(choosed_file);
    fillBackupList(choosed_file);
}


void FileBackupView::on_backupList_itemClicked(QListWidgetItem *item)
{
    QString file = item->data(Qt::UserRole).toString();
    if (!file.isEmpty())
        emit s_fileopen(file);
}




void FileBackupView::on_backupList_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = ui->backupList->itemAt(pos);
    if (item) {
        QString file = item->data(Qt::UserRole).toString();
        // 如果文件路径为空或文件不存在，禁用 "预览" 和 "还原"，并提供 "更换路径" 和 "删除" 功能
        qDebug() << "FileBackupView" << file << exists;
        bool isMissingFile = file.isEmpty() || !QFile::exists(file);

        QMenu contextMenu(this);
        QAction *actionEdit = contextMenu.addAction("预览");
        QAction *actionRestore = contextMenu.addAction("详情");
        QAction *actionReplace = contextMenu.addAction("更换路径");
        QAction *actionCommit = contextMenu.addAction("上传云端");
        QAction *actionUploadDrive = contextMenu.addAction("上传到网盘");
        QAction *actionRecover = contextMenu.addAction("从云端恢复");
        QAction *actionDelete = contextMenu.addAction("删除记录");

        if(exists)
            actionRecover->setEnabled(true);
        else
            actionRecover->setEnabled(false);

        if (isMissingFile) {
            actionEdit->setEnabled(false);
            // actionRestore->setEnabled(false);
            actionCommit->setEnabled(false);
            actionUploadDrive->setEnabled(false);
        } else {
            connect(actionEdit, &QAction::triggered, this, [this, item, file]() {
                emit s_fileopen(file);
            });

        }
        connect(actionRestore, &QAction::triggered, this, [this, item, file, isMissingFile]() {
            file_restore_wid = new FileRestoreWid(file, isMissingFile, this);
            file_restore_wid->exec();
        });
        connect(actionReplace, &QAction::triggered, this, [this, item, file]() {
            QString newFilePath = QFileDialog::getOpenFileName(this, tr("选择文件"), "", tr("所有文件 (*)"));
            if (!newFilePath.isEmpty()) {
                dbservice.dbBackup().updateFilePath(file, newFilePath);
                item->setData(Qt::UserRole, newFilePath);
                item->setText(QFileInfo(newFilePath).fileName());
            }
        });

        connect(actionCommit, &QAction::triggered, this, [this, item, file]() {
            serverManager->commitFile(file);
        });

        connect(actionUploadDrive, &QAction::triggered, this, [this, file]() {
            uploadFileToDrive(file);
        });

        connect(actionRecover, &QAction::triggered, this, [this, item, file]() {
            serverManager->downloadFile(file);

        });

        connect(actionDelete, &QAction::triggered, this, [this, item, file]() {
            if (dbservice.dbBackup().deleteBackupRecord(file)) {
                if (deleteBackupFile(file))
                    delete item;
                else
                    qDebug() << "Failed to delete backup file:" << file;
            }
        });
        contextMenu.exec(ui->backupList->mapToGlobal(pos));
    }
}


void FileBackupView::uploadFileToDrive(const QString &filePath)
{
    if (!QFileInfo(filePath).isFile()) {
        m_messagePopup->showMessage("文件不存在或已缺失，无法上传到网盘。", true);
        return;
    }

    QString uploadError;
    if (DriveUploadHelper::uploadToCurrentDirectory(this, filePath, &uploadError)) {
        m_messagePopup->showMessage(QString("已开始上传到网盘（网盘当前位置）：%1").arg(QFileInfo(filePath).fileName()));
    } else if (!uploadError.isEmpty()) {
        m_messagePopup->showMessage(uploadError, true);
    }
}


bool FileBackupView::deleteBackupFile(const QString &backupFilePath) {
    QFile file(backupFilePath);
    if (file.exists()) {
        if (file.remove()) {
            return true;
        } else {
            qDebug() << "Failed to delete file:" << backupFilePath;
            return false;
        }
    } else {
        qDebug() << "File does not exist:" << backupFilePath;
        return false;
    }
}


void FileBackupView::on_refreshBtn_clicked()
{
    loadFileNames();
}

