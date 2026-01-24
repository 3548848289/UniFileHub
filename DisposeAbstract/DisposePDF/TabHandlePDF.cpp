#include "TabHandlePDF.h"
#include "ControlWidPDF.h"
#include <QDir>
#include <QBuffer>
#include <QSplitter>

// ===== Qt PDF 相关（Android 不支持，全部禁用） =====
// #include <QPdfDocument>
// #include <QPdfView>
// #include <QPdfPageNavigator>

TabHandlePDF::TabHandlePDF(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent),
    isShowControl(false)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ===== PDF 核心对象（禁用） =====
    // pdfDoc = new QPdfDocument(this);
    // pdfView = new QPdfView(this);
    // pdfView->setDocument(pdfDoc);
    // pdfView->setPageMode(QPdfView::PageMode::MultiPage);
    // pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);
    // pdfView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 控制窗口（保留）
    controlWidPDF = new ControlWidPDF(this);

    splitter = new QSplitter(Qt::Vertical, this);

    // ===== PDF View 禁用 =====
    // splitter->addWidget(pdfView);

    splitter->addWidget(controlWidPDF);
    splitter->setSizes({800, 60});

    mainLayout->addWidget(splitter);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    setContentModified(false);

    // 信号槽（逻辑保留，函数体内部已空实现）
    connect(controlWidPDF, &ControlWidPDF::prevPageRequested,
            this, &TabHandlePDF::goToPrevPage);
    connect(controlWidPDF, &ControlWidPDF::nextPageRequested,
            this, &TabHandlePDF::goToNextPage);
    connect(controlWidPDF, &ControlWidPDF::pageChanged,
            this, &TabHandlePDF::goToPage);
    connect(controlWidPDF, &ControlWidPDF::zoomModeChanged,
            this, &TabHandlePDF::changeZoomMode);

    // ===== PDF 加载禁用 =====
    // if (!filePath.isEmpty()) {
    //     loadFromFile(filePath);
    // }
}

void TabHandlePDF::setContent(const QString &text)
{
    Q_UNUSED(text);
}

QString TabHandlePDF::getContent() const
{
    return QString();
}

// ===== PDF 文件加载（禁用） =====
void TabHandlePDF::loadFromFile(const QString &fileName)
{
    Q_UNUSED(fileName);
}

// ===== 网络 PDF 加载（禁用） =====
void TabHandlePDF::loadFromInternet(const QByteArray &content)
{
    Q_UNUSED(content);
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

// ===== 页面控制逻辑（全部空实现） =====
void TabHandlePDF::goToPage(int pageNumber)
{
    Q_UNUSED(pageNumber);
}

void TabHandlePDF::goToPrevPage()
{
}

void TabHandlePDF::goToNextPage()
{
}

void TabHandlePDF::changeZoomMode(const QString &mode)
{
    Q_UNUSED(mode);
}
