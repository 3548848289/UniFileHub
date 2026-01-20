#ifndef CONTROLWIDTXT_H
#define CONTROLWIDTXT_H

#include <QWidget>
#include <QApplication>
#include <QTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QTextDocument>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QComboBox>
#include "../../Setting/include/SettingManager.h"

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    enum Language {
        PlainText,
        Cpp,
        C,
        Java,
        Python,
        JavaScript,
        TypeScript,
        HTML,
        CSS,
        XML,
        JSON,
        Markdown
    };

    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);
    void setLanguage(Language language);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void setupRules();
    void highlightComments(const QString &text);
    void highlightStrings(const QString &text);
    void highlightNumbers(const QString &text);

    Language m_language;
    QList<HighlightingRule> m_highlightingRules;
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_functionFormat;
};


QT_BEGIN_NAMESPACE
namespace Ui { class ControlWidTXT; }
QT_END_NAMESPACE

class ControlWidTXT : public QWidget
{
    Q_OBJECT
public:
    explicit ControlWidTXT(QWidget *parent = nullptr);
    ~ControlWidTXT();
    

    QString getCurrentCodecName() const;
    

    void setCurrentCodecName(const QString& codecName);
    

    void updateTextStats(int lineCount, int charCount);

signals:

    void encodingChanged(const QString& codecName);
    

    void tabIndentChanged(int indent);
    

    void fontSizeChanged(int fontSize);

private slots:

    void onEncodingComboBoxCurrentIndexChanged(const QString& codecName);
    

    void onTabIndentLineEditTextChanged(const QString& text);
    

    void onFontSizeComboBoxCurrentIndexChanged(const QString& text);

private:
    Ui::ControlWidTXT *ui;
    QString m_currentCodecName;
};

#endif
