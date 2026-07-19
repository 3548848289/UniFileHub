#include "include/DriveView.h"
#include "ui_DriveView.h"
#include "include/DriveManager.h"
#include "include/DriveItem.h"
#include "include/DriveFile.h"
#include "include/DriveFolder.h"
#include "include/DriveViewDelegate.h"
#include "include/HistoryDelegate.h"
#include "../../Setting/include/SettingManager.h"
#include "../../main/include/TabFactory.h"
#include "../../main/include/TabManager.h"
#include "../../manager/include/FileLocationHelper.h"
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QInputDialog>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QLabel>
#include <QTimer>
#include <QHeaderView>
#include <QScrollBar>
#include <QResizeEvent>
#include <QApplication>
#include <QFrame>
#include <QShowEvent>
#include <QTabWidget>

DriveView::DriveView(QWidget *parent): QWidget(parent), ui(new Ui::DriveView), m_statusLabel(nullptr), m_currentDirId(0)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("Personal Drive"));

    // ===== 1. Breadcrumb =====
    breadcrumb = new QFileSystemBreadcrumbBar();
    breadcrumb->setAllowEditMode(false);
    breadcrumb->setBreadcrumbPath({new BreadcrumbNode("网盘")});

    ui->verticalLayout->insertWidget(0, breadcrumb);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_statusLabel->setVisible(false);

    connect(breadcrumb, &QFileSystemBreadcrumbBar::pathClicked,
            this, [this](int clickedIndex, const QString&) {
                if (clickedIndex < 0) return;

                BreadcrumbNode* node = breadcrumb->bar()->path().at(clickedIndex);
                if (!node) return;

                loadFileList(node->data.toInt());
            });

    // ===== 2. Model =====
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({
        tr("名称"), tr("大小"), tr("上传时间"), tr("操作")
    });

    // ===== 3. TableView =====
    ui->tableView->setModel(m_model);
    ui->tableView->setMouseTracking(true);          // ⭐ 必须
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->verticalHeader()->setVisible(false);
    polishTableView(ui->tableView);
    
    // 启用拖拽功能
    ui->tableView->setAcceptDrops(true);
    ui->tableView->setDropIndicatorShown(true);
    ui->tableView->setDragDropMode(QAbstractItemView::DropOnly);
    
    // 为tableView安装事件过滤器
    ui->tableView->installEventFilter(this);


    // ===== 4. Delegate =====
    auto delegate = new DriveViewDelegate(this);
    ui->tableView->setItemDelegate(delegate);

    connect(ui->tableView, &QTableView::entered,
            this, [=](const QModelIndex &index) {
                delegate->setHoverRow(index.row());
                ui->tableView->viewport()->update();
            });

    connect(ui->tableView, &QTableView::viewportEntered,
            this, [=]() {
                delegate->setHoverRow(-1);
                ui->tableView->viewport()->update();
            });

    connect(delegate, &DriveViewDelegate::actionClicked,
            this, &DriveView::onActionClicked);

    // ===== 5. View 行为 =====
    connect(ui->tableView, &QTableView::doubleClicked,
            this, &DriveView::onItemDoubleClicked);

    // ===== 6. DriveManager =====
    m_driveManager = &DriveManager::Instance();
    m_driveManager->initialize();

    connect(m_driveManager, &DriveManager::fileListUpdated,
            this, &DriveView::onFileListUpdated);
    connect(m_driveManager, &DriveManager::pathReceived,
            this, &DriveView::onPathReceived);
    connect(m_driveManager, &DriveManager::operationSuccess,
            this, &DriveView::onOperationSuccess);
    connect(m_driveManager, &DriveManager::operationFailed,
            this, &DriveView::onOperationFailed);
    connect(m_driveManager, &DriveManager::fileDownloaded,
            this, &DriveView::onDownloadSuccess);
    connect(m_driveManager, &DriveManager::downloadFailed,
            this, &DriveView::onDownloadFailed);
    connect(m_driveManager, &DriveManager::operationSuccess,
            this, &DriveView::onUploadSuccess);
    connect(m_driveManager, &DriveManager::uploadFailed,
            this, &DriveView::onUploadFailed);
    
    // ===== 7. Download History =====
    m_downloadHistoryModel = new QStandardItemModel(this);
    m_downloadHistoryModel->setHorizontalHeaderLabels({
        tr("文件名"), tr("大小"), tr("下载时间"), tr("保存路径"), tr("状态"), tr("操作")
    });
    
    m_uploadHistoryModel = new QStandardItemModel(this);
    m_uploadHistoryModel->setHorizontalHeaderLabels({
        tr("文件名"), tr("大小"), tr("上传时间"), tr("本地路径"), tr("状态"), tr("操作")
    });
    
    ui->downloadHistoryTableView->setModel(m_downloadHistoryModel);
    ui->downloadHistoryTableView->horizontalHeader()->setStretchLastSection(false);
    ui->downloadHistoryTableView->verticalHeader()->setVisible(false);
    ui->downloadHistoryTableView->setMouseTracking(true);
    ui->downloadHistoryTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->downloadHistoryTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->downloadHistoryTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    polishTableView(ui->downloadHistoryTableView);
    
    // 为下载历史添加 Delegate
    auto downloadHistoryDelegate = new HistoryDelegate(this);
    ui->downloadHistoryTableView->setItemDelegate(downloadHistoryDelegate);
    
    connect(ui->downloadHistoryTableView, &QTableView::entered,
            this, [=](const QModelIndex &index) {
                downloadHistoryDelegate->setHoverRow(index.row());
                ui->downloadHistoryTableView->viewport()->update();
            });
    
    connect(ui->downloadHistoryTableView, &QTableView::viewportEntered,
            this, [=]() {
                downloadHistoryDelegate->setHoverRow(-1);
                ui->downloadHistoryTableView->viewport()->update();
            });
    
    connect(downloadHistoryDelegate, &HistoryDelegate::openLocationClicked,
            this, &DriveView::onDownloadHistoryOpenLocationClicked);
    
    connect(downloadHistoryDelegate, &HistoryDelegate::previewClicked,
            this, &DriveView::onDownloadHistoryPreviewClicked);
    
    ui->uploadHistoryTableView->setModel(m_uploadHistoryModel);
    ui->uploadHistoryTableView->horizontalHeader()->setStretchLastSection(false);
    ui->uploadHistoryTableView->verticalHeader()->setVisible(false);
    ui->uploadHistoryTableView->setMouseTracking(true);
    ui->uploadHistoryTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->uploadHistoryTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->uploadHistoryTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    polishTableView(ui->uploadHistoryTableView);
    
    // 为上传历史添加 Delegate
    auto uploadHistoryDelegate = new HistoryDelegate(this);
    ui->uploadHistoryTableView->setItemDelegate(uploadHistoryDelegate);
    
    connect(ui->uploadHistoryTableView, &QTableView::entered,
            this, [=](const QModelIndex &index) {
                uploadHistoryDelegate->setHoverRow(index.row());
                ui->uploadHistoryTableView->viewport()->update();
            });
    
    connect(ui->uploadHistoryTableView, &QTableView::viewportEntered,
            this, [=]() {
                uploadHistoryDelegate->setHoverRow(-1);
                ui->uploadHistoryTableView->viewport()->update();
            });
    
    connect(uploadHistoryDelegate, &HistoryDelegate::openLocationClicked,
            this, &DriveView::onUploadHistoryOpenLocationClicked);
    
    connect(uploadHistoryDelegate, &HistoryDelegate::previewClicked,
            this, &DriveView::onUploadHistoryPreviewClicked);
    
    // 设置默认分割比例
    ui->splitter->setSizes({300, 150});
    
    // 加载下载历史和上传历史
    loadDownloadHistory();
    loadUploadHistory();

    
    // 连接清空历史按钮
    connect(ui->clearHistoryBtn, &QPushButton::clicked, this, &DriveView::onClearHistoryBtnClicked);
    connect(ui->historyTabWidget, &QTabWidget::currentChanged, this, [this]() {
        scheduleTableLayoutUpdate();
    });

    QTimer::singleShot(0, this, [this]() {
        loadFileList(m_currentDirId);
    });
}

