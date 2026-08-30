#include "include/InlineMessagePopup.h"

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

InlineMessagePopup::InlineMessagePopup(QWidget *host, int autoHideMs)
    : QWidget(host), // 直接挂到宿主上；若宿主是列表项等会被裁剪的容器，调用方应传入所属面板
      m_host(host),
      m_label(new QLabel(this)),
      m_autoHideMs(autoHideMs)
{
    // QWidget 子类必须显式开启，样式表背景/边框才会绘制
    setAttribute(Qt::WA_StyledBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setVisible(false);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 10, 16, 10);
    layout->setSpacing(8);

    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignCenter);

    layout->addWidget(m_label);
}

void InlineMessagePopup::showMessage(const QString &message, bool isError)
{
    applyTheme(isError);
    m_label->setText(message);
    reposition();
    raise();
    setVisible(true);

    const int serial = ++m_serial;
    QTimer::singleShot(m_autoHideMs, this, [this, serial]() {
        if (serial == m_serial) {
            setVisible(false);
        }
    });
}

void InlineMessagePopup::setPanelWidget(QWidget *panel)
{
    m_panel = panel;
    m_anchor = nullptr;
}

void InlineMessagePopup::setAnchorWidget(QWidget *widget)
{
    m_anchor = widget;
    m_panel = nullptr;
}

void InlineMessagePopup::reposition()
{
    // 弹窗挂在宿主（面板/对话框）上，宿主是普通 QWidget 不会裁剪子控件，
    // 坐标直接用相对宿主的局部坐标，避免顶层窗口坐标系换算的遮挡问题
    if (!m_host) {
        return;
    }

    const int availableWidth = qMax(120, m_host->width() - 40);
    const int popupMaxWidth = qMin(520, availableWidth);
    const int popupMinWidth = qMin(220, popupMaxWidth);

    setMinimumWidth(popupMinWidth);
    setMaximumWidth(popupMaxWidth);
    adjustSize();

    // 锚定模式：显示在锚点控件正下方（用于输入框等小控件）
    if (m_anchor) {
        const QPoint anchorPos = m_anchor->mapTo(m_host, QPoint(0, 0));
        const int x = anchorPos.x() + m_anchor->width() / 2 - width() / 2;
        const int y = anchorPos.y() + m_anchor->height() + 6;
        move(qMax(0, x), y);
        return;
    }

    // 面板模式（默认）：显示在面板内部顶部居中
    if (m_panel) {
        const QPoint panelPos = m_panel->mapTo(m_host, QPoint(0, 0));
        const int x = panelPos.x() + qMax(0, (m_panel->width() - width()) / 2);
        const int y = panelPos.y() + 16;
        move(x, y);
        return;
    }

    // 兜底：宿主顶部居中
    const int x = qMax(12, (m_host->width() - width()) / 2);
    move(x, 50);
}

void InlineMessagePopup::applyTheme(bool isError)
{
    const QString backgroundColor = isError ? "#fff1f0" : "#f6ffed";
    const QString borderColor = isError ? "#ff4d4f" : "#52c41a";
    const QString textColor = isError ? "#a8071a" : "#135200";

    setStyleSheet(QString(
        "QWidget {"
        "background-color: %1;"
        "border: 1px solid %2;"
        "border-radius: 6px;"
        "}"
        "QLabel {"
        "background: transparent;"
        "border: none;"
        "color: %3;"
        "font-size: 13px;"
        "}"
    ).arg(backgroundColor, borderColor, textColor));
}
