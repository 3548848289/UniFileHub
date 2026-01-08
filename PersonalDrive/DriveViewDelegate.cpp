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
    // 先画原始内容
    QStyledItemDelegate::paint(painter, option, index);

    // 只在 hover 行 + 最后一列画
    if (index.row() != m_hoverRow)
        return;

    if (index.column() != index.model()->columnCount() - 1)
        return;

    const int btnSize = 18;
    const int spacing = 6;
    const int btnCount = 4;

    QRect rect = option.rect;

    int x = rect.right() - btnCount * (btnSize + spacing);
    int y = rect.center().y() - btnSize / 2;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // QStringList texts = { tr("下载"), tr("删除"), tr("重命名"), tr("移动") };

    for (int i = 0; i < btnCount; ++i) {
        QRect btnRect(x + i * (btnSize + spacing),
                      y,
                      btnSize,
                      btnSize);

        // 背景
        // painter->setBrush(QColor(240, 240, 240));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(btnRect, 3, 3);
        // painter->setPen(Qt::black);
        // painter->drawText(btnRect, Qt::AlignCenter, texts[i]);
        // 图标
        QIcon icon;

        switch (i) {
        case 0: // 下载
            icon = QIcon::fromTheme(QIcon::ThemeIcon::GoDown);
            break;
        case 1: // 删除
            icon = QIcon::fromTheme(QIcon::ThemeIcon::EditDelete);
            break;
        case 2: // 重命名
            icon = QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaylistRepeat);
            break;
        case 3: // 移动
            icon = QIcon::fromTheme(QIcon::ThemeIcon::SystemLogOut);
            break;
        default:
            icon = QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh);
            break;
        }
        icon.paint(painter, btnRect);
    }

    painter->restore();
}
bool DriveViewDelegate::editorEvent(QEvent *event,
                                    QAbstractItemModel *,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index)
{
    // 只处理操作列
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

    // ===== 1️⃣ MouseMove：显示 tooltip =====
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

    // ===== 2️⃣ MouseButtonRelease：点击 =====
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
