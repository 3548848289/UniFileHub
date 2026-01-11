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
    int reminderInterval = SettingManager::Instance().tag_schedule_interval_time();
    // 确保定时器间隔至少为1秒，防止过快触发
    int safeInterval = qMax(reminderInterval, 1);
    expirationTimer->start(safeInterval * 1000);
}

void ScheduleWid::checkExpiration() {
    QList<FilePathInfo> files = dbservice.dbTags().getFilePathsByTag("全部");

    QString reminderType = SettingManager::Instance().tag_schedule_reminder_type();

    for (const auto &file : files) {
        QString path = file.filePath;
        QDateTime expDate = file.expirationDate;

        // 使用每个文件自己的提前提醒时间（reminderTime）
        int reminderTimeInSeconds = 0;
        
        // 检查reminderTime是否有效，如果无效则使用默认值或跳过提醒
        if (file.reminderTime.isValid()) {
            reminderTimeInSeconds = file.reminderTime.hour() * 3600 + file.reminderTime.minute() * 60 + file.reminderTime.second();
        } else {
            // 如果reminderTime无效，跳过提醒或使用默认值
            continue; // 跳过此文件的提醒
        }

        if (expDate.isValid() && QDateTime::currentDateTime().secsTo(expDate) <= reminderTimeInSeconds) {
            // 只有当文件还未过期时才执行提醒
            if (QDateTime::currentDateTime() < expDate) {
                // 使用每个文件自己的提醒间隔时间（intervalTime）
                bool shouldRemind = false;
                
                if (file.intervalTime.isValid()) {
                    int intervalInSeconds = file.intervalTime.hour() * 3600 + file.intervalTime.minute() * 60 + file.intervalTime.second();
                    
                    if (intervalInSeconds > 0) {
                        // 检查距离上次提醒的时间是否已经超过了间隔时间
                        QDateTime lastTime = lastReminderTimeMap.value(path, QDateTime());
                        if (lastTime.isValid()) {
                            // 如果已经超过了间隔时间，才执行新的提醒
                            if (lastTime.secsTo(QDateTime::currentDateTime()) >= intervalInSeconds) {
                                shouldRemind = true;
                            }
                        } else {
                            // 如果是第一次提醒，直接执行
                            shouldRemind = true;
                        }
                    } else {
                        // 如果间隔时间为0，每次都提醒
                        shouldRemind = true;
                    }
                } else {
                    // 如果intervalTime无效，使用默认间隔或每次都提醒
                    shouldRemind = true;
                }
                
                if (shouldRemind) {
                    QVariantMap data;
                    data["tag"] = file.tagName;
                    data["annotation"] = file.annotation;
                    data["expirationDate"] = file.expirationDate;

                    const QString allDetails = QString("标签: %1\n\n备注: %2\n\n到期时间: %3")
                                                   .arg(data.value("tag").toString()) .arg(data.value("annotation").toString())
                                                   .arg(data.value("expirationDate").toString());

                    if (reminderType == "弹窗提醒") {
                        // 如果NotifyManager支持为每个通知设置不同的显示时间，可以在这里设置
                        // 否则，继续使用默认的显示时间
                        manager->notify("到期提醒", path, data);
                    } else if (reminderType == "邮件提醒") {
                        qDebug() << "邮件提醒";
                        sendemail->sendEmailWithData(path, allDetails, QStringList());
                    }
                    
                    // 更新上次提醒时间
                    lastReminderTimeMap[path] = QDateTime::currentDateTime();
                }
            }
        } else {
            // 如果文件已经过了提醒时间范围，清除上次提醒记录
            if (lastReminderTimeMap.contains(path)) {
                lastReminderTimeMap.remove(path);
            }
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

