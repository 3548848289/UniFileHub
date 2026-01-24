#include "SyntaxHighlighter.h"
#include <QFont>

// SyntaxHighlighter 实现
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent), m_language(PlainText)
{
    // 初始化格式化
    m_keywordFormat.setForeground(Qt::darkBlue);
    m_keywordFormat.setFontWeight(QFont::Bold);
    
    m_stringFormat.setForeground(Qt::darkGreen);
    
    m_commentFormat.setForeground(Qt::gray);
    m_commentFormat.setFontItalic(true);
    
    m_numberFormat.setForeground(Qt::magenta);
    
    m_typeFormat.setForeground(Qt::darkCyan);
    
    m_functionFormat.setForeground(Qt::blue);
    m_functionFormat.setFontWeight(QFont::Bold);
    
    setupRules();
}

void SyntaxHighlighter::setLanguage(Language language)
{
    if (m_language != language) {
        m_language = language;
        setupRules();
        rehighlight();
    }
}

void SyntaxHighlighter::setupRules()
{
    m_highlightingRules.clear();
    
    QStringList keywords;
    QStringList types;
    
    switch (m_language) {
    case Cpp:
    case C:
        keywords = {"class", "struct", "public", "private", "protected", "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue", "return", "goto", "sizeof", "typedef", "enum", "extern", "static", "const", "volatile", "inline", "virtual", "override", "final", "namespace", "using", "template", "typename"};
        types = {"int", "float", "double", "char", "void", "bool", "short", "long", "unsigned", "signed", "wchar_t", "char16_t", "char32_t", "auto", "decltype"};
        break;
    case Java:
        keywords = {"class", "interface", "public", "private", "protected", "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue", "return", "goto", "new", "this", "super", "extends", "implements", "import", "package", "static", "final", "abstract", "synchronized", "volatile", "transient", "native", "strictfp", "enum", "assert", "try", "catch", "finally", "throw", "throws"};
        types = {"int", "float", "double", "char", "void", "boolean", "short", "long", "byte"};
        break;
    case Python:
        keywords = {"and", "as", "assert", "break", "class", "continue", "def", "del", "elif", "else", "except", "finally", "for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try", "while", "with", "yield", "async", "await"};
        types = {"int", "float", "str", "bool", "list", "dict", "set", "tuple", "None", "True", "False"};
        break;
    case JavaScript:
    case TypeScript:
        keywords = {"break", "case", "catch", "class", "const", "continue", "debugger", "default", "delete", "do", "else", "enum", "export", "extends", "finally", "for", "function", "if", "import", "in", "instanceof", "new", "return", "super", "switch", "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield", "async", "await", "implements", "interface", "let", "package", "private", "protected", "public", "static"};
        types = {"number", "string", "boolean", "object", "undefined", "null", "symbol", "bigint"};
        break;
    case HTML:
        // HTML 关键字由标签名组成
        break;
    case CSS:
        keywords = {"color", "background", "margin", "padding", "border", "width", "height", "font", "text", "display", "position", "top", "right", "bottom", "left", "float", "clear", "overflow", "visibility", "opacity", "z-index", "flex", "grid", "transition", "transform", "animation"};
        break;
    default:
        return;
    }
    
    // 添加关键字规则
    for (const QString &keyword : keywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_keywordFormat;
        m_highlightingRules.append(rule);
    }
    
    // 添加类型规则
    for (const QString &type : types) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\\b" + type + "\\b");
        rule.format = m_typeFormat;
        m_highlightingRules.append(rule);
    }
    
    // 添加函数规则
    HighlightingRule functionRule;
    functionRule.pattern = QRegularExpression("\\b[a-zA-Z_][a-zA-Z0-9_]*\\s*(?=\\()");
    functionRule.format = m_functionFormat;
    m_highlightingRules.append(functionRule);
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // 应用所有高亮规则
    for (const HighlightingRule &rule : m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    
    // 高亮字符串
    highlightStrings(text);
    
    // 高亮数字
    highlightNumbers(text);
    
    // 高亮注释
    highlightComments(text);
}

void SyntaxHighlighter::highlightComments(const QString &text)
{
    switch (m_language) {
    case Cpp:
    case C:
    case Java:
    case JavaScript:
    case TypeScript:
        // 单行注释
        {
            QRegularExpression commentRegex(QString("//[^\"]*$"));
            QRegularExpressionMatchIterator it = commentRegex.globalMatch(text);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                setFormat(match.capturedStart(), match.capturedLength(), m_commentFormat);
            }
        }
        // 多行注释
        {
            int startIndex = 0;
            if (previousBlockState() != 1) {
                startIndex = text.indexOf("/*");
            }
            
            while (startIndex >= 0) {
                int endIndex = text.indexOf("*/", startIndex + 2);
                int commentLength;
                if (endIndex == -1) {
                    setCurrentBlockState(1);
                    commentLength = text.length() - startIndex;
                } else {
                    commentLength = endIndex - startIndex + 2;
                }
                setFormat(startIndex, commentLength, m_commentFormat);
                startIndex = text.indexOf("/*", startIndex + commentLength);
            }
        }
        break;
    case Python:
        // Python 单行注释
        {
            QRegularExpression commentRegex(QString("#[^\"]*$"));
            QRegularExpressionMatchIterator it = commentRegex.globalMatch(text);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                setFormat(match.capturedStart(), match.capturedLength(), m_commentFormat);
            }
        }
        break;
    case HTML:
    case XML:
        // HTML/XML 注释
        {
            int startIndex = 0;
            if (previousBlockState() != 1) {
                startIndex = text.indexOf("<!--");
            }
            
            while (startIndex >= 0) {
                int endIndex = text.indexOf("-->", startIndex + 4);
                int commentLength;
                if (endIndex == -1) {
                    setCurrentBlockState(1);
                    commentLength = text.length() - startIndex;
                } else {
                    commentLength = endIndex - startIndex + 3;
                }
                setFormat(startIndex, commentLength, m_commentFormat);
                startIndex = text.indexOf("<!--", startIndex + commentLength);
            }
        }
        break;
    default:
        break;
    }
}

void SyntaxHighlighter::highlightStrings(const QString &text)
{
    // 单引号和双引号字符串
    QRegularExpression stringRegex("'([^'\\\\]|\\\\.)*'|\"([^\"\\\\]|\\\\.)*\"");
    QRegularExpressionMatchIterator matchIterator = stringRegex.globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_stringFormat);
    }
}

void SyntaxHighlighter::highlightNumbers(const QString &text)
{
    QRegularExpression numberRegex("\\b\\d+\\.?\\d*([eE][+-]?\\d+)?\\b");
    QRegularExpressionMatchIterator matchIterator = numberRegex.globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_numberFormat);
    }
}