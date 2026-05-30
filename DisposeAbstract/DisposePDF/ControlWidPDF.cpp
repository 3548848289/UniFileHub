#include "ControlWidPDF.h"
#include "ui_ControlWidPDF.h"

#include <QSignalBlocker>

ControlWidPDF::ControlWidPDF(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ControlWidPDF)
    , m_totalPages(0)
{
    ui->setupUi(this);

    ui->zoomSlider->setRange(50, 200);
    ui->zoomSlider->setSingleStep(10);
    ui->zoomSlider->setPageStep(10);
    ui->zoomSlider->setValue(100);
    ui->chkFitToCheckBox->setChecked(true);
    updateZoomDisplay(100);
    clearSearch();
}

ControlWidPDF::~ControlWidPDF()
{
    delete ui;
}

void ControlWidPDF::clearSearch()
{
    QSignalBlocker blocker(ui->searchLineEdit);
    ui->searchLineEdit->clear();
    setSearchResultInfo(-1, 0);
}

void ControlWidPDF::setTotalPages(int totalPages)
{
    m_totalPages = totalPages;
    ui->totalPagesLabel->setText(QString("/ %1").arg(totalPages));

    ui->prevPageButton->setEnabled(totalPages > 1);
    ui->nextPageButton->setEnabled(totalPages > 1);
}

void ControlWidPDF::setCurrentPage(int pageNumber)
{
    QSignalBlocker blocker(ui->pageLineEdit);
    ui->pageLineEdit->setText(QString::number(pageNumber));

    ui->prevPageButton->setEnabled(pageNumber > 1);
    ui->nextPageButton->setEnabled(pageNumber < m_totalPages);
}

int ControlWidPDF::getCurrentPage() const
{
    bool ok = false;
    const int page = ui->pageLineEdit->text().toInt(&ok);
    return ok ? page : 1;
}

void ControlWidPDF::setFitToWidthChecked(bool checked)
{
    QSignalBlocker blocker(ui->chkFitToCheckBox);
    ui->chkFitToCheckBox->setChecked(checked);
}

void ControlWidPDF::setZoomPercentage(int zoomPercentage)
{
    QSignalBlocker blocker(ui->zoomSlider);
    ui->zoomSlider->setValue(zoomPercentage);
    updateZoomDisplay(zoomPercentage);
}

void ControlWidPDF::setSearchResultInfo(int currentResultIndex, int totalResults)
{
    const int displayIndex =
        (currentResultIndex >= 0 && currentResultIndex < totalResults) ? currentResultIndex + 1 : 0;
    ui->searchCountLabel->setText(QStringLiteral("%1/%2").arg(displayIndex).arg(totalResults));

    const bool enableNavigation = !ui->searchLineEdit->text().trimmed().isEmpty() && totalResults > 0;
    ui->searchPrevButton->setEnabled(enableNavigation);
    ui->searchNextButton->setEnabled(enableNavigation);
}

void ControlWidPDF::on_prevPageButton_clicked()
{
    emit prevPageRequested();
}

void ControlWidPDF::on_nextPageButton_clicked()
{
    emit nextPageRequested();
}

void ControlWidPDF::on_pageLineEdit_editingFinished()
{
    bool ok = false;
    int page = ui->pageLineEdit->text().toInt(&ok);

    if (!ok || page < 1) {
        page = 1;
    } else if (page > m_totalPages && m_totalPages > 0) {
        page = m_totalPages;
    }

    ui->pageLineEdit->setText(QString::number(page));
    emit pageChanged(page);

    ui->prevPageButton->setEnabled(page > 1);
    ui->nextPageButton->setEnabled(page < m_totalPages);
}

void ControlWidPDF::on_searchLineEdit_textChanged(const QString &text)
{
    Q_UNUSED(text);
    setSearchResultInfo(-1, 0);
    emit searchTextChanged(ui->searchLineEdit->text());
}

void ControlWidPDF::on_searchLineEdit_returnPressed()
{
    emit searchNextRequested();
}

void ControlWidPDF::on_searchPrevButton_clicked()
{
    emit searchPreviousRequested();
}

void ControlWidPDF::on_searchNextButton_clicked()
{
    emit searchNextRequested();
}

void ControlWidPDF::on_chkFitToCheckBox_toggled(bool checked)
{
    emit fitToWidthChanged(checked);
}

void ControlWidPDF::on_zoomSlider_valueChanged(int value)
{
    updateZoomDisplay(value);

    if (ui->chkFitToCheckBox->isChecked()) {
        QSignalBlocker blocker(ui->chkFitToCheckBox);
        ui->chkFitToCheckBox->setChecked(false);
    }

    emit zoomValueChanged(value);
}

void ControlWidPDF::on_scrollModeCheckBox_stateChanged(int state)
{
    emit scrollModeChanged(state == Qt::Checked);
}

void ControlWidPDF::updateZoomDisplay(int zoomPercentage)
{
    const QString text = QStringLiteral("%1%").arg(zoomPercentage);
    ui->zoomSlider->setToolTip(text);
    ui->zoomPercentLabel->setText(text);
}
