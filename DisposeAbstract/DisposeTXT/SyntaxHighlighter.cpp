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
        keywords = QStringList{QStringLiteral("class"), QStringLiteral("struct"), QStringLiteral("public"), QStringLiteral("private"), QStringLiteral("protected"), QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"), QStringLiteral("do"), QStringLiteral("switch"), QStringLiteral("case"), QStringLiteral("default"), QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("return"), QStringLiteral("goto"), QStringLiteral("sizeof"), QStringLiteral("typedef"), QStringLiteral("enum"), QStringLiteral("extern"), QStringLiteral("static"), QStringLiteral("const"), QStringLiteral("volatile"), QStringLiteral("inline"), QStringLiteral("virtual"), QStringLiteral("override"), QStringLiteral("final"), QStringLiteral("namespace"), QStringLiteral("using"), QStringLiteral("template"), QStringLiteral("typename")};
        types = QStringList{QStringLiteral("int"), QStringLiteral("float"), QStringLiteral("double"), QStringLiteral("char"), QStringLiteral("void"), QStringLiteral("bool"), QStringLiteral("short"), QStringLiteral("long"), QStringLiteral("unsigned"), QStringLiteral("signed"), QStringLiteral("wchar_t"), QStringLiteral("char16_t"), QStringLiteral("char32_t"), QStringLiteral("auto"), QStringLiteral("decltype")};
        break;
    case Java:
        keywords = QStringList{QStringLiteral("class"), QStringLiteral("interface"), QStringLiteral("public"), QStringLiteral("private"), QStringLiteral("protected"), QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"), QStringLiteral("do"), QStringLiteral("switch"), QStringLiteral("case"), QStringLiteral("default"), QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("return"), QStringLiteral("goto"), QStringLiteral("new"), QStringLiteral("this"), QStringLiteral("super"), QStringLiteral("extends"), QStringLiteral("implements"), QStringLiteral("import"), QStringLiteral("package"), QStringLiteral("static"), QStringLiteral("final"), QStringLiteral("abstract"), QStringLiteral("synchronized"), QStringLiteral("volatile"), QStringLiteral("transient"), QStringLiteral("native"), QStringLiteral("strictfp"), QStringLiteral("enum"), QStringLiteral("assert"), QStringLiteral("try"), QStringLiteral("catch"), QStringLiteral("finally"), QStringLiteral("throw"), QStringLiteral("throws")};
        types = QStringList{QStringLiteral("int"), QStringLiteral("float"), QStringLiteral("double"), QStringLiteral("char"), QStringLiteral("void"), QStringLiteral("boolean"), QStringLiteral("short"), QStringLiteral("long"), QStringLiteral("byte")};
        break;
    case Python:
        keywords = QStringList{QStringLiteral("and"), QStringLiteral("as"), QStringLiteral("assert"), QStringLiteral("break"), QStringLiteral("class"), QStringLiteral("continue"), QStringLiteral("def"), QStringLiteral("del"), QStringLiteral("elif"), QStringLiteral("else"), QStringLiteral("except"), QStringLiteral("finally"), QStringLiteral("for"), QStringLiteral("from"), QStringLiteral("global"), QStringLiteral("if"), QStringLiteral("import"), QStringLiteral("in"), QStringLiteral("is"), QStringLiteral("lambda"), QStringLiteral("nonlocal"), QStringLiteral("not"), QStringLiteral("or"), QStringLiteral("pass"), QStringLiteral("raise"), QStringLiteral("return"), QStringLiteral("try"), QStringLiteral("while"), QStringLiteral("with"), QStringLiteral("yield"), QStringLiteral("async"), QStringLiteral("await")};
        types = QStringList{QStringLiteral("int"), QStringLiteral("float"), QStringLiteral("str"), QStringLiteral("bool"), QStringLiteral("list"), QStringLiteral("dict"), QStringLiteral("set"), QStringLiteral("tuple"), QStringLiteral("None"), QStringLiteral("True"), QStringLiteral("False")};
        break;
    case JavaScript:
    case TypeScript:
        keywords = QStringList{QStringLiteral("break"), QStringLiteral("case"), QStringLiteral("catch"), QStringLiteral("class"), QStringLiteral("const"), QStringLiteral("continue"), QStringLiteral("debugger"), QStringLiteral("default"), QStringLiteral("delete"), QStringLiteral("do"), QStringLiteral("else"), QStringLiteral("enum"), QStringLiteral("export"), QStringLiteral("extends"), QStringLiteral("finally"), QStringLiteral("for"), QStringLiteral("function"), QStringLiteral("if"), QStringLiteral("import"), QStringLiteral("in"), QStringLiteral("instanceof"), QStringLiteral("new"), QStringLiteral("return"), QStringLiteral("super"), QStringLiteral("switch"), QStringLiteral("this"), QStringLiteral("throw"), QStringLiteral("try"), QStringLiteral("typeof"), QStringLiteral("var"), QStringLiteral("void"), QStringLiteral("while"), QStringLiteral("with"), QStringLiteral("yield"), QStringLiteral("async"), QStringLiteral("await"), QStringLiteral("implements"), QStringLiteral("interface"), QStringLiteral("let"), QStringLiteral("package"), QStringLiteral("private"), QStringLiteral("protected"), QStringLiteral("public"), QStringLiteral("static")};
        types = QStringList{QStringLiteral("number"), QStringLiteral("string"), QStringLiteral("boolean"), QStringLiteral("object"), QStringLiteral("undefined"), QStringLiteral("null"), QStringLiteral("symbol"), QStringLiteral("bigint")};
        break;
    case HTML:
        // HTML 关键字由标签名组成
        break;
    case CSS:
        keywords = QStringList{QStringLiteral("color"), QStringLiteral("background"), QStringLiteral("margin"), QStringLiteral("padding"), QStringLiteral("border"), QStringLiteral("width"), QStringLiteral("height"), QStringLiteral("font"), QStringLiteral("text"), QStringLiteral("display"), QStringLiteral("position"), QStringLiteral("top"), QStringLiteral("right"), QStringLiteral("bottom"), QStringLiteral("left"), QStringLiteral("float"), QStringLiteral("clear"), QStringLiteral("overflow"), QStringLiteral("visibility"), QStringLiteral("opacity"), QStringLiteral("z-index"), QStringLiteral("flex"), QStringLiteral("grid"), QStringLiteral("transition"), QStringLiteral("transform"), QStringLiteral("animation")};
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
