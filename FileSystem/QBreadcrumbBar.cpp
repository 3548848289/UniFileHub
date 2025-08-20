#include "QBreadcrumbBar.h"
#include <QLabel>

QBreadcrumbBar::QBreadcrumbBar(QWidget *parent) : QWidget(parent) {
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    setLayout(layout);
}

void QBreadcrumbBar::setPath(const QStringList &parts) {
    // 清空原有控件
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    currentParts = parts;

    for (int i = 0; i < parts.size(); ++i) {
        QToolButton *btn = new QToolButton(this);
        btn->setText(parts[i]);
        btn->setProperty("index", i);
        connect(btn, &QToolButton::clicked, this, &QBreadcrumbBar::handleButtonClicked);
        layout->addWidget(btn);

        if (i < parts.size() - 1) {
            QLabel *sep = new QLabel(">", this);
            layout->addWidget(sep);
        }
    }
    layout->addStretch(); // 右侧空白填充
}

void QBreadcrumbBar::handleButtonClicked() {
    QToolButton *btn = qobject_cast<QToolButton *>(sender());
    if (!btn) return;
    int idx = btn->property("index").toInt();
    emit pathClicked(idx, currentParts[idx]);
}
