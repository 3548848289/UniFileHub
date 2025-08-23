#include "BreadcrumbNodeHelper.h"


void BreadcrumbNodeHelper::populateChildren(BreadcrumbNode *node, bool showFiles) {
    if (!node) return;

    if (node->isVirtualRoot) {
        node->children.clear();
        for (const QFileInfo& fi : QDir::drives()) {
            QString drivePath = fi.absoluteFilePath();               // e.g. "D:/"
            QString driveName = QDir::toNativeSeparators(drivePath); // e.g. "D:\"
            // 去掉末尾的斜杠
            if (driveName.endsWith("\\") || driveName.endsWith("/")) {
                driveName.chop(1);
            }
            node->children.append(new BreadcrumbNode(driveName, drivePath));
        }
    } else if (!node->fullPath.isEmpty()) {
        node->children.clear();
        QDir dir(node->fullPath);
        if (!dir.exists()) return;

        QDir::Filters filter = QDir::Dirs | QDir::NoDotAndDotDot;
        if (showFiles) filter |= QDir::Files;

        QFileInfoList entries = dir.entryInfoList(filter, QDir::Name | QDir::DirsFirst);
        for (const QFileInfo& f : entries) {
            node->children.append(new BreadcrumbNode(f.fileName(), f.absoluteFilePath()));
        }
    }
    // 自定义节点保持原样，不做处理
}
