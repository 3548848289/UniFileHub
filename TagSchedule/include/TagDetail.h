#ifndef TAGDETAIL_H
#define TAGDETAIL_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QString>
#include <QScrollBar>
#include "../../manager/include/dbService.h"

namespace Ui {
class TagDetail;
}

class TagDetail : public QWidget
{
    Q_OBJECT

public:
    TagDetail(QWidget *parent, FilePathInfo fileInfo);

    TagDetail(QWidget *parent, QString filePath);
    ~TagDetail();
    
signals:
    void tagDeleted(const QString &filePath);
    
private:
    void init( FilePathInfo fileInfo);
    void updateFileInfo();

    Ui::TagDetail *ui;
    dbService& dbservice;    // dbService 单例引用
    FilePathInfo fileInfo;  // 引用传递的文件路径信息
    QString oldFilePath;    // 原文件路径，用于比较是否修改

private slots:
    void on_choosePathBtn_clicked();
    void on_YesBtn_clicked();
};

#endif // TAGDETAIL_H
