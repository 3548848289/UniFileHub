#include "include/HistoryDelegate.h"
namespace {
constexpr int kDownloadProgressRole = Qt::UserRole + 2;
}

HistoryDelegate::HistoryDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

void HistoryDelegate::setHoverRow(int row)
{
    m_hoverRow = row;
}

void HistoryDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 背景
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    if (index.column() == index.model()->columnCount() - 2
        && index.data(kDownloadProgressRole).isValid()) {
        const int progress = qBound(0, index.data(kDownloadProgressRole).toInt(), 100);
        const QString progressText = QString("%1%").arg(progress);
        const QRect contentRect = option.rect.adjusted(8, 8, -8, -8);
        const int textWidth = 42;
        const int gap = 8;
        const QRect textRect(contentRect.right() - textWidth + 1,
                             contentRect.top(),
                             textWidth,
                             contentRect.height());
        const QRect barRect(contentRect.left(),
                            contentRect.center().y() - 4,
                            qMax(20, contentRect.width() - textWidth - gap),
                            8);
        const int chunkWidth = barRect.width() * progress / 100;

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#bae0ff"));
        painter->drawRoundedRect(barRect, 4, 4);

        if (chunkWidth > 0) {
            QRect chunkRect = barRect;
            chunkRect.setWidth(chunkWidth);
            painter->setBrush(QColor("#1677ff"));
            painter->drawRoundedRect(chunkRect, 4, 4);
        }

        painter->setPen(QColor("#003a8c"));
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, progressText);
        painter->restore();
        return;
    }

    // 绘制普通文本内容（除了最后一列）
    if (index.column() < index.model()->columnCount() - 1) {
        QStyledItemDelegate::paint(painter, option, index);
    }

    // -------------------
    // 绘制操作按钮 (hover 行)
    // -------------------
    if (index.row() == m_hoverRow && index.column() == index.model()->columnCount() - 1) {
        const int btnSize = 18;
        const int spacing = 6;
        const int btnCount = 2;

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
            IconManager::Icon::Location,  // 打开文件所在位置
            IconManager::Icon::Preview    // 预览
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

bool HistoryDelegate::editorEvent(QEvent *event,
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
    const int btnCount = 2;

    QRect rect = option.rect;
    int buttonsWidth = btnCount * btnSize + (btnCount - 1) * spacing;
    int x = rect.right() - buttonsWidth;
    int y = rect.center().y() - btnSize / 2;

    static const QString tooltips[] = {
        tr("位置"),
        tr("预览")
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
                if (i == 0) {
                    emit openLocationClicked(index.row());
                } else if (i == 1) {
                    emit previewClicked(index.row());
                }
                return true;
            }
        }
    }

    return false;
}
