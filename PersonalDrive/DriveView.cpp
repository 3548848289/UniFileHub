#include "include/DriveView.h"
#include "ui_DriveView.h"
#include "include/DriveManager.h"
#include "include/DriveItem.h"
#include "include/DriveFile.h"
#include "include/DriveFolder.h"
#include "include/DriveViewDelegate.h"
#include "../../Setting/include/SettingManager.h"
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QInputDialog>

DriveView::DriveView(QWidget *parent): QWidget(parent), ui(new Ui::DriveView), m_currentDirId(0)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("Personal Drive"));

    // ===== 1. Breadcrumb =====
    breadcrumb = new QFileSystemBreadcrumbBar();
    breadcrumb->setAllowEditMode(false);
    breadcrumb->setBreadcrumbPath({new BreadcrumbNode("网盘")});

    ui->verticalLayout->insertWidget(0, breadcrumb);

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
        tr("名称"), tr("大小(字节)"), tr("上传时间")
    });

    // ===== 3. TableView =====
    ui->tableView->setModel(m_model);
    ui->tableView->setMouseTracking(true);          // ⭐ 必须
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->verticalHeader()->setVisible(false);


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

    connect(m_driveManager, &DriveManager::fileListUpdated,
            this, &DriveView::onFileListUpdated);
    connect(m_driveManager, &DriveManager::pathReceived,
            this, &DriveView::onPathReceived);
    connect(m_driveManager, &DriveManager::operationSuccess,
            this, &DriveView::onOperationSuccess);
    connect(m_driveManager, &DriveManager::operationFailed,
            this, &DriveView::onOperationFailed);
}

DriveView::~DriveView()
{
    delete ui;
}

void DriveView::on_PushFileBtn_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"), "E:/Tmp", tr("所有文件 (*.*);"));

    if (filePath.isEmpty()) {
        qDebug() << "用户取消了选择";
        return;
    }

    qDebug() << "选中的文件路径:" << filePath;
    
    // 使用DriveManager上传文件
    m_driveManager->uploadFile(filePath, m_currentDirId);
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

    bool isDir = item->data(DriveRoles::RoleIsDir).toBool();
    if (!isDir) return;

    m_currentDirId = item->data(DriveRoles::RoleId).toInt();
    loadFileList(m_currentDirId);
}

void DriveView::onFileListUpdated(const QList<DriveItem *> &fileList)
{
    updateFileList(fileList);
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
            QString downloadPath = SettingManager::Instance().personal_drive_download_dir();
            QDir dir(downloadPath);
            
            // 如果设置的路径为空或不存在，则使用默认路径
            if (downloadPath.isEmpty() || !dir.exists()) {
                dir.setPath("E:/Tmp");
                if (!dir.exists()) {
                    dir.mkpath(".");
                }
            }
            
            QString savePath = dir.absoluteFilePath(fileName);
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
                    QMessageBox::warning(this, tr("错误"), tr("当前目录下已存在同名文件或文件夹"));
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
    QMessageBox::information(this, tr("提示"), message);
}

void DriveView::onOperationFailed(const QString &errorMessage)
{
    qDebug() << "操作失败:" << errorMessage;
    QMessageBox::warning(this, tr("错误"), errorMessage);
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
                sizeItem->setText(QString::number(file->getSize()));
            }
        }

        QStandardItem *timeItem = new QStandardItem(item->getCreatedAt().toString(Qt::ISODate));

        m_model->appendRow({nameItem, sizeItem, timeItem});
    }

    buildBreadcrumbPath();
}


void DriveView::on_RefreshBtn_clicked()
{
    loadFileList(0);
    int totalWidth = ui->tableView->viewport()->width();
    ui->tableView->setColumnWidth(0, totalWidth * 4 / 9); // 名称
    ui->tableView->setColumnWidth(1, totalWidth * 2 / 9); // 大小
    ui->tableView->setColumnWidth(2, totalWidth * 3 / 9); // 上传时间

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

