#include "PersonalDriveView.h"
#include "ui_PersonalDriveView.h"
#include "include/DriveManager.h"
#include "include/DriveItem.h"
#include "include/DriveFile.h"
#include "include/DriveFolder.h"
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>

PersonalDriveView::PersonalDriveView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PersonalDriveView)
    , m_currentDirId(0)
{
    ui->setupUi(this);

    // 创建面包屑控件
    breadcrumb = new QFileSystemBreadcrumbBar();
    // 将面包屑插入到标题下方（垂直布局的索引1位置）
    ui->verticalLayout->insertWidget(1, breadcrumb);

    // 捕捉信号
    connect(breadcrumb, &QFileSystemBreadcrumbBar::pathClicked, this,
            [this](int clickedIndex, const QString&) {
                if (clickedIndex < 0) return;

                BreadcrumbNode* node = breadcrumb->bar()->path().at(clickedIndex);
                if (!node) return;

                int dirId = node->data.toInt();  // ← 用这个访问后端
                loadFileList(dirId);
            });


    connect(breadcrumb, &QFileSystemBreadcrumbBar::fileClicked,
            [](const QString& path) {
                qDebug() << "File clicked:" << path;
            });
    connect(breadcrumb, &QFileSystemBreadcrumbBar::pathEdited, [this](const QString& path){
        // if(path != "")
            // changePath(path);
    });
    this->setWindowTitle(tr("Personal Drive"));

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(
        {"文件名", "大小(字节)", "上传时间"}
        );

    ui->tableView->setModel(m_model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->verticalHeader()->setVisible(false);

    connect(ui->tableView, &QTableView::doubleClicked,
            this, &PersonalDriveView::onItemDoubleClicked);

    // 获取DriveManager实例
    m_driveManager = &DriveManager::Instance();
    
    // 连接DriveManager信号
    connect(m_driveManager, &DriveManager::fileListUpdated,
            this, &PersonalDriveView::onFileListUpdated);

    connect(m_driveManager, &DriveManager::pathReceived,
            this, &PersonalDriveView::onPathReceived);

    connect(m_driveManager, &DriveManager::operationSuccess,
            this, &PersonalDriveView::onOperationSuccess);
    
    connect(m_driveManager, &DriveManager::operationFailed,
            this, &PersonalDriveView::onOperationFailed);


}

PersonalDriveView::~PersonalDriveView()
{
    delete ui;
    // DriveManager是单例，不需要手动删除
}

void PersonalDriveView::on_PushFileBtn_clicked()
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

void PersonalDriveView::loadFileList(int parentId)
{
    m_currentDirId = parentId; // 更新当前目录ID
    m_model->removeRows(0, m_model->rowCount());
    
    // 使用DriveManager获取文件列表
    m_driveManager->getCurrentDirectoryFiles(parentId);
}

void PersonalDriveView::onItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    int row = index.row();
    QStandardItem *item = m_model->item(row, 0);

    bool isDir = item->data(Qt::UserRole + 1).toBool();
    if (!isDir) return;

    m_currentDirId = item->data(Qt::UserRole).toInt();
    loadFileList(m_currentDirId);
}

void PersonalDriveView::onFileListUpdated(const QList<DriveItem *> &fileList)
{
    updateFileList(fileList);
}




void PersonalDriveView::onOperationSuccess(const QString &message)
{
    qDebug() << "操作成功:" << message;
    QMessageBox::information(this, tr("提示"), message);
}

void PersonalDriveView::onOperationFailed(const QString &errorMessage)
{
    qDebug() << "操作失败:" << errorMessage;
    QMessageBox::warning(this, tr("错误"), errorMessage);
}

// 构建面包屑路径
void PersonalDriveView::buildBreadcrumbPath()
{
    // 调用DriveManager的getPath方法获取当前目录的完整路径
    m_driveManager->getPath(m_currentDirId);
}

// 处理从DriveManager接收到的目录路径
void PersonalDriveView::onPathReceived(const QList<DriveItem *> &pathItems)
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

void PersonalDriveView::updateFileList(const QList<DriveItem *> &fileList)
 { 
     for (DriveItem *item : fileList) { 
         bool isDir = item->isFolder(); 

         QStandardItem *nameItem = 
             new QStandardItem( 
                 isDir ? QIcon(":/icons/folder.png") 
                       : QIcon(":/icons/file.png"), 
                 item->getName() 
                 ); 

         nameItem->setData(item->getId(), Qt::UserRole);        // itemId 
         nameItem->setData(isDir, Qt::UserRole + 1);           // isDir 

         QStandardItem *sizeItem = new QStandardItem(); 
         if (isDir) { 
             sizeItem->setText(""); 
         } else { 
             DriveFile *file = dynamic_cast<DriveFile *>(item); 
             if (file) { 
                 sizeItem->setText(QString::number(file->getSize())); 
             } 
         } 

         QStandardItem *timeItem = 
             new QStandardItem(item->getCreatedAt().toString(Qt::ISODate)); 

         m_model->appendRow({nameItem, sizeItem, timeItem}); 
     } 

     // 构建并设置面包屑路径
     buildBreadcrumbPath(); 
 }

void PersonalDriveView::on_pushButton_clicked()
{
    loadFileList(0);
}
