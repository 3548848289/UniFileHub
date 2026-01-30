#include "ControlWidPDF.h"
#include "ui_ControlWidPDF.h"

ControlWidPDF::ControlWidPDF(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlWidPDF),
    m_totalPages(0)
{
    ui->setupUi(this);

}

ControlWidPDF::~ControlWidPDF()
{
    delete ui;
}

// 设置总页数
void ControlWidPDF::setTotalPages(int totalPages)
{
    m_totalPages = totalPages;
    ui->totalPagesLabel->setText(QString("/ %1").arg(totalPages));

    // 禁用或启用按钮
    ui->prevPageButton->setEnabled(totalPages > 1);
    ui->nextPageButton->setEnabled(totalPages > 1);
}

// 设置当前页码
void ControlWidPDF::setCurrentPage(int pageNumber)
{
    QSignalBlocker blocker(ui->pageLineEdit);  // 自动在作用域结束时恢复信号
    ui->pageLineEdit->setText(QString::number(pageNumber));

    ui->prevPageButton->setEnabled(pageNumber > 1);
    ui->nextPageButton->setEnabled(pageNumber < m_totalPages);
}

// 获取当前页码
int ControlWidPDF::getCurrentPage() const
{
    bool ok;
    int page = ui->pageLineEdit->text().toInt(&ok);
    return ok ? page : 1;
}

// 上一页按钮点击
void ControlWidPDF::on_prevPageButton_clicked()
{
    emit prevPageRequested();
}

// 下一页按钮点击
void ControlWidPDF::on_nextPageButton_clicked()
{
    emit nextPageRequested();
}

// 页码输入完成
void ControlWidPDF::on_pageLineEdit_editingFinished()
{
    bool ok;
    int page = ui->pageLineEdit->text().toInt(&ok);

    // 验证输入是否有效
    if (!ok || page < 1) {
        page = 1;
    } else if (page > m_totalPages && m_totalPages > 0) {
        page = m_totalPages;
    }

    // 更新显示
    ui->pageLineEdit->setText(QString::number(page));

    // 发送页码变化信号
    emit pageChanged(page);

    // 更新按钮状态
    ui->prevPageButton->setEnabled(page > 1);
    ui->nextPageButton->setEnabled(page < m_totalPages);
}

// 缩放模式变化
void ControlWidPDF::on_zoomComboBox_currentTextChanged(const QString &arg1)
{
    emit zoomModeChanged(arg1);
}
