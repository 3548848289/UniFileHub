#ifndef QFILESYSTEMBREADCRUMBBAR_H
#define QFILESYSTEMBREADCRUMBBAR_H

#include <QScrollArea>
#include "QBreadcrumbBar.h"

class QFileSystemBreadcrumbBar : public QWidget {
    Q_OBJECT
public:
    explicit QFileSystemBreadcrumbBar(QWidget* parent = nullptr);

    void setBreadcrumbPath(const QList<BreadcrumbNode*>& path);
    void setPath(const QString& path);  // 新接口

    QBreadcrumbBar* bar() const;
signals:
    void pathClicked(int index, const QString& name);
    void fileClicked(const QString& filePath);
    void pathEdited(const QString& newPath);

private:
    QBreadcrumbBar* breadcrumb;
};


#endif // QFILESYSTEMBREADCRUMBBAR_H