DriveView::~DriveView()
{
    delete ui;
}

void DriveView::showInlineMessage(const QString &message, bool isError)
{
    if (!m_statusLabel) {
        return;
    }

    const QString backgroundColor = isError ? "#fff1f0" : "#f6ffed";
    const QString borderColor = isError ? "#ff4d4f" : "#52c41a";
    const QString textColor = isError ? "#a8071a" : "#135200";

    m_statusLabel->setStyleSheet(QString(
        "QLabel {"
        "background-color: %1;"
        "border: 1px solid %2;"
        "border-radius: 6px;"
        "color: %3;"
        "font-size: 13px;"
        "padding: 10px 16px;"
        "}"
    ).arg(backgroundColor, borderColor, textColor));
    m_statusLabel->setAutoFillBackground(false);
    m_statusLabel->setFrameShape(QFrame::NoFrame);
    m_statusLabel->setMargin(0);
    m_statusLabel->setText(isError ? tr("错误：%1").arg(message) : message);
    positionStatusPopup();
    m_statusLabel->raise();
    m_statusLabel->setVisible(true);

    const int serial = ++m_statusMessageSerial;
    QTimer::singleShot(3000, this, [this, serial]() {
        if (m_statusLabel && serial == m_statusMessageSerial) {
            m_statusLabel->setVisible(false);
        }
    });
}

