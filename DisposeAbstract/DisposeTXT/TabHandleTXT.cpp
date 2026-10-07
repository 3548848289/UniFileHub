#include "TabHandleTXT.h"
#include <QDebug>
#include <QPainter>
#include <QScrollBar>
#include <QClipboard>
#include <QAbstractTextDocumentLayout>

#include <QAction>
#include <QMimeData>
#include <QtGlobal>
#if QT_VERSION_MAJOR >= 6
#include <QtCore5Compat/QTextCodec>
#else
#include <QTextCodec>
#endif
#include <QFileDialog>
#include <QSignalBlocker>
#include <QTimer>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextEdit>
#include <QStackedWidget>

// TextTab 实现
TextTab::TextTab(const QString &filePath, QWidget *parent)  : TabAbstract(filePath, parent), m_currentCodecName("UTF-8")
{
    textEdit = new PlainTextEdit(this);
    lineNumberWidget = new LineNumberWidget(textEdit, this);

    // 设置Tab缩进
    QFontMetrics fm(textEdit->font());
    int tabWidth = fm.horizontalAdvance("中文");
    textEdit->setTabStopDistance(tabWidth);

    // 创建语法高亮器(默认不挂载到文档,由控制栏的"语法高亮"复选框控制,
    // 避免大文件在打开时同步高亮整个文档造成卡顿)
    m_syntaxHighlighter = new SyntaxHighlighter(nullptr);

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

    // 文本区用堆叠容器承载(普通模式/预览模式共用一个槽位):
    // splitter 的子项保持不变,切换预览不会触发它重新分配空间而挤压控制栏
    textStack = new QStackedWidget(this);
    textStack->addWidget(textContainer);

    controlWidtxt = new ControlWidTXT(this);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(textStack);
    splitter->addWidget(controlWidtxt);
    splitter->setSizes({700, 100});

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    setLayout(layout);

    int fontSize = SettingManager::Instance().file_see_font_size();

    QFont font = textEdit->font();
    font.setPointSize(fontSize);
    textEdit->setFont(font);
    controlWidtxt->setFontSize(fontSize);
    controlWidtxt->setVisible(SettingManager::Instance().file_see_txt());

    connect(&SettingManager::Instance(), &SettingManager::settingChanged,
            this, [this](const QString &key, const QVariant &value) {
                if (key == QStringLiteral("file_see/font_size")) {
                    QFont font = textEdit->font();
                    font.setPointSize(value.toInt());
                    textEdit->setFont(font);
                    if (m_previewEdit) {
                        m_previewEdit->setFont(font);
                    }
                    controlWidtxt->setFontSize(value.toInt());
                } else if (key == QStringLiteral("file_see/txt")) {
                    controlWidtxt->setVisible(value.toBool());
                }
            });

    connect(textEdit, &QPlainTextEdit::textChanged, this, [this]() {
        setContentModified(true);

        // 更新文本统计信息
        updateTextStatistics();
    });

    // 连接编码变化信号
    connect(controlWidtxt, &ControlWidTXT::encodingChanged, this, &TextTab::onEncodingChanged);
    
    // 连接md预览模式切换信号
    connect(controlWidtxt, &ControlWidTXT::mdPreviewToggled, this, &TextTab::onMdPreviewToggled);
    
    // 连接HTML预览模式切换信号
    connect(controlWidtxt, &ControlWidTXT::htmlPreviewToggled, this, &TextTab::onHtmlPreviewToggled);
    connect(controlWidtxt, &ControlWidTXT::refreshRequested, this, &TextTab::onRefreshRequested);

    // 连接语法高亮开关信号
    connect(controlWidtxt, &ControlWidTXT::syntaxHighlightToggled, this, &TextTab::onHighlightToggled);

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
        if (m_previewEdit) {
            m_previewEdit->setFont(font);
        }
    });

    // 初始化文本统计信息
    updateTextStatistics();

    // 初始化编码为UTF-8
    setCurrentCodecName("UTF-8");
    
    // 加载文件内容
    if (!filePath.isEmpty()) {
        loadFromFile(filePath);
    }
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
    textEdit->setPlainText(text);
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
    updateTextStatistics();
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
    setContent(text);
    updateTextStatistics();
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
    const QString filePath = getCurrentFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    // 切换编码只应刷新显示，不应把当前编辑状态误判成“未保存”
    const bool wasModified = isModified;

    loadFromFile(filePath);

    QTimer::singleShot(0, this, [this, wasModified]() {
        setContentModified(wasModified);
    });
}

