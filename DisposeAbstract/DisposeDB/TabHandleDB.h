#ifndef TABHANDLEDB_H
#define TABHANDLEDB_H

#include "../../main/include/TabAbstract.h"
#include <QTableView>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QtSql/QSql>
#include <QtSql/QSqlQueryModel>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QStandardItemModel>

// 前向声明
class ControlWidDB;

class TabHandleDB : public TabAbstract
{
    Q_OBJECT

public:
    explicit TabHandleDB(const QString &filePath, QWidget *parent = nullptr);
    ~TabHandleDB();

    // 必须实现 TabAbstract 的纯虚函数
    void setContent(const QString &text) override;            // DB 文件无需编辑，空实现
    QString getContent() const override;                      // 返回空
    void loadFromFile(const QString &fileName) override;       // 加载本地文件
    void loadFromInternet(const QByteArray &content) override; // 加载网络内容
    void saveToFile(const QString &fileName) override;         // 空实现
    void ControlWidget(bool judge) override;

private slots:
    // 数据库操作相关的槽函数
    void onTableSelectionChanged(const QModelIndex &index);
    void onExecuteQuery(const QString &query);
    void refreshDatabase();

private:
    QSqlDatabase db;                 // 数据库连接
    QTableView *tableView;           // 表格视图显示查询结果
    QTreeView *treeView;             // 树形视图显示数据库结构
    QVBoxLayout *mainLayout;         // 主布局
    bool isShowControl;              // 控制窗口显示状态
    ControlWidDB *controlWidDB;      // 控制窗口
    QSplitter *splitter;             // 分割器
    QSqlQueryModel *queryModel;      // 查询结果模型
    QStandardItemModel *treeModel;   // 树形模型

    // 私有方法
    void setupDatabaseConnection(const QString &fileName);
    void populateDatabaseTree();
    void displayTable(const QString &tableName);
    void closeDatabaseConnection();
};

#endif // TABHANDLEDB_H
