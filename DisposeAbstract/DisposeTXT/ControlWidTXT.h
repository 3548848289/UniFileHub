#ifndef CONTROLWIDTXT_H
#define CONTROLWIDTXT_H

#include <QWidget>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QMouseEvent>
#include <QCursor>
#include "../../Setting/include/SettingManager.h"
#include "SyntaxHighlighter.h"


QT_BEGIN_NAMESPACE
namespace Ui { class ControlWidTXT; }
QT_END_NAMESPACE

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent = nullptr) : QLabel(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet("ClickableLabel:hover { background-color: rgba(0, 0, 0, 0.1); border-radius: 2px; }");
    }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        emit clicked();
        QLabel::mousePressEvent(event);
    }
};

class ControlWidTXT : public QWidget
{
    Q_OBJECT
public:
    explicit ControlWidTXT(QWidget *parent = nullptr);
    ~ControlWidTXT();

    QString getCurrentCodecName() const;
    void setCurrentCodecName(const QString &codecName);
    void updateTextStatistics(int lineCount, int charCount);
    void updateTextExcerpt(const QString &excerpt);

signals:
    void encodingChanged(const QString &codecName);
    void tabIndentChanged(int indent);
    void fontSizeChanged(int size);
    void saveWithEncodingRequested();
    void mdPreviewToggled(bool enabled);
    void htmlPreviewToggled(bool enabled);

private slots:
    void onTabIndentLabelClicked();
    void onEncodingLabelClicked();
    void onFontSizeLabelClicked();

private:
    Ui::ControlWidTXT *ui;
    QString m_currentCodecName;
    int m_tabIndent;
    int m_fontSize;

    void updateDisplayLabels();
};

#endif
