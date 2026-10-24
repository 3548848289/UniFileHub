#include "include/TagList.h"
#include "ui/ui_TagList.h"
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include "../manager/include/dbService.h"
#include "../manager/include/FileLocationHelper.h"
#include "../manager/include/DataChangeNotifier.h"
#include "../PersonalDrive/include/DriveUploadHelper.h"

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
    if (!expDate.isValid()) {
        return QStringLiteral("无到期日");
    }

    const qint64 secondsLeft = QDateTime::currentDateTime().secsTo(expDate);
    QString relative;
    if (secondsLeft < 0) {
        const qint64 overdue = -secondsLeft;
        const qint64 days = overdue / 86400;
        const qint64 hours = (overdue % 86400) / 3600;
        if (days > 0) {
            relative = QStringLiteral("已过期 %1 天").arg(days);
        } else if (hours > 0) {
            relative = QStringLiteral("已过期 %1 小时").arg(hours);
        } else {
            relative = QStringLiteral("已过期不到 1 小时");
        }
    } else {
        const qint64 days = secondsLeft / 86400;
        const qint64 hours = (secondsLeft % 86400) / 3600;
        const qint64 minutes = (secondsLeft % 3600) / 60;
        if (days > 0) {
            relative = QStringLiteral("%1 天后到期").arg(days);
        } else if (hours > 0) {
            relative = QStringLiteral("%1 小时后到期").arg(hours);
        } else {
            relative = QStringLiteral("%1 分钟后到期").arg(minutes);
        }
    }

    return expDate.toString("MM-dd hh:mm") + "\n" + relative;
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
    QAction *uploadDriveAction = menu.addAction("上传到网盘");

    // 连接槽函数
    connect(previewAction, &QAction::triggered, this, &TagList::onPreviewAction);
    connect(openLocationAction, &QAction::triggered, this, &TagList::onOpenLocationAction);
    connect(openInFileSystemAction, &QAction::triggered, this, &TagList::onOpenInFileSystemAction);
    connect(detailAction, &QAction::triggered, this, &TagList::onDetailAction);
    connect(modifyPathAction, &QAction::triggered, this, &TagList::onModifyPathAction);
    connect(deleteTagAction, &QAction::triggered, this, &TagList::onDeleteTagAction);
    connect(uploadDriveAction, &QAction::triggered, this, &TagList::onUploadToDriveAction);

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
    // 单例入口:同一文件复用同一个详情窗口,保存后的刷新由 DataChangeNotifier 统一触发
    TagDetail::showForFile(fileInfo);
}

void TagList::onUploadToDriveAction()
{
    if (!QFileInfo(fileInfo.filePath).isFile()) {
        m_messagePopup->showMessage("文件不存在或已缺失，无法上传到网盘", true);
        return;
    }

    QString uploadError;
    if (DriveUploadHelper::uploadToCurrentDirectory(this, fileInfo.filePath, &uploadError)) {
        m_messagePopup->showMessage(QString("已开始上传到网盘（网盘当前位置）：%1").arg(QFileInfo(fileInfo.filePath).fileName()));
    } else if (!uploadError.isEmpty()) {
        m_messagePopup->showMessage(uploadError, true);
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
            // 通知备忘标签页自动刷新（本条目会按新路径重建）
            DataChangeNotifier::Instance().notifyTagChanged(newPath);
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
            // 通知备忘标签页自动刷新，从列表中移除该条目
            DataChangeNotifier::Instance().notifyTagChanged(fileInfo.filePath);
        } else {
            m_messagePopup->showMessage("删除过程中出现错误", true);
        }
    }
}

