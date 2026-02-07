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
    // painter.fillRect(event->rect(), QColor(240, 240, 240));
    // painter.setPen(QColor(120, 120, 120));
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
        
        // 绘制行号（上移一行，对齐到文本块顶部）
        painter.drawText(0, position.y() - m_textEdit->verticalScrollBar()->value(),
                         width() - 5, boundingRect.height(), Qt::AlignRight | Qt::AlignTop, QString::number(blockNumber));
        
        block = block.next();
        blockNumber++;
    }
}

void LineNumberWidget::resizeEvent(QResizeEvent *event)
{
    updateLineNumbers();
    QWidget::resizeEvent(event);
}
