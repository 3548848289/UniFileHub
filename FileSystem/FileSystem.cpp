#include "include/FileSystem.h"
#include "ui/ui_FileSystem.h"
#include <QTimer>

FileSystem::FileSystem(QWidget *parent)
    : QWidget(parent), ui(new Ui::FileSystem), serverManager(ServerManager::instance()),
    fileSystemModel(new QFileSystemModel(this))
{
    ui->setupUi(this);

    QString userDir = SettingManager::Instance().file_system_file_system_dir();
    if (userDir.isEmpty())
        userDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QDir dir(userDir);
    QString currentDir = dir.absolutePath();

    fileSystemModel->setRootPath(currentDir);

    QFont font = ui->treeView->font();
    font.setPointSize(12);
    ui->treeView->setFont(font);
    ui->treeView->setWordWrap(true);
    ui->treeView->setStyleSheet("QTreeView::item { height: 30px; }");
    ui->treeView->setModel(fileSystemModel);
    ui->treeView->setHeaderHidden(true);
    for (int i = 1; i < fileSystemModel->columnCount(); ++i) {
        ui->treeView->setColumnHidden(i, true);
    }
    ui->treeView->setRootIndex(fileSystemModel->index(currentDir));

    // ui->pathLineEdit->setText(currentDir);

    tagItemdelegate = new TagItemDelegate(this, serverManager);
    ui->treeView->setItemDelegate(tagItemdelegate);

    connect(ui->treeView, &QTreeView::clicked, this, &FileSystem::onItemClicked);
    connect(ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FileSystem::onSelectionChanged);

    connect(tagItemdelegate, &TagItemDelegate::openFileRequested, this, [=](const QString &filePath){
        emit fileOpened(filePath);
    });


    connect(tagItemdelegate, &TagItemDelegate::TagUpdated, this, [=](){
        // 使用更强制的刷新方法，先重置模型，再更新
        QModelIndex currentRootIndex = ui->treeView->rootIndex();
        ui->treeView->reset();
        ui->treeView->update();
        ui->treeView->setRootIndex(currentRootIndex); // 重新设置根索引以强制刷新
    });

    connect(tagItemdelegate, &TagItemDelegate::tagbutClicked, this, [this](const QModelIndex &index) {
        emit tagopened();
    });
    connect(tagItemdelegate, &TagItemDelegate::subbutClicked, this, [this](const QModelIndex &index) {
        emit filebackuplistOpened();
    });

    connect(tagItemdelegate, &TagItemDelegate::deleteFileRequested, this, [this](const QString &filePath) {
        emit deleteFileRequested(filePath);
    });

    // 创建面包屑控件
    breadcrumb = new QFileSystemBreadcrumbBar();
    breadcrumb->setPath(currentDir);

    ui->horizontalLayout->addWidget(breadcrumb);
    // 捕捉信号
    connect(breadcrumb, &QFileSystemBreadcrumbBar::pathClicked, [this](int index, const QString& name) {
        if(name != "")
            changePath(name);
    });

    connect(breadcrumb, &QFileSystemBreadcrumbBar::fileClicked,
            [](const QString& path) {
                qDebug() << "File clicked:" << path;
    });
    connect(breadcrumb, &QFileSystemBreadcrumbBar::pathEdited, [this](const QString& path){
        if(path != "")
            changePath(path);
    });

    // 安装事件过滤器来捕获鼠标和键盘事件
    ui->treeView->installEventFilter(this);
    isMouseClick = false;
    isKeyboardSelection = false;
}



void FileSystem::onItemClicked(const QModelIndex &index) {
    if (fileSystemModel->isDir(index) || tagItemdelegate->isButtonClicked) {
        tagItemdelegate->isButtonClicked = false;
        return;
    }

    QString curfilePath = fileSystemModel->filePath(index);
    QString directoryPath = QFileInfo(curfilePath).absolutePath();

    currentDir = directoryPath;

    // 发射文件打开信号
    emit fileOpened(curfilePath);

    // 延迟设置焦点到文件系统视图，确保在信号处理完成后焦点仍然在treeView上
    QTimer::singleShot(0, this, [this]() {
        ui->treeView->setFocus();
    });
}


void FileSystem::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    if (!selected.isEmpty()) {
        QModelIndex index = selected.first().topLeft();
        if (!fileSystemModel->isDir(index)) {
            QString curfilePath = fileSystemModel->filePath(index);
            this->curfilePath = curfilePath;
            // qDebug() << "curfilePath；" << curfilePath;

            // 区分鼠标和键盘触发的选择
            if (isMouseClick) {
                qDebug() << "选择由鼠标触发";
                isMouseClick = false;
            } else if (isKeyboardSelection) {
                qDebug() << "选择由键盘触发";
                emit fileSelectedByKeyboard(curfilePath);
                isKeyboardSelection = false;
            } else {
                qDebug() << "选择由其他方式触发";
            }

        }
    }
}



void FileSystem::changePath(const QString& path){
    QFileInfo fileInfo(path);
    if (fileInfo.exists() && fileInfo.isDir()) {
        ui->treeView->setRootIndex(fileSystemModel->index(path));
        // 更新面包屑路径
        breadcrumb->setPath(path);
        // 更新当前目录记录
        currentDir = path;
    } else {
        QMessageBox::warning(this, "", "改路径不是有效路径，请重新输入");
    }
}

bool FileSystem::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->treeView) {
        if (event->type() == QEvent::MouseButtonPress) {
            isMouseClick = true;
            isKeyboardSelection = false;
            qDebug() << "Mouse click detected";
        } else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down ||
                keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right ||
                keyEvent->key() == Qt::Key_PageUp || keyEvent->key() == Qt::Key_PageDown ||
                keyEvent->key() == Qt::Key_Home || keyEvent->key() == Qt::Key_End) {
                isKeyboardSelection = true;
                isMouseClick = false;
                qDebug() << "Keyboard navigation detected";
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

FileSystem::~FileSystem() {
    delete ui;
}



