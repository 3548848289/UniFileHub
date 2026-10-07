#include "LineNumberWidget.h"
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextLayout>

LineNumberWidget::LineNumberWidget(PlainTextEdit *editor, QWidget *parent) : QWidget(parent), m_textEdit(editor)
{
    setFixedWidth(50);
    setFont(editor->font());
    
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
    painter.setFont(m_textEdit->font());

    // 从首个可见块开始绘制,只遍历可视区域内的块。
    // QPlainTextEdit 对不可见块是懒排版的,从文档头遍历会强制排版所有块。
    QTextBlock block = m_textEdit->firstVisibleBlock();
    if (!block.isValid()) {
        return;
    }

    const qreal offset = m_textEdit->contentOffset().y();
    qreal top = m_textEdit->blockBoundingGeometry(block).translated(0, offset).top();
    int blockNumber = block.blockNumber() + 1;

    while (block.isValid() && top <= event->rect().bottom()) {
        const qreal height = m_textEdit->blockBoundingRect(block).height();
        if (block.isVisible() && top + height >= event->rect().top()) {
            painter.drawText(0, (int)top, width() - 5, (int)height,
                             Qt::AlignRight | Qt::AlignTop, QString::number(blockNumber));
        }
        top += height;
        block = block.next();
        blockNumber++;
    }
}

void LineNumberWidget::resizeEvent(QResizeEvent *event)
{
    updateLineNumbers();
    QWidget::resizeEvent(event);
}
