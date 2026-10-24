#include "./include/AddTag.h"
#include "ui/ui_AddTag.h"

#include <QFileInfo>
#include <QFontMetrics>
#include <QLineEdit>
#include <QDateTimeEdit>

namespace {
constexpr int kTargetLabelWidth = 360; // 目标文件名的省略宽度,配合 tooltip 显示完整路径
}

AddTag::AddTag(QWidget *parent) : QDialog(parent), ui(new Ui::AddTag),
    dbservice(dbService::instance("./SmartDesk.db"))
{
    ui->setupUi(this);
    QDateTime newDateTime = QDateTime::currentDateTime().addDays(1);
    ui->dateTimeEdit->setDateTime(newDateTime);

    ui->reminderTimeEdit->setValue(24);
    ui->intervalTimeEdit->setValue(1440);
    // 默认"不提醒":快速打一个标签不应该默认就挂上弹窗提醒
    ui->reminderTypeComboBox->setCurrentIndex(0);

    // 已有标签列表:输入时自动补全,避免同一概念被拼成多个标签
    ui->tagsComboBox->addItems(dbservice.dbTags().getAllTags());
    ui->tagsComboBox->setCurrentIndex(-1);

    connect(ui->tagsComboBox, &QComboBox::currentTextChanged,
            this, &AddTag::updateSaveButtonState);
    connect(ui->tagsComboBox->lineEdit(), &QLineEdit::returnPressed, this, [this]() {
        if (ui->saveButton->isEnabled()) {
            on_saveButton_clicked();
        }
    });
    connect(ui->reminderTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateReminderFieldsVisibility(); });

    updateReminderFieldsVisibility();
    updateSaveButtonState();
}

AddTag::~AddTag()
{
    delete ui;
}

void AddTag::setTargetFile(const QString &target)
{
    filePath = target;
    if (filePath.isEmpty()) {
        ui->targetFileLabel->hide();
        return;
    }

    const QFileInfo info(filePath);
    const QFontMetrics fm(ui->targetFileLabel->font());
    ui->targetFileLabel->setText(fm.elidedText(info.fileName(), Qt::ElideMiddle, kTargetLabelWidth));
    ui->targetFileLabel->setToolTip(filePath);

    FilePathInfo existing;
    if (dbservice.dbTags().getFileInfoByFilePath(filePath, existing)
        && !existing.tagName.trimmed().isEmpty()) {
        // 文件已带标签:预填现有内容,窗口复用为"修改标签"
        setWindowTitle(QStringLiteral("修改标签"));
        ui->tagsComboBox->setEditText(existing.tagName.trimmed());
        if (existing.expirationDate.isValid()) {
            ui->dateTimeEdit->setDateTime(existing.expirationDate);
        }
        ui->annotationTextEdit->setPlainText(existing.annotation);
        const int typeIndex = ui->reminderTypeComboBox->findText(existing.reminderType);
        ui->reminderTypeComboBox->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
        if (existing.reminderTime > 0) {
            ui->reminderTimeEdit->setValue(existing.reminderTime);
        }
        if (existing.intervalTime > 0) {
            ui->intervalTimeEdit->setValue(existing.intervalTime);
        }
    }
}

QString AddTag::getTagName() const
{
    return ui->tagsComboBox->currentText().trimmed();
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
    if (getTagName().isEmpty()) {
        return;
    }
    accept();
}

void AddTag::on_cancelButton_clicked()
{
    reject();
}

void AddTag::updateReminderFieldsVisibility()
{
    // "不提醒"时隐藏两个提醒参数,表单只剩最常用的三项
    const bool reminderEnabled = ui->reminderTypeComboBox->currentIndex() != 0;
    ui->reminderWidget->setVisible(reminderEnabled);
    adjustSize();
}

void AddTag::updateSaveButtonState()
{
    ui->saveButton->setEnabled(!getTagName().isEmpty());
}
