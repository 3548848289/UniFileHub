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

    void clearSearch();
    void setTotalPages(int totalPages);
    void setCurrentPage(int pageNumber);
    int getCurrentPage() const;
    void setFitToWidthChecked(bool checked);
    void setPageModeChecked(bool checked);
    void setZoomPercentage(int zoomPercentage);
    void setSearchResultInfo(int currentResultIndex, int totalResults);

signals:
    void prevPageRequested();
    void nextPageRequested();
    void pageChanged(int pageNumber);
    void searchTextChanged(const QString &text);
    void searchPreviousRequested();
    void searchNextRequested();
    void fitToWidthChanged(bool checked);
    void zoomValueChanged(int value);
    void scrollModeChanged(bool enabled);

private slots:
    void on_prevPageButton_clicked();
    void on_nextPageButton_clicked();
    void on_pageLineEdit_editingFinished();
    void on_searchLineEdit_textChanged(const QString &text);
    void on_searchLineEdit_returnPressed();
    void on_searchPrevButton_clicked();
    void on_searchNextButton_clicked();
    void on_chkFitToCheckBox_toggled(bool checked);
    void on_zoomSlider_valueChanged(int value);
    void on_scrollModeCheckBox_stateChanged(int state);

private:
    void updateZoomDisplay(int zoomPercentage);

    Ui::ControlWidPDF *ui;
    int m_totalPages;
};

#endif // CONTROLWIDPDF_H
