#include "ControlWidTXT.h"
#include "ui/ui_ControlWidTXT.h"
#include <QIntValidator>

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

ControlWidTXT::ControlWidTXT(QWidget *parent) : QWidget(parent), ui(new Ui::ControlWidTXT), m_currentCodecName("UTF-8")
{
    ui->setupUi(this);
    bool isShow = SettingManager::Instance().file_see_txt();
    this->setVisible(isShow);
    
    // 获取所有可用的编码名称
    QList<QByteArray> codecNames = {"UTF-8", "GBK", "UTF-16", "UTF-16LE", "UTF-16BE"};
    
    // 填充编码下拉框
    for (const QByteArray& codecName : codecNames) {
        QString name = QString::fromLatin1(codecName);
        ui->encodingComboBox->addItem(name, QVariant(codecName));
    }
    
    // 设置默认编码为UTF-8
    setCurrentCodecName("UTF-8");
    
    // 初始化Tab缩进输入框，设置整数验证器
    ui->tabIndentLineEdit->setValidator(new QIntValidator(1, 20, this));
    ui->tabIndentLineEdit->setText("4");
    
    // 初始化字体大小下拉框
    for (int size = 8; size <= 24; size += 2) {
        ui->fontSizeComboBox->addItem(QString::number(size), size);
    }
    ui->fontSizeComboBox->setCurrentText("12");
    
    // 连接编码选择变化信号
    connect(ui->encodingComboBox, &QComboBox::currentTextChanged, this, &ControlWidTXT::onEncodingComboBoxCurrentIndexChanged);
    
    // 连接Tab缩进字符数变化信号
    connect(ui->tabIndentLineEdit, &QLineEdit::textChanged, this, &ControlWidTXT::onTabIndentLineEditTextChanged);
    
    // 连接字体大小变化信号
    connect(ui->fontSizeComboBox, &QComboBox::currentTextChanged, this, &ControlWidTXT::onFontSizeComboBoxCurrentIndexChanged);
}

ControlWidTXT::~ControlWidTXT()
{
    delete ui;
}



QString ControlWidTXT::getCurrentCodecName() const
{
    return m_currentCodecName;
}

void ControlWidTXT::setCurrentCodecName(const QString& codecName)
{
    if (m_currentCodecName != codecName) {
        m_currentCodecName = codecName;
        int index = ui->encodingComboBox->findText(codecName);
        if (index != -1) {
            ui->encodingComboBox->setCurrentIndex(index);
        }
    }
}

void ControlWidTXT::updateTextStats(int lineCount, int charCount)
{
    ui->lineCountValueLabel->setText(QString::number(lineCount));
    ui->charCountValueLabel->setText(QString::number(charCount));
}

void ControlWidTXT::onEncodingComboBoxCurrentIndexChanged(const QString& codecName)
{
    if (m_currentCodecName != codecName) {
        m_currentCodecName = codecName;
        emit encodingChanged(codecName);
    }
}

void ControlWidTXT::onTabIndentLineEditTextChanged(const QString& text)
{
    int indent = text.toInt();
    if (indent > 0) {
        emit tabIndentChanged(indent);
    }
}

void ControlWidTXT::onFontSizeComboBoxCurrentIndexChanged(const QString& text)
{
    int fontSize = text.toInt();
    if (fontSize > 0) {
        emit fontSizeChanged(fontSize);
    }
}
