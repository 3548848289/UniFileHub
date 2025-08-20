#pragma once

#include <QWidget>
#include <QToolButton>
#include <QHBoxLayout>
#include <QStringList>

class QBreadcrumbBar : public QWidget {
    Q_OBJECT

public:
    explicit QBreadcrumbBar(QWidget *parent = nullptr);

    // 设置路径，例如 ["C:", "Users", "Admin", "Documents"]
    void setPath(const QStringList &parts);
    QStringList currentParts;

signals:
    // 用户点击某一层
    void pathClicked(int index, const QString &pathPart);

private slots:
    void handleButtonClicked();

private:
    QHBoxLayout *layout;
};
