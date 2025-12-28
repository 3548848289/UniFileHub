#ifndef BREADCRUMBNODEHELPER_H
#define BREADCRUMBNODEHELPER_H

#include <QString>
#include <QDir>
#include <QVariant>

struct BreadcrumbNode {
    QString name;
    QString fullPath;              // 为空 => 非文件系统节点
    QList<BreadcrumbNode*> children;
    bool isVirtualRoot = false;    // 只有“计算机”这种虚拟根为 true
    QVariant data;                 // 存储自定义数据，用于导航等功能

    BreadcrumbNode(const QString& n,
                   const QString& p = QString(),
                   bool vroot = false)
        : name(n), fullPath(p), isVirtualRoot(vroot) {}
};


class BreadcrumbNodeHelper {
public:
    static void populateChildren(BreadcrumbNode* node, bool showFiles);
};

#endif // BREADCRUMBNODEHELPER_H
