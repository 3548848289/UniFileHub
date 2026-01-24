#include "TabHandleTXT.h"
#include <QDebug>
#include <QPainter>
#include <QScrollBar>
#include <QClipboard>
#include <QAbstractTextDocumentLayout>

#include <QAction>
#include <QMimeData>
#include <QTextCodec>
#include <QFileDialog>

// TextTab 实现
TextTab::TextTab(const QString &filePath, QWidget *parent)  : TabAbstract(filePath, parent), m_currentCodecName("UTF-8")
{
    textEdit = new PlainTextEdit(this);
    lineNumberWidget = new LineNumberWidget(textEdit, this);

    // 设置Tab缩进
    QFontMetrics fm(textEdit->font());
    int tabWidth = fm.horizontalAdvance("中文");
    textEdit->setTabStopDistance(tabWidth);

    // 创建语法高亮器
    m_syntaxHighlighter = new SyntaxHighlighter(textEdit->document());

    // 根据文件扩展名设置语言
    QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == "cpp" || ext == "hpp" || ext == "cc") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::Cpp);
    } else if (ext == "c" || ext == "h") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::C);
    } else if (ext == "java") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::Java);
    } else if (ext == "py") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::Python);
    } else if (ext == "js") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::JavaScript);
    } else if (ext == "ts") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::TypeScript);
    } else if (ext == "html" || ext == "htm") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::HTML);
    } else if (ext == "css") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::CSS);
    } else if (ext == "xml" || ext == "qrc") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::XML);
    } else if (ext == "json") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::JSON);
    } else if (ext == "md") {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::Markdown);
    } else {
        m_syntaxHighlighter->setLanguage(SyntaxHighlighter::PlainText);
    }

    // 安装事件过滤器处理粘贴操作
    textEdit->installEventFilter(this);



    // 创建文本容器，包含行号和文本编辑
    textContainer = new QWidget(this);
    textLayout = new QHBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(0);
    textLayout->addWidget(lineNumberWidget);
    textLayout->addWidget(textEdit);

    controlWidtxt = new ControlWidTXT(this);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(textContainer);
    splitter->addWidget(controlWidtxt);
    splitter->setSizes({700, 100});

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    setLayout(layout);

    int fontSize = SettingManager::Instance().file_see_font_size();

    QFont font = textEdit->font();
    font.setPointSize(fontSize);
    textEdit->setFont(font);

    connect(textEdit, &QTextEdit::textChanged, this, [this]() {
        setContentModified(true);

        // 更新文本统计信息
        int lineCount = textEdit->document()->blockCount();
        int charCount = textEdit->toPlainText().length();
        controlWidtxt->updateTextStatistics(lineCount, charCount);
    });

    // 连接编码变化信号
    connect(controlWidtxt, &ControlWidTXT::encodingChanged, this, &TextTab::onEncodingChanged);
    
    // 连接以该编码保存信号
    connect(controlWidtxt, &ControlWidTXT::saveWithEncodingRequested, this, [this]() {
        QString filePath = getCurrentFilePath();
        if (!filePath.isEmpty()) {
            saveToFile(filePath);
        } else {
            // 如果没有文件路径，弹出保存对话框
            QString fileName = QFileDialog::getSaveFileName(this, "保存文件", QString(), "文本文件 (*.txt);;所有文件 (*.*)");
            if (!fileName.isEmpty()) {
                saveToFile(fileName);
            }
        }
    });

    // 连接Tab缩进字符数变化信号
    connect(controlWidtxt, &ControlWidTXT::tabIndentChanged, this, [this](int indent) {
        QFontMetrics fm(textEdit->font());
        int tabWidth = fm.horizontalAdvance(" ") * indent;
        textEdit->setTabStopDistance(tabWidth);
    });

    // 连接字体大小变化信号
    connect(controlWidtxt, &ControlWidTXT::fontSizeChanged, this, [this](int fontSize) {
        QFont font = textEdit->font();
        font.setPointSize(fontSize);
        textEdit->setFont(font);
    });

    // 初始化文本统计信息
    int lineCount = textEdit->document()->blockCount();
    int charCount = textEdit->toPlainText().length();
    controlWidtxt->updateTextStatistics(lineCount, charCount);

    // 初始化编码为UTF-8
    setCurrentCodecName("UTF-8");
}

