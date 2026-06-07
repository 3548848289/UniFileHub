#include "TabHandlePDF.h"
#include "ControlWidPDF.h"

#include <QBuffer>
#include <QPdfPageNavigator>
#include <QTimer>

TabHandlePDF::TabHandlePDF(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent)
    , pdfDoc(new QPdfDocument(this))
    , searchModel(new QPdfSearchModel(this))
    , pdfView(new QPdfView(this))
    , mainLayout(new QVBoxLayout(this))
    , isShowControl(false)
    , m_zoomPercentage(100)
    , m_currentSearchResultIndex(-1)
    , controlWidPDF(new ControlWidPDF(this))
    , splitter(new QSplitter(Qt::Vertical, this))
{
    mainLayout->setContentsMargins(0, 0, 0, 0);

    pdfView->setDocument(pdfDoc);
    pdfView->setSearchModel(searchModel);
    pdfView->setPageMode(QPdfView::PageMode::MultiPage);
    pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    pdfView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    searchModel->setDocument(pdfDoc);

    splitter->addWidget(pdfView);
    splitter->addWidget(controlWidPDF);
    splitter->setSizes({800, 92});

    mainLayout->addWidget(splitter);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    setContentModified(false);

    connect(controlWidPDF, &ControlWidPDF::prevPageRequested, this, &TabHandlePDF::goToPrevPage);
    connect(controlWidPDF, &ControlWidPDF::nextPageRequested, this, &TabHandlePDF::goToNextPage);
    connect(controlWidPDF, &ControlWidPDF::pageChanged, this, &TabHandlePDF::goToPage);
    connect(controlWidPDF, &ControlWidPDF::searchTextChanged, this, &TabHandlePDF::updateSearchText);
    connect(controlWidPDF, &ControlWidPDF::searchPreviousRequested, this, &TabHandlePDF::findPreviousSearchResult);
    connect(controlWidPDF, &ControlWidPDF::searchNextRequested, this, &TabHandlePDF::findNextSearchResult);
    connect(controlWidPDF, &ControlWidPDF::fitToWidthChanged, this, &TabHandlePDF::changeFitToWidth);
    connect(controlWidPDF, &ControlWidPDF::zoomValueChanged, this, &TabHandlePDF::changeZoomValue);
    connect(controlWidPDF, &ControlWidPDF::scrollModeChanged, this, &TabHandlePDF::changeScrollMode);
    connect(searchModel, &QPdfSearchModel::countChanged, this, &TabHandlePDF::updateSearchResultInfo);
    connect(searchModel, &QAbstractItemModel::modelReset, this, &TabHandlePDF::updateSearchResultInfo);
    connect(pdfView->pageNavigator(), &QPdfPageNavigator::currentPageChanged, this, [this](int page) {
        controlWidPDF->setCurrentPage(page + 1);
    });

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
    const QPdfDocument::Error err = pdfDoc->load(fileName);
    if (err != QPdfDocument::Error::None) {
        QMessageBox::warning(this, tr("打开失败"),
                             tr("无法加载 PDF 文件：%1").arg(fileName));
        return;
    }

    pdfView->setPageMode(QPdfView::PageMode::MultiPage);

    const int totalPages = pdfDoc->pageCount();
    controlWidPDF->setTotalPages(totalPages);
    controlWidPDF->setCurrentPage(1);

    m_zoomPercentage = 100;
    resetSearch();
    controlWidPDF->setZoomPercentage(m_zoomPercentage);
    controlWidPDF->setFitToWidthChecked(true);
    controlWidPDF->setPageModeChecked(false);

    QTimer::singleShot(100, this, [this]() {
        changeFitToWidth(true);
        pdfView->resize(pdfView->size());
        pdfView->update();
    });
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

    pdfView->setPageMode(QPdfView::PageMode::MultiPage);

    const int totalPages = pdfDoc->pageCount();
    controlWidPDF->setTotalPages(totalPages);
    controlWidPDF->setCurrentPage(1);

    m_zoomPercentage = 100;
    resetSearch();
    controlWidPDF->setZoomPercentage(m_zoomPercentage);
    controlWidPDF->setFitToWidthChecked(true);
    controlWidPDF->setPageModeChecked(false);

    QTimer::singleShot(100, this, [this]() {
        changeFitToWidth(true);
        pdfView->resize(pdfView->size());
        pdfView->update();
    });
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

void TabHandlePDF::goToPage(int pageNumber)
{
    if (pageNumber < 1 || pageNumber > pdfDoc->pageCount()) {
        return;
    }

    controlWidPDF->setCurrentPage(pageNumber);

    if (pdfView->pageNavigator()) {
        pdfView->pageNavigator()->jump(pageNumber - 1, QPointF(0, 0), 1.0);
    }
}

void TabHandlePDF::goToPrevPage()
{
    const int current = controlWidPDF->getCurrentPage();
    if (current > 1) {
        goToPage(current - 1);
    }
}

void TabHandlePDF::goToNextPage()
{
    const int current = controlWidPDF->getCurrentPage();
    const int total = pdfDoc->pageCount();
    if (current < total) {
        goToPage(current + 1);
    }
}

void TabHandlePDF::changeFitToWidth(bool enabled)
{
    if (enabled) {
        pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);
        pdfView->update();
        return;
    }

    changeZoomValue(m_zoomPercentage);
}

