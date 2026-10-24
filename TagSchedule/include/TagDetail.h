#ifndef TAGDETAIL_H
#define TAGDETAIL_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QHash>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QString>
#include <QScrollBar>
class QDateTimeEdit;
class QLineEdit;
class QPlainTextEdit;
#include "../../manager/include/dbService.h"
#include "../../manager/include/InlineMessagePopup.h"

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

    // 单例入口:同一文件路径复用同一个详情窗口,避免点几次就堆几个窗口
    static TagDetail *showForFile(const FilePathInfo &fileInfo);
    static TagDetail *showForPath(const QString &filePath);

signals:
    void tagDeleted(const QString &filePath);

private:
    void init(FilePathInfo info);
    void registerInstance(const QString &key);
    void rekeyInstance(const QString &oldKey, const QString &newKey);
    static QHash<QString, TagDetail *> &instances();

    Ui::TagDetail *ui;
    dbService& dbservice;    // dbService 单例引用
    FilePathInfo fileInfo;  // 引用传递的文件路径信息
    QString oldFilePath;    // 原文件路径，用于比较是否修改
    QString registryKey;    // 当前在单例注册表中的键
    InlineMessagePopup *m_messagePopup = nullptr;

    QLineEdit *tagNameEdit = nullptr;
    QDateTimeEdit *expirationEdit = nullptr;
    QPlainTextEdit *annotationEdit = nullptr;

private slots:
    void on_choosePathBtn_clicked();
    void on_YesBtn_clicked();
};

#endif // TAGDETAIL_H