void TextTab::onTextScrolled(int value)
{
    Q_UNUSED(value);
    lineNumberWidget->updateLineNumbers();
}

bool TextTab::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == textEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        // 处理Ctrl+V粘贴操作 - 保存格式
        if (keyEvent->matches(QKeySequence::Paste)) {
            // 允许默认的带格式粘贴行为
            return false;
        }
    }

    // 其他事件交给父类处理
    return QObject::eventFilter(obj, event);
}







void TextTab::setContent(const QString &text)
{
    Q_ASSERT(textEdit != nullptr);
    textEdit->setText(text);
}

QString TextTab::getContent() const
{
    return textEdit->toPlainText();
}



void TextTab::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("错误"), tr("无法打开文件"));
        return;
    }

    QByteArray content = file.readAll();
    file.close();

    QString text = decodeContent(content);

    setContent(text);
    setContentModified(false);
}


void TextTab::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "错误", "无法保存文件：" + file.errorString());
        return;
    }

    // 使用辅助函数根据当前编码编码内容
    QByteArray content = encodeContent(getContent());


    file.write(content);
    file.close();
    setContentModified(false);
}


void TextTab::loadFromInternet(const QByteArray &content)
{
    // 使用辅助函数根据当前编码解码内容
    QString text = decodeContent(content);
    qDebug() << "Converted text:" << text;
    setContent(text);
}

void TextTab::findNext(const QString &str, Qt::CaseSensitivity cs)
{
    QTextDocument::FindFlags findFlags = QTextDocument::FindWholeWords;

    if (cs == Qt::CaseSensitive) {
        findFlags |= QTextDocument::FindCaseSensitively;
    }

    // 第一次查找
    if (!textEdit->find(str, findFlags)) {
        // 没找到：移动光标到文档开头
        QTextCursor cursor = textEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        textEdit->setTextCursor(cursor);

        // 尝试再次查找
        textEdit->find(str, findFlags);
    }

    // 如果找到了，设置高亮颜色
    if (textEdit->textCursor().hasSelection()) {
        QPalette palette = textEdit->palette();
        palette.setColor(QPalette::Highlight, palette.color(QPalette::Active, QPalette::Highlight));
        textEdit->setPalette(palette);
    }
}



void TextTab::findAll(const QString &str, Qt::CaseSensitivity cs)
{
    QTextDocument *document = textEdit->document();
    document->undo();
    bool found = false;
    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextCursor cursor(document);
    cursor.beginEditBlock();

    QTextCharFormat plainFormat(cursor.charFormat());
    QTextCharFormat colorFormat = plainFormat;
    colorFormat.setForeground(Qt::red);

    while (!cursor.isNull() && !cursor.atEnd()) {
        cursor = document->find(str, cursor, QTextDocument::FindWholeWords);
        if (!cursor.isNull()) {
            found = true;
            // 创建额外的选择（高亮）
            QTextEdit::ExtraSelection selection;
            selection.format = colorFormat;
            selection.cursor = cursor;
            extraSelections.append(selection);
        }
    }

    cursor.endEditBlock();
    textEdit->setExtraSelections(extraSelections);

    if (!found)
        QMessageBox::information(this, tr("查找"), tr("找不到此单词"));
}

void TextTab::clearHighlight()
{
    textEdit->setExtraSelections(QList<QTextEdit::ExtraSelection>());
}



QString TextTab::getCurrentCodecName() const
{
    return m_currentCodecName;
}

QString TextTab::normalizeCodecName(const QString& codecName)
{
    // 将用户友好的编码名称转换为Qt支持的编码名称
    if (codecName == "Shift-JIS") {
        return "Shift_JIS";
    } else if (codecName == "ISO-8859-1") {
        return "ISO-8859-1";
    } else if (codecName == "EUC-KR") {
        return "EUC-KR";
    } else if (codecName == "EUC-JP") {
        return "EUC-JP";
    } else if (codecName == "ISO-2022-JP") {
        return "ISO-2022-JP";
    } else if (codecName == "ISO-2022-KR") {
        return "ISO-2022-KR";
    }
    // 其他编码名称保持不变
    return codecName;
}

