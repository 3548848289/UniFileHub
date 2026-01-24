#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

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

#endif // SYNTAXHIGHLIGHTER_H