void DriveView::positionStatusPopup()
{
    if (!m_statusLabel) {
        return;
    }

    const int availableWidth = qMax(120, width() - 40);
    const int popupMaxWidth = qMin(520, availableWidth);
    const int popupMinWidth = qMin(220, popupMaxWidth);

    m_statusLabel->setMinimumWidth(popupMinWidth);
    m_statusLabel->setMaximumWidth(popupMaxWidth);
    m_statusLabel->adjustSize();

    const int x = qMax(12, (width() - m_statusLabel->width()) / 2);
    m_statusLabel->move(x, 50);
}

QString DriveView::ensureDownloadDirectory()
{
    QString downloadPath = SettingManager::Instance().personal_drive_download_dir();
    if (downloadPath.isEmpty()) {
        downloadPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        SettingManager::Instance().set_personal_drive_download_dir(downloadPath);
        showInlineMessage(tr("下载目录未设置，已使用用户目录"));
    }

    QDir dir(downloadPath);
    if (!dir.exists() && !dir.mkpath(".")) {
        showInlineMessage(tr("无法创建下载目录：%1").arg(downloadPath), true);
        return QString();
    }

    return dir.absolutePath();
}

void DriveView::polishTableView(QTableView *tableView)
{
    if (!tableView) {
        return;
    }

    tableView->setShowGrid(false);
    tableView->setAlternatingRowColors(true);
    tableView->verticalHeader()->setDefaultSectionSize(38);
    tableView->verticalHeader()->setMinimumSectionSize(34);
    tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tableView->horizontalHeader()->setHighlightSections(false);
    tableView->horizontalHeader()->setStretchLastSection(false);
    tableView->horizontalHeader()->setMinimumSectionSize(56);
    tableView->setTextElideMode(Qt::ElideMiddle);
    tableView->setFrameShape(QFrame::StyledPanel);
    tableView->setFrameShadow(QFrame::Plain);
}

void DriveView::scheduleTableLayoutUpdate()
{
    if (m_layoutUpdatePending) {
        return;
    }

    m_layoutUpdatePending = true;
    QTimer::singleShot(0, this, [this]() {
        m_layoutUpdatePending = false;
        applyDriveTableLayout();
        applyHistoryTableLayout(ui->downloadHistoryTableView);
        applyHistoryTableLayout(ui->uploadHistoryTableView);

        QTimer::singleShot(0, this, [this]() {
            applyDriveTableLayout();
            applyHistoryTableLayout(ui->downloadHistoryTableView);
            applyHistoryTableLayout(ui->uploadHistoryTableView);
        });
    });
}

void DriveView::applyDriveTableLayout()
{
    if (!ui || !ui->tableView || !m_model || m_model->columnCount() < 4) {
        return;
    }

    const int totalWidth = ui->tableView->viewport()->width();
    if (totalWidth <= 0) {
        return;
    }

    const int actionWidth = 126;
    const int sizeWidth = 112;
    const int timeWidth = 178;
    const int nameWidth = qMax(220, totalWidth - actionWidth - sizeWidth - timeWidth);

    ui->tableView->setColumnWidth(0, nameWidth);
    ui->tableView->setColumnWidth(1, sizeWidth);
    ui->tableView->setColumnWidth(2, timeWidth);
    ui->tableView->setColumnWidth(3, actionWidth);
}

