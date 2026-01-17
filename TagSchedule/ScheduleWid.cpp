#include "./include/ScheduleWid.h"
#include "ui/ui_ScheduleWid.h"
#include "notifymanager.h"

ScheduleWid::ScheduleWid(QWidget *parent) : QWidget(parent), ui(new Ui::ScheduleWid),
    dbservice(dbService::instance("./SmartDesk.db"))
{
    ui->setupUi(this);

    on_refreshBtn_clicked("全部");
    loadTags();

    connect(ui->listWidget, &QListWidget::itemClicked, this, &ScheduleWid::onItemClicked);
    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, &ScheduleWid::onSearch);
    connect(ui->refreshBtn, &QPushButton::clicked, this, [this]() {
        on_refreshBtn_clicked("全部");
    });

    startExpirationCheck();    // 启动定时器检查到期文件

    int showTime = SettingManager::Instance().tag_schedule_show_time();
    manager = new NotifyManager(this);
    manager->setMaxCount(5);
    manager->setDisplayTime(showTime * 1000);
    manager->setNotifyWndSize(300, 80);
    connect(manager, &NotifyManager::notifyDetail, [](const QVariantMap &data){
        QMessageBox msgbox(QMessageBox::Information,
            QStringLiteral("具体信息"), data.value("title").toString());
        msgbox.setWindowIcon(QIcon::fromTheme("utilities-system-monitor"));
        QStringList tagList = data.value("tag").toStringList();

        QString allDetails = QString("%1\n\n标签: %2\n备注: %3\n到期时间: %4")
            .arg(data.value("body").toString()).arg(tagList.join(", "))
            .arg(data.value("annotation").toString()).arg(data.value("expirationDate").toString());

        msgbox.setInformativeText(allDetails);
        msgbox.findChild<QDialogButtonBox*>()->setMinimumWidth(500);
        msgbox.exec();
    });
}

ScheduleWid::~ScheduleWid()
{
    delete ui;
}


void ScheduleWid::onItemClicked(QListWidgetItem *item) {
    TagList *widget = qobject_cast<TagList *>(ui->listWidget->itemWidget(item));
    if (widget) {
        QString path = widget->getFilePath();
        if (QFile::exists(path)) {
            emit fileClicked(path);
        } else {
            QMessageBox::warning(this, tr("文件不存在"),
                                 tr("文件已被移动或删除：\n%1").arg(path));
        }
    }
}


void ScheduleWid::loadTags() {
    ui->comboBox->clear();
    ui->comboBox->addItem("全部");
    QStringList tags = dbservice.dbTags().getAllTags();
    for (const QString &tag : tags) {
        ui->comboBox->addItem(tag);
    }
}


void ScheduleWid::onSearch(const QString &keyword) {
    ui->listWidget->clear();
    QList<FilePathInfo> files = dbservice.dbTags().searchFiles(keyword);
    for (const FilePathInfo &fileInfo : files) {
        TagList *widget = new TagList(fileInfo);

        QListWidgetItem *listItem = new QListWidgetItem(ui->listWidget);
        listItem->setSizeHint(widget->sizeHint());
        ui->listWidget->addItem(listItem);
        ui->listWidget->setItemWidget(listItem, widget);
    }
}


void ScheduleWid::startExpirationCheck() {
    expirationTimer = new QTimer(this);
    connect(expirationTimer, &QTimer::timeout, this, &ScheduleWid::checkExpiration);
    // 设置定时器间隔为1分钟，实际提醒间隔由每个文件的intervalTime决定
    expirationTimer->start(60000);
}

void ScheduleWid::checkExpiration()
{
    qDebug() << "ScheduleWid::checkExpiration";

    QList<FilePathInfo> files = dbservice.dbTags().getFilePathsByTag("全部");
    QString reminderType = SettingManager::Instance().tag_schedule_reminder_type();
    const QDateTime now = QDateTime::currentDateTime();

    for (auto &file : files)
    {
        // 跳过没有设置提醒时间的文件
        if (file.reminderTime <= 0 || !file.expirationDate.isValid())
            continue;

        int reminderSeconds = file.reminderTime * 3600; // 小时转换为秒
        int intervalSeconds = file.intervalTime * 60; // 分钟转换为秒

        // 计算提醒开始时间（到期时间 - 提前提醒时间）
        QDateTime start = file.expirationDate.addSecs(-reminderSeconds);
        qint64 elapsed = start.secsTo(now);

        // 如果还没到提醒时间，跳过
        if (elapsed < 0 || now >= file.expirationDate)
            continue;

        // 计算当前应该提醒的索引
        int currentIndex = elapsed / intervalSeconds;

        // 如果当前索引大于已提醒的最大索引，需要提醒
        if (currentIndex > file.lastReminderIndex)
        {
            QVariantMap data;
            data["tag"] = file.tagName;
            data["annotation"] = file.annotation;
            data["expirationDate"] = file.expirationDate;

            if (reminderType == "弹窗提醒")
            {
                manager->notify("到期提醒", file.filePath, data);
            }
            else if (reminderType == "邮件提醒")
            {
                const QString allDetails = QString("标签: %1\n备注: %2\n到期时间: %3")
                                               .arg(file.tagName)
                                               .arg(file.annotation)
                                               .arg(file.expirationDate.toString());
                sendemail->sendEmailWithData(file.filePath, allDetails, QStringList());
            }

            // 更新数据库中的lastReminderIndex
            dbservice.dbTags().updateLastReminderIndex(file.filePath, currentIndex);
        }
    }
}

void ScheduleWid::on_comboBox_currentIndexChanged(int index) {
    QString tag = ui->comboBox->currentText();
    on_refreshBtn_clicked(tag);
}


void ScheduleWid::on_sortComboBox_currentIndexChanged(int index)
{
    if(index == 0 || index == 1)
    {
        ui->listWidget->clear();
        QList<FilePathInfo> files = dbservice.dbTags().getFilePathsByTag("全部");

        bool isAscending = (index == 0);
        std::sort(files.begin(), files.end(), [isAscending](const FilePathInfo &a, const FilePathInfo &b) {
            return isAscending ? a.expirationDate < b.expirationDate : a.expirationDate > b.expirationDate;
        });

        for (const FilePathInfo &file : files) {
            TagList *widget = new TagList(file);

            QListWidgetItem *listItem = new QListWidgetItem(ui->listWidget);
            listItem->setSizeHint(widget->sizeHint());
            ui->listWidget->addItem(listItem);
            ui->listWidget->setItemWidget(listItem, widget);
        }
    }
}



void ScheduleWid::on_refreshBtn_clicked(const QString &tag = "全部")
{
    if(tag == "全部")
        ui->comboBox->setCurrentIndex(0);
    ui->listWidget->clear();
    QList<FilePathInfo> files = dbservice.dbTags().getFilePathsByTag(tag);

    for (const auto &info : files) {
        TagList *taglist = new TagList(info);

        QListWidgetItem *listItem = new QListWidgetItem(ui->listWidget);
        listItem->setSizeHint(taglist->sizeHint());
        ui->listWidget->addItem(listItem);
        ui->listWidget->setItemWidget(listItem, taglist);
    }

}

