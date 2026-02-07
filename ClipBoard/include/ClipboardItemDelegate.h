#ifndef CLIPBOARDITEMDELEGATE_H
#define CLIPBOARDITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QColor>

class ClipboardItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ClipboardItemDelegate(QObject *parent = nullptr);
    
    void setThemeColors(const QColor& borderColor, const QColor& selectedColor);
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    static constexpr int SEQUENCE_WIDTH = 30; // 序号区域宽度
    QColor m_borderColor;
    QColor m_selectedColor;
};

#endif // CLIPBOARDITEMDELEGATE_H