void DriveView::applyHistoryTableLayout(QTableView *tableView)
{
    if (!tableView || !tableView->model() || tableView->model()->columnCount() < 6) {
        return;
    }

    const int totalWidth = tableView->viewport()->width();
    if (totalWidth <= 0) {
        return;
    }

    const int sizeWidth = 96;
    const int timeWidth = 166;
    const int statusWidth = 92;
    const int actionWidth = 86;
    const int flexibleWidth = qMax(360, totalWidth - sizeWidth - timeWidth - statusWidth - actionWidth);
    const int fileNameWidth = qMax(160, flexibleWidth * 40 / 100);
    const int pathWidth = qMax(200, flexibleWidth - fileNameWidth);

    tableView->setColumnWidth(0, fileNameWidth);
    tableView->setColumnWidth(1, sizeWidth);
    tableView->setColumnWidth(2, timeWidth);
    tableView->setColumnWidth(3, pathWidth);
    tableView->setColumnWidth(4, statusWidth);
    tableView->setColumnWidth(5, actionWidth);
}

void DriveView::on_PushFileBtn_clicked()
{
    QString downloadDir = SettingManager::Instance().personal_drive_download_dir();
    if (downloadDir.isEmpty()) {
        downloadDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件", downloadDir, "所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        DriveManager::Instance().uploadFile(filePath, m_currentDirId);
    }
}

void DriveView::loadFileList(int parentId)
{
    m_currentDirId = parentId; // 更新当前目录ID
    m_model->removeRows(0, m_model->rowCount());
    
    // 使用DriveManager获取文件列表
    m_driveManager->getCurrentDirectoryFiles(parentId);
}

void DriveView::onItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    int row = index.row();
    QStandardItem *item = m_model->item(row, 0);
    if (!item) return;

    bool isDir = item->data(DriveRoles::RoleIsDir).toBool();
    int id = item->data(DriveRoles::RoleId).toInt();

    if (isDir) {
        // 如果是文件夹，进入文件夹
        loadFileList(id);
    } else {
        // 如果是文件，下载文件
        DriveItem *driveItem = nullptr;
        // 查找对应的DriveItem
        for (DriveItem *currentItem : m_driveManager->getCurrentFileList()) {
            if (currentItem->getId() == id) {
                driveItem = currentItem;
                break;
            }
        }
        if (driveItem) {
            DriveFile *driveFile = dynamic_cast<DriveFile*>(driveItem);
            if (driveFile) {
                QString fileName = driveFile->getName();
                QString downloadPath = ensureDownloadDirectory();
                if (downloadPath.isEmpty()) {
                    return;
                }

                QDir dir(downloadPath);
                QString savePath = dir.absoluteFilePath(fileName);
                
                // 开始下载（下载记录由DriveManager在downloadFile中自动添加）
                m_driveManager->downloadFile(id, savePath);
            }
        }
    }
}

void DriveView::onFileListUpdated(const QList<DriveItem *> &fileList)
{
    updateFileList(fileList);
    scheduleTableLayoutUpdate();
}




