#ifndef HISTORYDELEGATE_H
#define HISTORYDELEGATE_H

#include <QStyledItemDelegate>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include "../Setting/include/IconManager.h"

class HistoryDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit HistoryDelegate(QObject *parent = nullptr);

    void setHoverRow(int row);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

signals:
    void openLocationClicked(int row);
    void previewClicked(int row);

private:
    int m_hoverRow = -1;
};

#endif // HISTORYDELEGATE_H