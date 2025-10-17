#include "TabHandleDB.h"
#include "ControlWidDB.h"
#include <QDir>
#include <QBuffer>
#include <QMessageBox>
#include <QHeaderView>

TabHandleDB::TabHandleDB(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent),
    isShowControl(false)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 创建查询模型和表格视图
    queryModel = new QSqlQueryModel(this);
    tableView = new QTableView(this);
    tableView->setModel(queryModel);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 创建树形视图显示数据库结构
    treeModel = new QStandardItemModel(this);
    treeModel->setHorizontalHeaderLabels({tr("数据库结构")});
    treeView = new QTreeView(this);
    treeView->setModel(treeModel);
    treeView->setFixedWidth(200);
    treeView->header()->setSectionResizeMode(QHeaderView::Stretch);

    // 创建水平分割器
    QSplitter *horizontalSplitter = new QSplitter(Qt::Horizontal, this);
    horizontalSplitter->addWidget(treeView);
    horizontalSplitter->addWidget(tableView);
    horizontalSplitter->setSizes({200, 600});

    // 创建控制窗口
    controlWidDB = new ControlWidDB(this);

    // 创建垂直分割器
    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(horizontalSplitter);
    splitter->addWidget(controlWidDB);
    splitter->setSizes({600, 100});

    mainLayout->addWidget(splitter);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 连接信号槽
    connect(treeView, &QTreeView::clicked, this, &TabHandleDB::onTableSelectionChanged);
    connect(controlWidDB, &ControlWidDB::executeQuery, this, &TabHandleDB::onExecuteQuery);
    connect(controlWidDB, &ControlWidDB::refreshRequested, this, &TabHandleDB::refreshDatabase);

    // 如果提供了文件路径，则加载文件
    if (!filePath.isEmpty()) {
        loadFromFile(filePath);
    }
}

TabHandleDB::~TabHandleDB()
{
    closeDatabaseConnection();
}

void TabHandleDB::setContent(const QString &text)
{
    Q_UNUSED(text);
    // DB 文件通常不直接编辑文本内容
}

QString TabHandleDB::getContent() const
{
    return QString();
    // 返回空，因为 DB 文件不是纯文本
}

void TabHandleDB::loadFromFile(const QString &fileName)
{
    // 首先关闭可能存在的连接
    closeDatabaseConnection();
    
    // 设置当前文件路径
    setCurrentFilePath(fileName);
    
    // 设置数据库连接
    setupDatabaseConnection(fileName);
    
    // 填充数据库结构树
    if (db.isOpen()) {
        populateDatabaseTree();
    }
}

void TabHandleDB::loadFromInternet(const QByteArray &content)
{
    // 创建临时文件
    QString tempPath = QDir::tempPath() + "/temp_db_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".db";
    QFile tempFile(tempPath);
    
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(content);
        tempFile.close();
        
        // 加载临时文件
        loadFromFile(tempPath);
        
        // 设置一个标志，以便在关闭时删除临时文件
        // 注意：实际应用中可能需要更复杂的临时文件管理
    } else {
        QMessageBox::warning(this, tr("加载失败"), tr("无法创建临时文件"));
    }
}

void TabHandleDB::saveToFile(const QString &fileName)
{
    Q_UNUSED(fileName);
    // DB 文件查看器通常不支持直接保存，除非需要实现导出功能
    QMessageBox::information(this, tr("提示"), tr("数据库文件查看器不支持直接保存"));
}

void TabHandleDB::ControlWidget(bool judge)
{
    isShowControl = judge;
    controlWidDB->setVisible(judge);
}

void TabHandleDB::onTableSelectionChanged(const QModelIndex &index)
{
    if (!index.isValid()) return;
    
    QStandardItem *item = treeModel->itemFromIndex(index);
    if (!item || item->parent() == nullptr) return; // 跳过顶层项目
    
    QString tableName = item->text();
    displayTable(tableName);
}

void TabHandleDB::onExecuteQuery(const QString &query)
{
    if (!db.isOpen()) {
        QMessageBox::warning(this, tr("查询失败"), tr("数据库未打开"));
        return;
    }
    
    queryModel->setQuery(query, db);
    
    if (queryModel->lastError().isValid()) {
        QMessageBox::warning(this, tr("查询错误"), queryModel->lastError().text());
    }
}

void TabHandleDB::refreshDatabase()
{
    QString currentFile = getCurrentFilePath();
    if (!currentFile.isEmpty()) {
        loadFromFile(currentFile);
    }
}

void TabHandleDB::setupDatabaseConnection(const QString &fileName)
{
    // 为避免连接名冲突，使用唯一标识
    QString connectionName = "db_viewer_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    
    // 关闭同名连接（如果存在）
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::database(connectionName).close();
        QSqlDatabase::removeDatabase(connectionName);
    }
    
    // 创建新的数据库连接
    db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(fileName);
    
    // 打开数据库
    if (!db.open()) {
        QMessageBox::warning(this, tr("打开失败"), 
                            tr("无法打开数据库文件：%1\n%2").arg(fileName).arg(db.lastError().text()));
    }
}

void TabHandleDB::populateDatabaseTree()
{
    treeModel->clear();
    treeModel->setHorizontalHeaderLabels({tr("数据库结构")});
    
    QStandardItem *rootItem = new QStandardItem(tr("表格"));
    treeModel->appendRow(rootItem);
    
    // 查询所有表格
    QSqlQuery query("SELECT name FROM sqlite_master WHERE type='table';", db);
    while (query.next()) {
        QString tableName = query.value(0).toString();
        QStandardItem *tableItem = new QStandardItem(tableName);
        rootItem->appendRow(tableItem);
    }
    
    treeView->expandAll();
}

void TabHandleDB::displayTable(const QString &tableName)
{
    queryModel->setQuery("SELECT * FROM " + tableName, db);
    
    if (queryModel->lastError().isValid()) {
        QMessageBox::warning(this, tr("查询错误"), queryModel->lastError().text());
    }
}

void TabHandleDB::closeDatabaseConnection()
{
    if (db.isOpen()) {
        QString connectionName = db.connectionName();
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
    }
}