void DriveView::onActionClicked(int row, int action)
{
    QStandardItem *item = m_model->item(row, 0);
    if (!item) return;

    int id = item->data(DriveRoles::RoleId).toInt();
    bool isDir = item->data(DriveRoles::RoleIsDir).toBool();

    switch (action) {
    case 0: // 下载
        if (!isDir) {
            QString fileName = item->text();
            QString downloadPath = ensureDownloadDirectory();
            if (downloadPath.isEmpty()) {
                return;
            }

            QDir dir(downloadPath);
            QString savePath = dir.absoluteFilePath(fileName);
            
            // 开始下载（下载记录由DriveManager在downloadFile中自动添加）
            m_driveManager->downloadFile(id, savePath);
        } else {
            qDebug() << "不能下载文件夹，行：" << row;
        }
        break;
    case 1: // 删除
        m_driveManager->deleteItem(id);
        break;
    case 2: // 重命名
        {
            QString oldName = item->text();
            bool ok;
            QString newName = QInputDialog::getText(this, tr("重命名"), tr("请输入新名称:"), QLineEdit::Normal, oldName, &ok);
            
            if (ok && !newName.isEmpty() && newName != oldName) {
                // 检查当前目录下是否有同名文件/文件夹
                bool isDuplicate = false;
                for (int row = 0; row < m_model->rowCount(); ++row) {
                    QStandardItem *currentItem = m_model->item(row, 0);
                    if (currentItem && currentItem->text() == newName) {
                        isDuplicate = true;
                        break;
                    }
                }
                
                if (isDuplicate) {
                    showInlineMessage(tr("当前目录下已存在同名文件或文件夹"), true);
                } else {
                    // 调用DriveManager进行重命名
                    m_driveManager->renameItem(id, newName);
                }
            }
        }
        break;
    case 3: // 移动
        break;

    default:
        break;
    }
}

void DriveView::onOperationSuccess(const QString &message)
{
    qDebug() << "操作成功:" << message;

    if (m_isClearDriveRequesting) {
        m_isClearDriveRequesting = false;
        m_currentDirId = 0;
        ui->ClearDriveBtn->setEnabled(true);
    }

    showInlineMessage(message);
}

void DriveView::onOperationFailed(const QString &errorMessage)
{
    qDebug() << "操作失败:" << errorMessage;

    if (m_isClearDriveRequesting) {
        m_isClearDriveRequesting = false;
        ui->ClearDriveBtn->setEnabled(true);
    }

    showInlineMessage(errorMessage, true);
}

// 加载上传历史
void DriveView::loadUploadHistory() {
    m_uploadHistoryModel->clear();
    m_uploadHistoryModel->setHorizontalHeaderLabels({
        tr("文件名"), tr("大小"), tr("上传时间"), tr("本地路径"), tr("状态"), tr("操作")
    });
    
    QList<DriveUploadRecord> records = m_driveManager->getUploadHistory();
    
    for (const DriveUploadRecord &record : records) {
        QStandardItem *fileNameItem = new QStandardItem(record.fileName);
        QStandardItem *fileSizeItem = new QStandardItem(formatFileSize(record.fileSize));
        QStandardItem *uploadTimeItem = new QStandardItem(record.uploadTime.toString("yyyy-MM-dd HH:mm:ss"));
        QStandardItem *localPathItem = new QStandardItem(record.localPath);
        
        // 根据状态设置显示文本和颜色
        QString statusText;
        if (record.uploadStatus == "uploading") {
            statusText = tr("上传中");
        } else if (record.uploadStatus == "success") {
            statusText = tr("上传成功");
        } else if (record.uploadStatus == "failed") {
            statusText = tr("上传失败");
        } else {
            statusText = record.uploadStatus;
        }
        QStandardItem *statusItem = new QStandardItem(statusText);
        
        // 设置状态文本颜色
        if (record.uploadStatus == "uploading") {
            statusItem->setForeground(QBrush(Qt::blue));
        } else if (record.uploadStatus == "success") {
            statusItem->setForeground(QBrush(Qt::green));
        } else if (record.uploadStatus == "failed") {
            statusItem->setForeground(QBrush(Qt::red));
        }
        
        QStandardItem *actionItem = new QStandardItem();
        QList<QStandardItem*> items = {fileNameItem, fileSizeItem, uploadTimeItem, localPathItem, statusItem, actionItem};
        m_uploadHistoryModel->appendRow(items);
    }

    scheduleTableLayoutUpdate();
}

