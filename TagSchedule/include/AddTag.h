#ifndef ADDTAG_H
#define ADDTAG_H

#include <QDialog>
#include <QDateTime>
#include <QString>
#include "../../manager/include/dbService.h"

namespace Ui {
class AddTag;
}

// 打标对话框:默认只需填一个标签名,提醒相关字段在选择了提醒方式后才出现。
// 目标文件已带标签时预填现有内容,同一个窗口复用为"修改标签"。
class AddTag : public QDialog
{
    Q_OBJECT

public:
    explicit AddTag(QWidget *parent = nullptr);

    void setTargetFile(const QString &filePath); // 显示目标文件,并预填已有标签

    QString getTagName() const;
    QString getAnnotation() const;
    QDateTime getExpirationDate() const;
    int getReminderTime() const;      // 提前多久提醒(小时)
    int getIntervalTime() const;      // 每次提醒间隔时间(分钟)
    QString getReminderType() const;  // 提醒方式
    ~AddTag();

private slots:
    void on_saveButton_clicked();
    void on_cancelButton_clicked();
    void updateReminderFieldsVisibility();
    void updateSaveButtonState();

private:
    Ui::AddTag *ui;
    dbService& dbservice;
    QString filePath;
};

#endif // ADDTAG_H
