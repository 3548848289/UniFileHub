#include "TabHandleTXT.h"
#include <QDebug>

TextTab::TextTab(const QString &filePath, QWidget *parent)  : TabAbstract(filePath, parent), m_currentCodecName("UTF-8")
{
    textEdit = new QTextEdit(this);

    QFontMetrics fm(textEdit->font());
    int tabWidth = fm.horizontalAdvance("中文");
    textEdit->setTabStopDistance(tabWidth);

    // 为 QTextEdit 设置 C++ 语法高亮
    // new CppHighlighter(textEdit->document());

    controlWidtxt = new ControlWidTXT(this);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(textEdit);
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
    });
    
    // 连接编码变化信号
    connect(controlWidtxt, &ControlWidTXT::encodingChanged, this, &TextTab::onEncodingChanged);
    
    // 初始化编码为UTF-8
    setCurrentCodecName("UTF-8");
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


