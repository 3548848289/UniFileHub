#include "./include/AddTag.h"
#include "ui/ui_AddTag.h"

AddTag::AddTag(QWidget *parent) :QDialog(parent),ui(new Ui::AddTag)
{
    ui->setupUi(this);
    QDateTime newDateTime = QDateTime::currentDateTime().addDays(1);
    ui->dateTimeEdit->setDateTime(newDateTime);

    ui->reminderTimeEdit->setValue(24);
    ui->intervalTimeEdit->setValue(1440);
    ui->reminderTypeComboBox->setCurrentIndex(1); // 默认选择"弹窗提醒"
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

int AddTag::getReminderTime() const
{
    return ui->reminderTimeEdit->value();
}

int AddTag::getIntervalTime() const
{
    return ui->intervalTimeEdit->value();
}

QString AddTag::getReminderType() const
{
    return ui->reminderTypeComboBox->currentText();
}

void AddTag::on_saveButton_clicked()
{
    accept();
}

