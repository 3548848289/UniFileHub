#include "include/ClipboardItemDelegate.h"
#include <QPainter>
#include <QStyle>
#include <QApplication>

ClipboardItemDelegate::ClipboardItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    // 初始化默认颜色
    m_borderColor = QColor(200, 200, 200);
    m_selectedColor = QColor(200, 200, 200);
}

void ClipboardItemDelegate::setThemeColors(const QColor& borderColor, const QColor& selectedColor)
{
    m_borderColor = borderColor;
    m_selectedColor = selectedColor;
}

void ClipboardItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // 保存原始的绘制区域
    QStyleOptionViewItem opt = option;
    opt.rect.setLeft(SEQUENCE_WIDTH);
    
    // 绘制原始的列表项内容
    QStyledItemDelegate::paint(painter, opt, index);
    
    // 绘制序号区域
    painter->save();
    
    QRect sequenceRect = option.rect;
    sequenceRect.setWidth(SEQUENCE_WIDTH);
    
    // 根据选项状态设置背景色
    if (option.state & QStyle::State_Selected) {
        // 选中状态：使用选中背景色
        painter->fillRect(sequenceRect, m_selectedColor);
    } else if (option.state & QStyle::State_MouseOver) {
        // 悬停状态：使用悬停背景色（半透明）
        painter->fillRect(sequenceRect, m_selectedColor);
    } else {
        // 普通状态：使用与列表项相同的背景色
        // painter->fillRect(sequenceRect, option.palette.color(QPalette::Base));
    }
    
    // 绘制顶部分隔线（与列表项样式一致）
    // 样式表中设置了 margin: 2px，所以横线需要向下偏移 2 像素以对齐
    painter->setPen(m_borderColor);
    int topLineY = sequenceRect.top() + 2; // 向下偏移 2 像素
    painter->drawLine(sequenceRect.left(), topLineY, sequenceRect.right(), topLineY);
    
    // 绘制序号文本
    QString sequenceText = QString::number(index.row() + 1); // 序号从1开始
    QFont font = painter->font();
    font.setPointSize(14);
    painter->setFont(font);
    
    // 根据状态设置文本颜色
    if (option.state & QStyle::State_Selected) {
        painter->setPen(QColor(0, 0, 0)); // 选中时使用黑色文本
    } else {
        painter->setPen(QColor(100, 100, 100));
    }
    painter->drawText(sequenceRect, Qt::AlignCenter, sequenceText);
    
    // 绘制右侧分隔线
    painter->setPen(m_borderColor);
    painter->drawLine(SEQUENCE_WIDTH - 1, sequenceRect.top(), SEQUENCE_WIDTH - 1, sequenceRect.bottom());
    
    painter->restore();
}

QSize ClipboardItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setWidth(size.width() + SEQUENCE_WIDTH);
    return size;
}