void TextTab::ensurePreviewWidget()
{
    if (m_previewEdit) {
        return;
    }
    m_previewEdit = new QTextBrowser(this);
    m_previewEdit->setFont(textEdit->font());
    // 加入堆叠容器(作为隐藏页),不动 splitter
    textStack->addWidget(m_previewEdit);
}

void TextTab::showPreview(bool enabled)
{
    if (!m_previewEdit) {
        return;
    }
    // 只切换堆叠页,splitter 子项不变,控制栏尺寸不受影响
    textStack->setCurrentWidget(enabled ? static_cast<QWidget *>(m_previewEdit)
                                        : static_cast<QWidget *>(textContainer));
}

void TextTab::onMdPreviewToggled(bool enabled)
{
    // 源文本保留在编辑器里不动，预览渲染到独立控件，因此无需屏蔽信号或恢复状态
    if (enabled) {
        ensurePreviewWidget();
        m_previewEdit->setMarkdown(textEdit->toPlainText());
    }
    showPreview(enabled);
}

void TextTab::onHtmlPreviewToggled(bool enabled)
{
    if (enabled) {
        ensurePreviewWidget();
        // setText 按 Qt::AutoText 自动识别纯文本/HTML，与旧的 QTextEdit::setText 行为一致
        m_previewEdit->setText(textEdit->toPlainText());
    }
    showPreview(enabled);
}

void TextTab::onHighlightToggled(bool enabled)
{
    // Qt 的实现:setDocument(文档) 挂载并在下一事件循环整篇重新高亮;
    // setDocument(nullptr) 断开连接并清除所有已着色块的格式
    m_syntaxHighlighter->setDocument(enabled ? textEdit->document() : nullptr);
}

void TextTab::updateTextStatistics()
{
    const int lineCount = textEdit->document()->blockCount();
    // characterCount 含每个块末尾的分隔符;toPlainText().length() 与之相差 1,保持旧口径
    const int charCount = textEdit->document()->characterCount() - 1;
    controlWidtxt->updateTextStatistics(lineCount, charCount);
}

void TextTab::onRefreshRequested()
{
    const QString filePath = getCurrentFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    if (isModified) {
        const auto reply = QMessageBox::question(
            this,
            tr("刷新文本"),
            tr("当前有未保存修改，刷新会用磁盘内容覆盖当前文本。是否继续？"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    const QTextCursor previousCursor = textEdit->textCursor();
    const int previousScrollValue = textEdit->verticalScrollBar()->value();
    const int previousHorizontalScrollValue = textEdit->horizontalScrollBar()->value();
    const bool wasModified = isModified;

    loadFromFile(filePath);

    QTextCursor restoredCursor = textEdit->textCursor();
    const int documentLength = textEdit->document()->characterCount();
    const int selectionStart = previousCursor.selectionStart();
    const int selectionEnd = previousCursor.selectionEnd();
    const bool hadSelection = previousCursor.hasSelection();

    restoredCursor.setPosition(qMin(previousCursor.position(), documentLength - 1));
    if (hadSelection) {
        restoredCursor.setPosition(qMin(selectionStart, documentLength - 1));
        restoredCursor.setPosition(qMin(selectionEnd, documentLength - 1), QTextCursor::KeepAnchor);
    }
    textEdit->setTextCursor(restoredCursor);

    textEdit->verticalScrollBar()->setValue(qMin(previousScrollValue, textEdit->verticalScrollBar()->maximum()));
    textEdit->horizontalScrollBar()->setValue(qMin(previousHorizontalScrollValue, textEdit->horizontalScrollBar()->maximum()));

    QTimer::singleShot(0, this, [this, wasModified]() {
        setContentModified(wasModified && isModified);
    });
}
