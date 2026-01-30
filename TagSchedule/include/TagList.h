#ifndef TAGLIST_H
#define TAGLIST_H

#include <QWidget>
#include <QString>
#include <QFileInfo>
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QTableWidget>
#include "TagDetail.h"

#include "../../manager/include/dbService.h"

namespace Ui {
class TagList;
}


class TagList : public QWidget
{
    Q_OBJECT

public:
    explicit TagList(const FilePathInfo &fileInfo, QWidget *parent = nullptr);
    ~TagList();

    // 获取文件路径
    QString getFilePath() const;

    QString getExpInfo(const QDateTime expDate);
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onPreviewAction();
    void onOpenLocationAction();
    void onOpenInFileSystemAction();
    void onDetailAction();
    void onModifyPathAction();
    void onDeleteTagAction();

signals:
    void openInFileSystemRequested(const QString &filePath);

private:
    Ui::TagList *ui;
    FilePathInfo fileInfo;
    TagDetail *tagdetail = nullptr; // 标签详情窗口实例
    dbService& dbservice;    // dbService 单例引用
};

#endif // TAGLIST_H
