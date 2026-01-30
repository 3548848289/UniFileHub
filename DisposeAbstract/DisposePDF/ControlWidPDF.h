#ifndef CONTROLWIDPDF_H
#define CONTROLWIDPDF_H

#include <QWidget>

namespace Ui {
class ControlWidPDF;
}

class ControlWidPDF : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWidPDF(QWidget *parent = nullptr);
    ~ControlWidPDF();
    
    // 设置总页数
    void setTotalPages(int totalPages);
    // 设置当前页码
    void setCurrentPage(int pageNumber);
    // 获取当前页码
    int getCurrentPage() const;

signals:
    // 翻页信号
    void prevPageRequested();
    void nextPageRequested();
    // 页码跳转信号
    void pageChanged(int pageNumber);
    // 缩放模式变化信号
    void zoomModeChanged(const QString &mode);

private slots:
    // 按钮点击槽函数
    void on_prevPageButton_clicked();
    void on_nextPageButton_clicked();
    // 页码输入框变化槽函数
    void on_pageLineEdit_editingFinished();
    // 缩放模式变化槽函数
    void on_zoomComboBox_currentTextChanged(const QString &arg1);

private:
    Ui::ControlWidPDF *ui;
    int m_totalPages;
};

#endif // CONTROLWIDPDF_H
