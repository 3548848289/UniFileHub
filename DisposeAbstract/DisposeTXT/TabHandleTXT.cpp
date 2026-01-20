#include "TabHandleTXT.h"
#include <QDebug>
#include <QPainter>
#include <QScrollBar>
#include <QClipboard>
#include <QAbstractTextDocumentLayout>
#include <QMenu>
#include <QAction>

// LineNumberWidget 实现
LineNumberWidget::LineNumberWidget(QTextEdit *editor, QWidget *parent) : QWidget(parent), m_textEdit(editor)
{
    setFixedWidth(50);
    setFont(editor->font());
    
    // 连接滚动条信号
    connect(editor->verticalScrollBar(), &QScrollBar::valueChanged, this, &LineNumberWidget::updateLineNumbers);
    connect(editor->document(), &QTextDocument::blockCountChanged, this, &LineNumberWidget::updateLineNumbers);
    connect(editor->document(), &QTextDocument::documentLayoutChanged, this, &LineNumberWidget::updateLineNumbers);
    connect(editor->document(), &QTextDocument::contentsChanged, this, &LineNumberWidget::updateLineNumbers);
}

void LineNumberWidget::updateLineNumbers()
{
    update();
}

void LineNumberWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), QColor(240, 240, 240));
    painter.setPen(QColor(120, 120, 120));
    painter.setFont(m_textEdit->font());
    
    QTextBlock block = m_textEdit->document()->begin();
    int blockNumber = 1;
    
    while (block.isValid()) {
        QTextLayout *layout = block.layout();
        QRectF boundingRect = layout->boundingRect();
        QPointF position = m_textEdit->document()->documentLayout()->blockBoundingRect(block).topLeft();
        
        // 检查当前块是否在可视区域内
        if (position.y() >= m_textEdit->verticalScrollBar()->value() + m_textEdit->viewport()->height()) {
            break;
        }
        
        if (position.y() + boundingRect.height() <= m_textEdit->verticalScrollBar()->value()) {
            block = block.next();
            blockNumber++;
            continue;
        }
        
        // 绘制行号
        painter.drawText(0, position.y() - m_textEdit->verticalScrollBar()->value() + boundingRect.height(),
                         width() - 5, boundingRect.height(), Qt::AlignRight, QString::number(blockNumber));
        
        block = block.next();
        blockNumber++;
    }
}

void LineNumberWidget::resizeEvent(QResizeEvent *event)
{
    updateLineNumbers();
    QWidget::resizeEvent(event);
}

// TextTab 实现
TextTab::TextTab(const QString &filePath, QWidget *parent)  : TabAbstract(filePath, parent), m_currentCodecName("UTF-8")
{
    textEdit = new QTextEdit(this);
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
    
    // 启用自定义右键菜单
    textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(textEdit, &QWidget::customContextMenuRequested, this, &TextTab::createCustomContextMenu);

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
        controlWidtxt->updateTextStats(lineCount, charCount);
    });
    
    // 连接编码变化信号
    connect(controlWidtxt, &ControlWidTXT::encodingChanged, this, &TextTab::onEncodingChanged);
    
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
    controlWidtxt->updateTextStats(lineCount, charCount);
    
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

// 实现自定义右键菜单
void TextTab::createCustomContextMenu(const QPoint &pos)
{
    // 获取默认菜单
    QMenu *menu = textEdit->createStandardContextMenu();
    
    // 查找并移除默认的粘贴菜单项
    QAction *defaultPasteAction = nullptr;
    QList<QAction*> actions = menu->actions();
    for (QAction *action : actions) {
        if (action->shortcut() == QKeySequence::Paste) {
            defaultPasteAction = action;
            break;
        }
    }
    
    if (defaultPasteAction) {
        // 移除默认粘贴菜单项
        menu->removeAction(defaultPasteAction);
        
        // 在合适的位置添加分隔符
        menu->addSeparator();
        
        // 添加带格式粘贴选项（默认选项）
        QAction *pasteWithFormatAction = new QAction("带格式粘贴", this);
        pasteWithFormatAction->setShortcut(QKeySequence::Paste);
        connect(pasteWithFormatAction, &QAction::triggered, this, &TextTab::pasteWithFormat);
        menu->addAction(pasteWithFormatAction);
        
        // 添加只粘贴文本选项
        QAction *pasteAsPlainTextAction = new QAction("只粘贴文本", this);
        connect(pasteAsPlainTextAction, &QAction::triggered, this, &TextTab::pasteAsPlainText);
        menu->addAction(pasteAsPlainTextAction);
        
        menu->addSeparator();
    }
    
    // 显示菜单
    menu->exec(textEdit->mapToGlobal(pos));
    delete menu;
}

// 带格式粘贴
void TextTab::pasteWithFormat()
{
    // 使用默认的粘贴行为，保留格式
    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier);
    QApplication::sendEvent(textEdit, &keyEvent);
}

// 只粘贴文本
void TextTab::pasteAsPlainText()
{
    // 获取剪贴板内容
    QClipboard *clipboard = QApplication::clipboard();
    QString plainText = clipboard->text();
    
    // 插入纯文本
    textEdit->insertPlainText(plainText);
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

    QString text;

    if (m_currentCodecName == "UTF-8 BOM") {
        // UTF-8 BOM 文件
        if (content.startsWith("\xEF\xBB\xBF"))
            content = content.mid(3);
        text = QString::fromUtf8(content);
    } else if (m_currentCodecName == "UTF-8") {
        text = QString::fromUtf8(content);
    } else if (m_currentCodecName == "GBK" || m_currentCodecName == "ANSI" || m_currentCodecName == "System") {
        text = gbkToQString(content);  // 用 Windows API 解码
    } else {
        // 默认尝试 UTF-8
        text = QString::fromUtf8(content);
    }

    setContent(text);
    setContentModified(false);
}


void TextTab::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("错误"), tr("无法保存文件"));
        return;
    }

    QByteArray content;

    if (m_currentCodecName == "UTF-8 BOM") {
        content = "\xEF\xBB\xBF" + getContent().toUtf8();
    } else if (m_currentCodecName == "UTF-8") {
        content = getContent().toUtf8();
    } else if (m_currentCodecName == "GBK" || m_currentCodecName == "ANSI" || m_currentCodecName == "System") {
        content = qStringToGbk(getContent());  // 用 Windows API 编码
    } else {
        content = getContent().toUtf8();
    }

    file.write(content);
    file.close();
    setContentModified(false);
}


void TextTab::loadFromInternet(const QByteArray &content)
{
    QString text;
    if (!m_currentCodecName.isEmpty()) {
        QStringDecoder decoder(m_currentCodecName.toUtf8());
        text = decoder.decode(content);
    } else {
        text = QString::fromUtf8(content);
    }
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

void TextTab::setCurrentCodecName(const QString& codecName)
{
    if (m_currentCodecName != codecName) {
        m_currentCodecName = codecName;
        if (controlWidtxt) {
            controlWidtxt->setCurrentCodecName(codecName);
        }
    }
}

void TextTab::onEncodingChanged(const QString& codecName)
{
    if (m_currentCodecName != codecName) {
        m_currentCodecName = codecName;
        // 重新加载文件内容以应用新编码
        QString currentFileName = getCurrentFilePath();
        if (!currentFileName.isEmpty()) {
            loadFromFile(currentFileName);
        }
        // 设置文档为已修改状态
        setContentModified(true);
    }
}