// 加载下载历史
void DriveView::loadDownloadHistory() {
    m_downloadHistoryModel->clear();
    m_downloadHistoryModel->setHorizontalHeaderLabels({
        tr("文件名"), tr("大小"), tr("下载时间"), tr("保存路径"), tr("状态"), tr("操作")
    });
    
    QList<DriveDownloadRecord> records = m_driveManager->getDownloadHistory();
    
    for (const DriveDownloadRecord &record : records) {
        QStandardItem *fileNameItem = new QStandardItem(record.fileName);
        QStandardItem *fileSizeItem = new QStandardItem(formatFileSize(record.fileSize));
        QStandardItem *downloadTimeItem = new QStandardItem(record.downloadTime.toString("yyyy-MM-dd HH:mm:ss"));
        QStandardItem *savePathItem = new QStandardItem(record.savePath);
        
        // 根据状态设置显示文本和颜色
        QString statusText;
        if (record.downloadStatus == "downloading") {
            statusText = tr("下载中");
        } else if (record.downloadStatus == "success") {
            statusText = tr("下载成功");
        } else if (record.downloadStatus == "failed") {
            statusText = tr("下载失败");
        } else {
            statusText = record.downloadStatus;
        }
        QStandardItem *statusItem = new QStandardItem(statusText);
        
        // 设置状态文本颜色
        if (record.downloadStatus == "downloading") {
            statusItem->setForeground(QBrush(Qt::blue));
        } else if (record.downloadStatus == "success") {
            statusItem->setForeground(QBrush(Qt::green));
        } else if (record.downloadStatus == "failed") {
            statusItem->setForeground(QBrush(Qt::red));
        }
        
        QStandardItem *actionItem = new QStandardItem();
        QList<QStandardItem*> items = {fileNameItem, fileSizeItem, downloadTimeItem, savePathItem, statusItem, actionItem};
        m_downloadHistoryModel->appendRow(items);
    }

    scheduleTableLayoutUpdate();
}

// 清空历史记录
void DriveView::onClearHistoryBtnClicked() {
    int currentIndex = ui->historyTabWidget->currentIndex();
    
    if (currentIndex == 0) {
        // 清空下载历史
        if (QMessageBox::question(this, tr("确认清空"), tr("确定要清空所有下载历史记录吗？")) == QMessageBox::Yes) {
            if (m_driveManager->clearDownloadHistory()) {
                loadDownloadHistory();
                showInlineMessage(tr("下载历史已清空"));
            } else {
                showInlineMessage(tr("清空下载历史失败"), true);
            }
        }
    } else if (currentIndex == 1) {
        // 清空上传历史
        if (QMessageBox::question(this, tr("确认清空"), tr("确定要清空所有上传历史记录吗？")) == QMessageBox::Yes) {
            if (m_driveManager->clearUploadHistory()) {
                loadUploadHistory();
                showInlineMessage(tr("上传历史已清空"));
            } else {
                showInlineMessage(tr("清空上传历史失败"), true);
            }
        }
    }
}

// 构建面包屑路径
void DriveView::buildBreadcrumbPath()
{
    // 调用DriveManager的getPath方法获取当前目录的完整路径
    m_driveManager->getPath(m_currentDirId);
}

// 处理从DriveManager接收到的目录路径
void DriveView::onPathReceived(const QList<DriveItem *> &pathItems)
{
    // 清空之前的路径
    m_breadcrumbPath.clear();

    QList<BreadcrumbNode*> path;
    for (DriveItem* item : pathItems) {
        BreadcrumbNode* node = new BreadcrumbNode(item->getName());
        node->data = item->getId(); // 保存目录ID
        path.append(node);
        m_breadcrumbPath.append(node); // 初始化成员变量
    }

    // 添加当前目录下的子目录（不影响成员变量）
    if (!path.isEmpty()) {
        QList<DriveItem*> currentFiles = m_driveManager->getCurrentFileList();
        for (DriveItem* item : currentFiles) {
            if (item->isFolder()) {
                BreadcrumbNode* childNode = new BreadcrumbNode(item->getName());
                childNode->data = item->getId();
                path.last()->children.append(childNode);
            }
        }
    }

    breadcrumb->setBreadcrumbPath(path); // 更新显示
}