void TextTab::setCurrentCodecName(const QString& codecName)
{
    if (m_currentCodecName != codecName) {
        m_currentCodecName = codecName;
        if (controlWidtxt) {
            controlWidtxt->setCurrentCodecName(codecName);
        }
    }
}

QString TextTab::decodeContent(const QByteArray &content) const
{
    if (m_currentCodecName == "UTF-8 BOM") {
        // UTF-8 BOM 文件
        QByteArray adjustedContent = content;
        if (adjustedContent.startsWith("\xEF\xBB\xBF"))
            adjustedContent = adjustedContent.mid(3);
        return QString::fromUtf8(adjustedContent);
    } else if (m_currentCodecName == "UTF-8") {
        return QString::fromUtf8(content);
    } else if (m_currentCodecName == "GBK" || m_currentCodecName == "ANSI" || m_currentCodecName == "System") {
        QString normalizedCodec = normalizeCodecName("GBK");
        QTextCodec *codec = QTextCodec::codecForName(normalizedCodec.toUtf8());
        if (!codec) {
            codec = QTextCodec::codecForName("GB18030"); //  fallback
        }
        return codec ? codec->toUnicode(content) : QString::fromUtf8(content);
    } else if (m_currentCodecName == "Shift-JIS") {
        QString normalizedCodec = normalizeCodecName(m_currentCodecName);
        QTextCodec *codec = QTextCodec::codecForName(normalizedCodec.toUtf8());
        return codec ? codec->toUnicode(content) : QString::fromUtf8(content);
    } else if (m_currentCodecName == "EUC-KR") {
        QString normalizedCodec = normalizeCodecName(m_currentCodecName);
        QTextCodec *codec = QTextCodec::codecForName(normalizedCodec.toUtf8());
        return codec ? codec->toUnicode(content) : QString::fromUtf8(content);
    } else if (m_currentCodecName == "ISO-8859-1") {
        QString normalizedCodec = normalizeCodecName(m_currentCodecName);
        QTextCodec *codec = QTextCodec::codecForName(normalizedCodec.toUtf8());
        return codec ? codec->toUnicode(content) : QString::fromUtf8(content);
    } else {
        // 默认使用UTF-8
        return QString::fromUtf8(content);
    }
}

QByteArray TextTab::encodeContent(const QString &text) const
{
    if (m_currentCodecName == "UTF-8" || m_currentCodecName == "UTF-8 BOM") {
        QByteArray content;
        if (m_currentCodecName == "UTF-8 BOM") {
            // 添加 UTF-8 BOM
            content = QByteArray::fromHex("EFBBBF");
        }
        content += text.toUtf8();
        return content;
    } else if (m_currentCodecName == "GBK" || m_currentCodecName == "ANSI" || m_currentCodecName == "System") {
        QString normalizedCodec = normalizeCodecName("GBK");
        QTextCodec *codec = QTextCodec::codecForName(normalizedCodec.toUtf8());
        if (!codec) {
            codec = QTextCodec::codecForName("GB18030"); // fallback
        }
        return codec ? codec->fromUnicode(text) : text.toUtf8();
    } else if (m_currentCodecName == "Shift-JIS") {
        QString normalizedCodec = normalizeCodecName(m_currentCodecName);
        QTextCodec *codec = QTextCodec::codecForName(normalizedCodec.toUtf8());
        return codec ? codec->fromUnicode(text) : text.toUtf8();
    } else if (m_currentCodecName == "EUC-KR") {
        QString normalizedCodec = normalizeCodecName(m_currentCodecName);
        QTextCodec *codec = QTextCodec::codecForName(normalizedCodec.toUtf8());
        return codec ? codec->fromUnicode(text) : text.toUtf8();
    } else if (m_currentCodecName == "ISO-8859-1") {
        QString normalizedCodec = normalizeCodecName(m_currentCodecName);
        QTextCodec *codec = QTextCodec::codecForName(normalizedCodec.toUtf8());
        return codec ? codec->fromUnicode(text) : text.toUtf8();
    } else {
        // 默认使用UTF-8
        return text.toUtf8();
    }
}

void TextTab::onEncodingChanged(const QString& codecName)
{
    setCurrentCodecName(codecName);
    // 重新加载文件以应用新编码
    if (!getCurrentFilePath().isEmpty()) {
        loadFromFile(getCurrentFilePath());
    }
}


