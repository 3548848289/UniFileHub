#include "./include/AddTag.h"
#include "ui/ui_AddTag.h"

AddTag::AddTag(QWidget *parent) :QDialog(parent),ui(new Ui::AddTag)
{
    ui->setupUi(this);
    QDateTime newDateTime = QDateTime::currentDateTime().addDays(1);
    ui->dateTimeEdit->setDateTime(newDateTime);
    
    // 设置默认提醒时间为提前1小时
    ui->reminderTimeEdit->setTime(QTime(1, 0, 0));
    
    // 设置默认提醒间隔为10分钟
    ui->intervalTimeEdit->setTime(QTime(0, 10, 0));

}

AddTag::~AddTag()
{
    delete ui;
}

QString AddTag::getTagName() const
{
    return ui->tagsLineEdit->text();
}

QString AddTag::getAnnotation() const
{
    return ui->annotationTextEdit->toPlainText();
}

QDateTime AddTag::getExpirationDate() const
{
    return ui->dateTimeEdit->dateTime();
}

QTime AddTag::getReminderTime() const
{
    return ui->reminderTimeEdit->time();
}

QTime AddTag::getIntervalTime() const
{
    return ui->intervalTimeEdit->time();
}

void AddTag::on_saveButton_clicked()
{
    accept();
}

