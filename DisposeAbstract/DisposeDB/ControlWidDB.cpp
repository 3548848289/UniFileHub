#include "ControlWidDB.h"
#include "ui_ControlWidDB.h"
#include <QKeyEvent>
ControlWidDB::ControlWidDB(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::ControlWidDB)
{
    ui->setupUi(this);
    
    // 获取UI控件的指针
    queryEdit = ui->queryEdit;
    executeButton = ui->executeButton;
    refreshButton = ui->refreshButton;
    
    // 设置默认SQL查询
    queryEdit->setPlaceholderText("输入SQL查询，例如: SELECT * FROM table_name");
    
    // 连接信号槽
    connect(executeButton, &QPushButton::clicked, this, &ControlWidDB::onExecuteButtonClicked);
    connect(refreshButton, &QPushButton::clicked, this, &ControlWidDB::onRefreshButtonClicked);
    
    // 设置快捷键
    queryEdit->setToolTip("按Ctrl+Enter执行查询");
    queryEdit->installEventFilter(this);
}

ControlWidDB::~ControlWidDB()
{
    delete ui;
}

void ControlWidDB::onExecuteButtonClicked()
{
    QString query = queryEdit->text().trimmed();
    if (!query.isEmpty()) {
        emit executeQuery(query);
    }
}

void ControlWidDB::onRefreshButtonClicked()
{
    emit refreshRequested();
}

// 重写eventFilter以支持Ctrl+Enter快捷键
bool ControlWidDB::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == queryEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_Return) {
            onExecuteButtonClicked();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
