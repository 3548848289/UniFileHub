#ifndef LINENUMBERWIDGET_H
#define LINENUMBERWIDGET_H

#include <QWidget>
#include <QAbstractTextDocumentLayout>
#include "PlainTextEdit.h"

class LineNumberWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberWidget(PlainTextEdit *editor, QWidget *parent = nullptr);
    void updateLineNumbers();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
private:
    PlainTextEdit *m_textEdit;
};

#endif // LINENUMBERWIDGET_H