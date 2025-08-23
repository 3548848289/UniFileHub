#pragma once
#include <QWidget>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include "BreadcrumbNodeHelper.h"

class QBreadcrumbBar : public QWidget {
    Q_OBJECT
public:
    explicit QBreadcrumbBar(QWidget* parent = nullptr);

    void setShowFiles(bool value);
    void setPath(const QList<BreadcrumbNode*>& path);
    void addMenuItem(int index, const QString& text);
    void setAllowEditMode(bool value);
    void parsePath(const QString& pathText);

signals:
    void pathClicked(int index, const QString& name);
    void fileClicked(const QString& filePath);
    void pathEdited(const QString& newPath);

private:
    bool showFiles = false;
    QHBoxLayout* layout;
    QList<BreadcrumbNode*> currentPath;
    bool editMode = false;
    bool allowEditMode = true;
    void clearLayout();
    void rebuild();
    void populateChildren(BreadcrumbNode* node);
    void switchToEditMode();
};
