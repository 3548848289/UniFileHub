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
    QTime getReminderTime() const;      // 提前多久提醒
    QTime getIntervalTime() const;      // 每次提醒间隔时间
    ~AddTag();

private slots:
    void on_saveButton_clicked();

private:
    Ui::AddTag *ui;
};

#endif // ADDTAG_H
