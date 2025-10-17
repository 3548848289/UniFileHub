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
    ui->pageSpinBox->setMaximum(totalPages > 0 ? totalPages : 1);
    
    // 禁用或启用按钮
    ui->prevPageButton->setEnabled(totalPages > 1);
    ui->nextPageButton->setEnabled(totalPages > 1);
}

// 设置当前页码
void ControlWidPDF::setCurrentPage(int pageNumber)
{
    QSignalBlocker blocker(ui->pageSpinBox);  // 自动在作用域结束时恢复信号
    ui->pageSpinBox->setValue(pageNumber);

    ui->prevPageButton->setEnabled(pageNumber > 1);
    ui->nextPageButton->setEnabled(pageNumber < m_totalPages);
}

// 获取当前页码
int ControlWidPDF::getCurrentPage() const
{
    return ui->pageSpinBox->value();
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

// 页码变化
void ControlWidPDF::on_pageSpinBox_valueChanged(int arg1)
{
    emit pageChanged(arg1);
    
    // 更新按钮状态
    ui->prevPageButton->setEnabled(arg1 > 1);
    ui->nextPageButton->setEnabled(arg1 < m_totalPages);
}

// 缩放模式变化
void ControlWidPDF::on_zoomComboBox_currentTextChanged(const QString &arg1)
{
    emit zoomModeChanged(arg1);
}