void TabHandlePDF::changeZoomValue(int zoomPercentage)
{
    if (zoomPercentage < 50) {
        zoomPercentage = 50;
    } else if (zoomPercentage > 200) {
        zoomPercentage = 200;
    }

    m_zoomPercentage = zoomPercentage;
    pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
    pdfView->setZoomFactor(m_zoomPercentage / 100.0);
    pdfView->update();
}

void TabHandlePDF::changeScrollMode(bool enabled)
{
    if (enabled) {
        pdfView->setPageMode(QPdfView::PageMode::SinglePage);
    } else {
        pdfView->setPageMode(QPdfView::PageMode::MultiPage);
    }
}

void TabHandlePDF::updateSearchText(const QString &text)
{
    m_currentSearchResultIndex = -1;
    pdfView->setCurrentSearchResultIndex(-1);
    searchModel->setSearchString(text.trimmed());
    controlWidPDF->setSearchResultInfo(-1, 0);
}

void TabHandlePDF::findPreviousSearchResult()
{
    const int totalResults = searchModel->rowCount({});
    if (totalResults <= 0) {
        return;
    }

    int previousIndex = m_currentSearchResultIndex - 1;
    if (m_currentSearchResultIndex < 0 || previousIndex < 0) {
        previousIndex = totalResults - 1;
    }

    goToSearchResult(previousIndex);
}

void TabHandlePDF::findNextSearchResult()
{
    const int totalResults = searchModel->rowCount({});
    if (totalResults <= 0) {
        return;
    }

    int nextIndex = m_currentSearchResultIndex + 1;
    if (m_currentSearchResultIndex < 0 || nextIndex >= totalResults) {
        nextIndex = 0;
    }

    goToSearchResult(nextIndex);
}

void TabHandlePDF::updateSearchResultInfo()
{
    const int totalResults = searchModel->rowCount({});
    if (totalResults <= 0) {
        m_currentSearchResultIndex = -1;
    } else if (m_currentSearchResultIndex >= totalResults) {
        m_currentSearchResultIndex = totalResults - 1;
    }

    controlWidPDF->setSearchResultInfo(m_currentSearchResultIndex, totalResults);
}

void TabHandlePDF::resetSearch()
{
    m_currentSearchResultIndex = -1;
    searchModel->setSearchString(QString());
    pdfView->setCurrentSearchResultIndex(-1);
    controlWidPDF->clearSearch();
}

void TabHandlePDF::goToSearchResult(int resultIndex)
{
    const int totalResults = searchModel->rowCount({});
    if (resultIndex < 0 || resultIndex >= totalResults) {
        return;
    }

    const QModelIndex index = searchModel->index(resultIndex, 0);
    if (!index.isValid()) {
        return;
    }

    const int page = index.data(int(QPdfSearchModel::Role::Page)).toInt();
    const QPointF location = index.data(int(QPdfSearchModel::Role::Location)).toPointF();

    if (pdfView->pageNavigator()) {
        pdfView->pageNavigator()->jump(page, location);
    }

    pdfView->setCurrentSearchResultIndex(resultIndex);
    m_currentSearchResultIndex = resultIndex;
    controlWidPDF->setSearchResultInfo(m_currentSearchResultIndex, totalResults);
}
