#include "TabHandlePDF.h"
#include "ControlWidPDF.h"
#include <QDir>
#include <QBuffer>
#include <QSplitter>
#include <QPdfPageNavigator>

TabHandlePDF::TabHandlePDF(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent),
    isShowControl(false)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    pdfDoc = new QPdfDocument(this);
    pdfView = new QPdfView(this);
    pdfView->setDocument(pdfDoc);
    pdfView->setPageMode(QPdfView::PageMode::MultiPage);
    pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);
    pdfView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 创建控制窗口
    controlWidPDF = new ControlWidPDF(this);

    // 创建分割器并添加PDF视图和控制窗口
    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(pdfView);
    splitter->addWidget(controlWidPDF);
    splitter->setSizes({800, 60});

    mainLayout->addWidget(splitter);
    mainLayout->setContentsMargins(0,0,0,0);

    setContentModified(false);

    // 连接信号槽
    connect(controlWidPDF, &ControlWidPDF::prevPageRequested, this, &TabHandlePDF::goToPrevPage);
    connect(controlWidPDF, &ControlWidPDF::nextPageRequested, this, &TabHandlePDF::goToNextPage);
    connect(controlWidPDF, &ControlWidPDF::pageChanged, this, &TabHandlePDF::goToPage);
    connect(controlWidPDF, &ControlWidPDF::zoomModeChanged, this, &TabHandlePDF::changeZoomMode);

    if (!filePath.isEmpty()) {
        loadFromFile(filePath);
    }
}

void TabHandlePDF::setContent(const QString &text)
{
    Q_UNUSED(text);
}

QString TabHandlePDF::getContent() const
{
    return QString();
}

void TabHandlePDF::loadFromFile(const QString &fileName)
{
    QPdfDocument::Error err = pdfDoc->load(fileName);
    if (err != QPdfDocument::Error::None) {
        QMessageBox::warning(this, tr("打开失败"),
                             tr("无法加载 PDF 文件：%1").arg(fileName));
        return;
    }

    pdfView->setPageMode(QPdfView::PageMode::SinglePage);
    pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);

    // 更新控制窗口的页码信息
    int totalPages = pdfDoc->pageCount();
    controlWidPDF->setTotalPages(totalPages);
    controlWidPDF->setCurrentPage(1);
}


void TabHandlePDF::loadFromInternet(const QByteArray &content)
{
    QBuffer buffer;
    buffer.setData(content);
    if (!buffer.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("加载失败"), tr("无法打开内存缓冲区"));
        return;
    }

    pdfDoc->load(&buffer);

    if (pdfDoc->error() != QPdfDocument::Error::None) {
        QMessageBox::warning(this, tr("加载失败"), tr("无法加载 PDF 内容"));
        return;
    }

    pdfView->setPageMode(QPdfView::PageMode::SinglePage);
    pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);

    // 更新控制窗口的页码信息
    int totalPages = pdfDoc->pageCount();
    controlWidPDF->setTotalPages(totalPages);
    controlWidPDF->setCurrentPage(1);
}



void TabHandlePDF::saveToFile(const QString &fileName)
{
    Q_UNUSED(fileName);
}

void TabHandlePDF::ControlWidget(bool judge)
{
    isShowControl = judge;
    controlWidPDF->setVisible(judge);
}

// 跳转到指定页码
void TabHandlePDF::goToPage(int pageNumber)
{
    if (pageNumber < 1 || pageNumber > pdfDoc->pageCount())
        return;

    controlWidPDF->setCurrentPage(pageNumber);

    if (pdfView->pageNavigator()) {
        // QPdfPageNavigator使用0-based索引，而我们的界面使用1-based页码
        pdfView->pageNavigator()->jump(pageNumber - 1, QPointF(0,0), 1.0);
    }
}



// 上一页
void TabHandlePDF::goToPrevPage()
{
    int current = controlWidPDF->getCurrentPage();
    if (current > 1)
        goToPage(current - 1);
}

// 下一页
void TabHandlePDF::goToNextPage()
{
    int current = controlWidPDF->getCurrentPage();
    int total = pdfDoc->pageCount();
    if (current < total)
        goToPage(current + 1);
}


// 改变缩放模式
void TabHandlePDF::changeZoomMode(const QString &mode)
{
    if (mode == "适应窗口")
    {
        pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);
    }
    else if (mode == "适应宽度")
    {
        pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    }
    else if (mode.endsWith("%"))
    {
        // 处理百分比缩放
        pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
        int zoomPercentage = mode.left(mode.length() - 1).toInt();
        pdfView->setZoomFactor(zoomPercentage / 100.0);
    }
}
