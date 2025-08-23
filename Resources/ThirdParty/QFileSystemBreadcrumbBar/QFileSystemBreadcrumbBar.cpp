#include "QFileSystemBreadcrumbBar.h"


QFileSystemBreadcrumbBar::QFileSystemBreadcrumbBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(50);
    setStyleSheet("QWidget { border: 1px solid lightgray; border-radius: 2px; }");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);

    breadcrumb = new QBreadcrumbBar();
    breadcrumb->setShowFiles(false);

    // 初始路径
    BreadcrumbNode* root = new BreadcrumbNode("计算机", "", true);
    QList<BreadcrumbNode*> path = { root };
    breadcrumb->setPath(path);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(breadcrumb);

    layout->addWidget(scrollArea);

    connect(breadcrumb, &QBreadcrumbBar::pathClicked,
            this, &QFileSystemBreadcrumbBar::pathClicked);
    connect(breadcrumb, &QBreadcrumbBar::fileClicked,
            this, &QFileSystemBreadcrumbBar::fileClicked);
    connect(breadcrumb, &QBreadcrumbBar::pathEdited,
            this, &QFileSystemBreadcrumbBar::pathEdited);

}
void QFileSystemBreadcrumbBar::setBreadcrumbPath(const QList<BreadcrumbNode*>& path) {
    breadcrumb->setPath(path);
}

void QFileSystemBreadcrumbBar::setPath(const QString& path) {
    if (breadcrumb) {
        breadcrumb->parsePath(path);
    }
}

QBreadcrumbBar *QFileSystemBreadcrumbBar::bar() const {
    return breadcrumb;
}
