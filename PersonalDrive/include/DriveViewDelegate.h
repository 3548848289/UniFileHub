#ifndef DRIVEVIEWDELEGATE_H
#define DRIVEVIEWDELEGATE_H

#include <QStyledItemDelegate>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

class DriveViewDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit DriveViewDelegate(QObject *parent = nullptr);

    void setHoverRow(int row);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

signals:
    void actionClicked(int row, int action);

private:
    int m_hoverRow = -1;
};

#endif // DRIVEVIEWDELEGATE_H