void DriveView::updateFileList(const QList<DriveItem *> &fileList)
{
    m_model->removeRows(0, m_model->rowCount());

    for (DriveItem *item : fileList) {
        bool isDir = item->isFolder();

        IconManager::Icon iconEnum = isDir ? IconManager::Icon::Folder
                                           : IconManager::Icon::File;

        QStandardItem *nameItem = new QStandardItem(item->getName());
        nameItem->setData(int(iconEnum), DriveRoles::RoleIcon);  // 保存 Icon enum
        nameItem->setData(item->getName(), DriveRoles::RoleText);
        nameItem->setData(item->getId(), DriveRoles::RoleId);
        nameItem->setData(isDir, DriveRoles::RoleIsDir);

        QStandardItem *sizeItem = new QStandardItem();
        if (!isDir) {
            DriveFile *file = dynamic_cast<DriveFile *>(item);
            if (file) {
                sizeItem->setText(formatFileSize(file->getSize()));
            }
        }

        QStandardItem *timeItem = new QStandardItem(item->getCreatedAt().toString(Qt::ISODate));
        QStandardItem *actionItem = new QStandardItem();

        m_model->appendRow({nameItem, sizeItem, timeItem, actionItem});
    }

    scheduleTableLayoutUpdate();
    buildBreadcrumbPath();
}


void DriveView::on_RefreshBtn_clicked()
{
    loadFileList(m_currentDirId);
    loadDownloadHistory();
    loadUploadHistory();
}

void DriveView::on_ClearDriveBtn_clicked()
{
    if (m_isClearDriveRequesting) {
        return;
    }

    if (QMessageBox::question(this,
                              tr("确认清空"),
                              tr("确定要清空整个网盘吗？此操作会删除网盘中的所有文件和文件夹。"))
        != QMessageBox::Yes) {
        return;
    }

    m_isClearDriveRequesting = true;
    ui->ClearDriveBtn->setEnabled(false);
    m_driveManager->clearDrive();
}

void DriveView::on_NewFloderBtn_clicked()
{
    // 弹出对话框让用户输入文件夹名称
    bool ok;
    QString folderName = QInputDialog::getText(this, tr("新建文件夹"), 
                                               tr("文件夹名称:"), QLineEdit::Normal, 
                                               tr("新建文件夹"), &ok);
    
    if (ok && !folderName.isEmpty()) {
        m_driveManager->createFolder(folderName, m_currentDirId);
    }
}

void DriveView::onDownloadSuccess(const QString &filePath) {
    qDebug() << "文件下载成功:" << filePath;
    // 下载状态更新已由DriveManager在onFileDownloaded中处理
    loadDownloadHistory();
}

// 下载失败处理
void DriveView::onDownloadFailed(const QString &errorMessage) {
    qDebug() << "文件下载失败:" << errorMessage;
    // 下载状态更新已由DriveManager在onError中处理
    loadDownloadHistory();
    
    // 如果是文件不存在的错误，提示用户刷新文件列表
    if (errorMessage.contains("404") || errorMessage.contains("不存在") || 
        errorMessage.contains("not found", Qt::CaseInsensitive)) {
        showInlineMessage(tr("该文件可能已被删除或移动，已刷新文件列表。"), true);
        on_RefreshBtn_clicked();
    }
}

// 上传成功处理
void DriveView::onUploadSuccess(const QString &message) {
    qDebug() << "文件上传成功:" << message;
    // 上传状态更新已由DriveManager在onFileUploaded中处理
    loadUploadHistory();
}

// 上传失败处理
void DriveView::onUploadFailed(const QString &errorMessage) {
    qDebug() << "文件上传失败:" << errorMessage;
    // 上传状态更新已由DriveManager在onError中处理
    loadUploadHistory();
}

void DriveView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    positionStatusPopup();
    scheduleTableLayoutUpdate();
}

void DriveView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    positionStatusPopup();
    scheduleTableLayoutUpdate();
}

void DriveView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DriveView::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls()) {
        return;
    }
    
    QList<QUrl> urls = mimeData->urls();
    for (const QUrl &url : urls) {
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);
        
        if (fileInfo.isFile()) {
            // 上传文件
            DriveManager::Instance().uploadFile(filePath, m_currentDirId);
        }
        // 文件夹暂时不管
    }
}

