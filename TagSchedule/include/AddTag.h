#ifndef ADDTAG_H
#define ADDTAG_H

#include <QDialog>
#include <QDateTime>
namespace Ui {
class AddTag;
}

class AddTag : public QDialog
{
    Q_OBJECT

public:
    explicit AddTag(QWidget *parent = nullptr);

    QString getTagName() const;
    QString getAnnotation() const;
    QDateTime getExpirationDate() const;
    int getReminderTime() const;      // 提前多久提醒(小时)
    int getIntervalTime() const;      // 每次提醒间隔时间(分钟)
    QString getReminderType() const;  // 提醒方式
    ~AddTag();

private slots:
    void on_saveButton_clicked();

private:
    Ui::AddTag *ui;
};

#endif // ADDTAG_H
