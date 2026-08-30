#include "include/TagList.h"
#include "ui/ui_TagList.h"
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include "../manager/include/dbService.h"
#include "../manager/include/FileLocationHelper.h"

TagList::TagList(const FilePathInfo &fileInfo, QWidget *parent)
    : QWidget(parent), ui(new Ui::TagList), fileInfo(fileInfo), dbservice(dbService::instance("./SmartDesk.db"))
{
    ui->setupUi(this);
    // 列表项自身会被 QListWidget 裁剪，弹窗统一挂到父面板（ScheduleWid）上
    m_messagePopup = new InlineMessagePopup(parent ? parent : this);
    m_messagePopup->setPanelWidget(parent ? parent : this); // 弹窗显示在面板内部顶部居中

    QFileInfo fileInfoObj(fileInfo.filePath);
    ui->fileLabel->setText(fileInfoObj.fileName());

    ui->lineEdit->setMaximumHeight(30);
    ui->lineEdit->setText(fileInfoObj.path());
    ui->lineEdit->setToolTip(fileInfoObj.path());

    ui->expirationLabel->setText(getExpInfo(fileInfo.expirationDate));
    ui->tagLabel->setText(fileInfo.tagName);
}



TagList::~TagList()
{
    // popup 挂在父面板（ScheduleWid）上，不会随 TagList 自动析构，需手动释放
    delete m_messagePopup;
    delete ui;
}

QString TagList::getFilePath() const
{
    return fileInfo.filePath;
}

QString TagList::getExpInfo(const QDateTime expDate)
{
    QDateTime now = QDateTime::currentDateTime();
    QString expInfo;

    if (expDate.isValid()) {
        qint64 mnow = now.secsTo(expDate);
        if (mnow >= 0) {
            int s = mnow / 3600;        // 计算小时数
            int m = (mnow % 3600) / 60; // 计算分钟数
            expInfo = QString("%1:%2").arg(s, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0'));
        } else {
            // int oh = -mnow / 3600;         // 计算小时数（过期时是负值）
            // int om = (-mnow % 3600) / 60;  // 计算分钟数（过期时是负值）
            // expInfo = QString("%1:%2").arg(oh, 2, 10, QChar('0')).arg(om, 2, 10, QChar('0'));
            expInfo = "已过期";
        }
    } else {
        expInfo = "No expiration date";
    }

    return expDate.isValid()
               ? expDate.toString("MM-dd hh:mm\n") + " (" + expInfo + ")" : expInfo;
}

void TagList::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    // 创建右键菜单项
    QAction *previewAction = menu.addAction("预览");
    QAction *detailAction = menu.addAction("详情");
    QAction *modifyPathAction = menu.addAction("修改路径");
    QAction *deleteTagAction = menu.addAction("删除标签");
    QAction *openLocationAction = menu.addAction("打开文件位置");
    QAction *openInFileSystemAction = menu.addAction("在文件系统打开");

    // 连接槽函数
    connect(previewAction, &QAction::triggered, this, &TagList::onPreviewAction);
    connect(openLocationAction, &QAction::triggered, this, &TagList::onOpenLocationAction);
    connect(openInFileSystemAction, &QAction::triggered, this, &TagList::onOpenInFileSystemAction);
    connect(detailAction, &QAction::triggered, this, &TagList::onDetailAction);
    connect(modifyPathAction, &QAction::triggered, this, &TagList::onModifyPathAction);
    connect(deleteTagAction, &QAction::triggered, this, &TagList::onDeleteTagAction);

    // 显示右键菜单
    menu.exec(event->globalPos());
}

void TagList::onOpenInFileSystemAction()
{
    // 发送信号，通知文件系统打开并跳转到该文件所在路径
    emit openInFileSystemRequested(fileInfo.filePath);
}

void TagList::onPreviewAction()
{
    // 预览功能与之前的点击功能一致，打开文件详情窗口
    onDetailAction();
}

void TagList::onOpenLocationAction()
{
    // 打开文件所在位置并选中该文件
    FileLocationHelper::openFileLocationWithSelection(fileInfo.filePath);
}

void TagList::onDetailAction()
{
    // 实现单例模式，确保只有一个标签详情窗口实例
    if (!tagdetail) {
        tagdetail = new TagDetail(nullptr, fileInfo);
        // 连接destroyed信号，当窗口关闭时重置指针
        connect(tagdetail, &QObject::destroyed, this, [=]() {
            tagdetail = nullptr;
        });
        tagdetail->show();
    } else {
        // 如果窗口已存在，将其显示在前台
        tagdetail->show();
        tagdetail->raise();
        tagdetail->activateWindow();
    }
}

void TagList::onModifyPathAction()
{
    // 选择新文件路径
    QString newPath = QFileDialog::getOpenFileName(this, "选择新文件", "", "所有文件 (*)");
    if (!newPath.isEmpty()) {
        QString oldFilePath = fileInfo.filePath;

        // 验证文件是否存在
        QFileInfo fileInfoCheck(newPath);
        if (!fileInfoCheck.exists()) {
            m_messagePopup->showMessage("新文件不存在", true);
            return;
        }

        // 更新数据库中的文件路径
        bool result = dbservice.dbTags().updateFilePath(newPath, oldFilePath);
        if (result) {
            // 更新本地fileInfo
            fileInfo.filePath = newPath;

            // 更新UI显示
            QFileInfo newFileInfoObj(newPath);
            ui->fileLabel->setText(newFileInfoObj.fileName());
            ui->lineEdit->setText(newFileInfoObj.path());
            ui->lineEdit->setToolTip(newFileInfoObj.path());

            m_messagePopup->showMessage("文件路径已更新");
        } else {
            m_messagePopup->showMessage("更新文件路径失败", true);
        }
    }
}

void TagList::onDeleteTagAction()
{
    // 确认删除操作
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除", "确认删除标签吗?", QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        int fileId;
        // 获取文件ID
        if (!dbservice.dbTags().getFileId(fileInfo.filePath, fileId)) {
            m_messagePopup->showMessage("无法找到文件ID！", true);
            return;
        }

        // 删除标签
        bool result = dbservice.dbTags().deleteTag(fileId);
        if (result) {
            m_messagePopup->showMessage("标签已删除");
            // 可以考虑发出信号通知父组件删除此TagList
        } else {
            m_messagePopup->showMessage("删除过程中出现错误", true);
        }
    }
}

