#ifndef TABHANDLEPDF_H
#define TABHANDLEPDF_H

#include "../../main/include/TabAbstract.h"
#include <QMessageBox>
#include <QPdfDocument>
#include <QPdfSearchModel>
#include <QPdfView>
#include <QSplitter>
#include <QVBoxLayout>

class ControlWidPDF;

class TabHandlePDF : public TabAbstract
{
    Q_OBJECT

public:
    explicit TabHandlePDF(const QString &filePath, QWidget *parent = nullptr);

    void setContent(const QString &text) override;
    QString getContent() const override;
    void loadFromFile(const QString &fileName) override;
    void loadFromInternet(const QByteArray &content) override;
    void saveToFile(const QString &fileName) override;
    void ControlWidget(bool judge) override;

private slots:
    void goToPrevPage();
    void goToNextPage();
    void goToPage(int pageNumber);
    void changeFitToWidth(bool enabled);
    void changeZoomValue(int zoomPercentage);
    void changeScrollMode(bool enabled);
    void updateSearchText(const QString &text);
    void findPreviousSearchResult();
    void findNextSearchResult();
    void updateSearchResultInfo();

private:
    void resetSearch();
    void goToSearchResult(int resultIndex);

    QPdfDocument *pdfDoc;
    QPdfSearchModel *searchModel;
    QPdfView *pdfView;
    QVBoxLayout *mainLayout;
    bool isShowControl;
    int m_zoomPercentage;
    int m_currentSearchResultIndex;
    ControlWidPDF *controlWidPDF;
    QSplitter *splitter;
};

#endif // TABHANDLEPDF_H