bool DriveView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->tableView) {
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEnterEvent = static_cast<QDragEnterEvent*>(event);
            if (dragEnterEvent->mimeData()->hasUrls()) {
                dragEnterEvent->acceptProposedAction();
                return true;
            }
        } else if (event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
            const QMimeData *mimeData = dropEvent->mimeData();
            if (mimeData->hasUrls()) {
                QList<QUrl> urls = mimeData->urls();
                for (const QUrl &url : urls) {
                    QString filePath = url.toLocalFile();
                    QFileInfo fileInfo(filePath);
                    
                    if (fileInfo.isFile()) {
                        // 上传文件
                        DriveManager::Instance().uploadFile(filePath, m_currentDirId);
                    }
                    // 文件夹暂时不管
                }
                dropEvent->acceptProposedAction();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

// 下载历史 Delegate - 打开文件所在位置
void DriveView::onDownloadHistoryOpenLocationClicked(int row)
{
    if (row < 0 || row >= m_downloadHistoryModel->rowCount()) {
        return;
    }
    
    QString filePath = m_downloadHistoryModel->item(row, 3)->text(); // 保存路径在第4列
    
    if (filePath.isEmpty()) {
        showInlineMessage(tr("文件路径为空"), true);
        return;
    }
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        showInlineMessage(tr("文件不存在: %1").arg(filePath), true);
        return;
    }
    
    // 打开文件所在文件夹
    FileLocationHelper::openFileLocationWithSelection(filePath);
}

// 下载历史 Delegate - 预览文件
void DriveView::onDownloadHistoryPreviewClicked(int row)
{
    if (row < 0 || row >= m_downloadHistoryModel->rowCount()) {
        return;
    }
    
    QString filePath = m_downloadHistoryModel->item(row, 3)->text(); // 保存路径在第4列
    
    if (filePath.isEmpty()) {
        showInlineMessage(tr("文件路径为空"), true);
        return;
    }
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        showInlineMessage(tr("文件不存在: %1").arg(filePath), true);
        return;
    }
    
    // 使用TabFactory创建对应的标签页来预览文件
    TabAbstract* newTab = TabFactory::create(filePath);
    if (!newTab) {
        // 用户取消或文件类型不支持
        return;
    }
    
    newTab->loadFromFile(filePath);
    
    // 获取TabManager并添加标签页
    QWidget *parent = this->parentWidget();
    while (parent) {
        TabManager *tabManager = parent->findChild<TabManager*>();
        if (tabManager) {
            QString displayName = fileInfo.fileName();
            tabManager->addTab(newTab, displayName, filePath);
            return;
        }
        parent = parent->parentWidget();
    }
    
    // 如果找不到TabManager，直接打开文件
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}

// 上传历史 Delegate - 打开文件所在位置
void DriveView::onUploadHistoryOpenLocationClicked(int row)
{
    if (row < 0 || row >= m_uploadHistoryModel->rowCount()) {
        return;
    }
    
    QString filePath = m_uploadHistoryModel->item(row, 3)->text(); // 本地路径在第4列
    
    if (filePath.isEmpty()) {
        showInlineMessage(tr("文件路径为空"), true);
        return;
    }
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        showInlineMessage(tr("文件不存在: %1").arg(filePath), true);
        return;
    }
    
    // 打开文件所在文件夹
    FileLocationHelper::openFileLocationWithSelection(filePath);
}

// 上传历史 Delegate - 预览文件
void DriveView::onUploadHistoryPreviewClicked(int row)
{
    if (row < 0 || row >= m_uploadHistoryModel->rowCount()) {
        return;
    }
    
    QString filePath = m_uploadHistoryModel->item(row, 3)->text(); // 本地路径在第4列
    
    if (filePath.isEmpty()) {
        showInlineMessage(tr("文件路径为空"), true);
        return;
    }
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        showInlineMessage(tr("文件不存在: %1").arg(filePath), true);
        return;
    }
    
    // 使用TabFactory创建对应的标签页来预览文件
    TabAbstract* newTab = TabFactory::create(filePath);
    if (!newTab) {
        // 用户取消或文件类型不支持
        return;
    }
    
    newTab->loadFromFile(filePath);
    
    // 获取TabManager并添加标签页
    QWidget *parent = this->parentWidget();
    while (parent) {
        TabManager *tabManager = parent->findChild<TabManager*>();
        if (tabManager) {
            QString displayName = fileInfo.fileName() + tr(" (预览)");
            tabManager->addTab(newTab, displayName, filePath);
            return;
        }
        parent = parent->parentWidget();
    }
    
    // 如果找不到TabManager，直接打开文件
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}

QString DriveView::formatFileSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 2);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 2);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 2);
    }
}

