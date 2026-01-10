#include "include/DriveViewDelegate.h"


DriveViewDelegate::DriveViewDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

void DriveViewDelegate::setHoverRow(int row)
{
    m_hoverRow = row;
}

void DriveViewDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 背景
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    // -------------------
    // 绘制文件/文件夹图标 + 名称
    // -------------------
    if (index.column() == 0) {
        QVariant v = index.data(DriveRoles::RoleIcon);
        QVariant text = index.data(DriveRoles::RoleText);

        if (v.isValid() && text.isValid()) {
            IconManager::Icon iconEnum = static_cast<IconManager::Icon>(v.toInt());
            QIcon icon = IconManager::icon(iconEnum, QSize(12,12)); // 12px 可改

            QRect iconRect(option.rect.left() + 8,
                           option.rect.center().y() - 8,
                           12, 12);
            icon.paint(painter, iconRect);

            QRect textRect = option.rect.adjusted(32, 0, -8, 0);
            painter->setPen(option.palette.text().color());
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text.toString());
        }
    }

    // -------------------
    // 绘制操作按钮 (hover 行)
    // -------------------
    if (index.row() == m_hoverRow && index.column() == index.model()->columnCount() - 1) {
        const int btnSize = 18;
        const int spacing = 6;
        const int btnCount = 4;

        QRect rect = option.rect;
        int buttonsWidth = btnCount * btnSize + (btnCount - 1) * spacing;
        int x = rect.right() - buttonsWidth;
        int y = rect.center().y() - btnSize / 2;

        // 背景
        QRect bgRect(x - 4, rect.top(), buttonsWidth + 8, rect.height());
        painter->setPen(Qt::NoPen);
        painter->setBrush(option.palette.base());
        painter->drawRect(bgRect);

        // 按钮
        static const IconManager::Icon icons[] = {
            IconManager::Icon::Download,
            IconManager::Icon::Delete,
            IconManager::Icon::Rename,
            IconManager::Icon::Move
        };

        for (int i = 0; i < btnCount; ++i) {
            QRect btnRect(x + i * (btnSize + spacing),
                          y,
                          btnSize,
                          btnSize);
            QIcon ic = IconManager::icon(icons[i], QSize(btnSize,btnSize));
            ic.paint(painter, btnRect);
        }
    }

    painter->restore();
}

bool DriveViewDelegate::editorEvent(QEvent *event,
                                    QAbstractItemModel *,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index)
{
    if (index.column() != index.model()->columnCount() - 1)
        return false;

    // 只在 hover 行有效
    if (index.row() != m_hoverRow)
        return false;

    const int btnSize  = 18;
    const int spacing  = 6;
    const int btnCount = 4;

    QRect rect = option.rect;
    int x = rect.right() - btnCount * (btnSize + spacing);
    int y = rect.center().y() - btnSize / 2;

    static const QString tooltips[] = {
        tr("下载"),
        tr("删除"),
        tr("重命名"),
        tr("移动")
    };

    auto *e = static_cast<QMouseEvent *>(event);

    if (event->type() == QEvent::MouseMove) {
        for (int i = 0; i < btnCount; ++i) {
            QRect btnRect(x + i * (btnSize + spacing),
                          y,
                          btnSize,
                          btnSize);

            if (btnRect.contains(e->pos())) {
                QToolTip::showText(e->globalPos(), tooltips[i]);
                return true;
            }
        }
        QToolTip::hideText();
        return false;
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        for (int i = 0; i < btnCount; ++i) {
            QRect btnRect(x + i * (btnSize + spacing),
                          y,
                          btnSize,
                          btnSize);

            if (btnRect.contains(e->pos())) {
                emit actionClicked(index.row(), i);
                return true;
            }
        }
    }

    return false;
}
