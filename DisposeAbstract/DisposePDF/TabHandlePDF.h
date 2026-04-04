#ifndef TABHANDLEPDF_H
#define TABHANDLEPDF_H

#include "../../main/include/TabAbstract.h"
#include <QPdfDocument>
#include <QPdfView>
#include <QVBoxLayout>
#include <QFile>
#include <QMessageBox>
#include <QSplitter>

// 前向声明
class ControlWidPDF;

class TabHandlePDF : public TabAbstract
{
    Q_OBJECT

public:
    explicit TabHandlePDF(const QString &filePath, QWidget *parent = nullptr);

    // 必须实现 TabAbstract 的纯虚函数
    void setContent(const QString &text) override;            // PDF 无需编辑，空实现
    QString getContent() const override;                      // 返回空
    void loadFromFile(const QString &fileName) override;       // 加载本地文件
    void loadFromInternet(const QByteArray &content) override; // 加载网络内容
    void saveToFile(const QString &fileName) override;         // 空实现
    void ControlWidget(bool judge) override;

private slots:
    // 翻页相关的槽函数
    void goToPrevPage();
    void goToNextPage();
    void goToPage(int pageNumber);
    void changeZoomMode(const QString &mode);
    // 滚动模式变化槽函数
    void changeScrollMode(bool enabled);

private:
    QPdfDocument *pdfDoc;
    QPdfView *pdfView;
    QVBoxLayout *mainLayout;
    bool isShowControl;

    // 新增成员变量
    ControlWidPDF *controlWidPDF;
    QSplitter *splitter;
};

#endif // TABHANDLEPDF_H
