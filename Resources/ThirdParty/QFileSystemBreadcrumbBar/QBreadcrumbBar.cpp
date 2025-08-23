#include "QBreadcrumbBar.h"
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>

QBreadcrumbBar::QBreadcrumbBar(QWidget* parent) : QWidget(parent) {
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    setLayout(layout);
}

void QBreadcrumbBar::setShowFiles(bool value) {
    showFiles = value;
    rebuild();
}

void QBreadcrumbBar::setPath(const QList<BreadcrumbNode*>& path) {
    currentPath = path;
    rebuild();
}

void QBreadcrumbBar::clearLayout() {
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }
}
void QBreadcrumbBar::switchToEditMode() {
    if (!allowEditMode) return;   // ✅ 禁止编辑模式
    if (editMode) return;
    editMode = true;
    clearLayout();

    QLineEdit* edit = new QLineEdit(this);
    edit->setStyleSheet("border: none; background: transparent;");

    QString fullPath;
    if (!currentPath.isEmpty()) {
        fullPath = currentPath.last()->fullPath;
        fullPath.replace("\\", "/");
    }
    edit->setText(fullPath);
    layout->addWidget(edit);
    edit->setFocus();
    edit->selectAll();

    connect(edit, &QLineEdit::editingFinished, this, [this, edit]() {
        QString newPath = edit->text();
        parsePath(newPath);
        editMode = false;
        emit pathEdited(newPath);
    });

}


void QBreadcrumbBar::parsePath(const QString& pathText) {
    QList<BreadcrumbNode*> newPath;

#ifdef Q_OS_WIN
    BreadcrumbNode* root = new BreadcrumbNode("计算机", "", true);
    newPath.append(root);
#endif

#ifdef Q_OS_UNIX
    // Linux/Ubuntu 下把 / 作为虚拟根
    BreadcrumbNode* root = new BreadcrumbNode("/", "/");
    newPath.append(root);
#endif

    QString p = pathText;
    p.replace("\\", "/");
    p = QDir::cleanPath(p);

    if (!p.isEmpty() && p != "/") { // / 已经是根了
        QStringList parts = p.split("/", Qt::SkipEmptyParts);
        QString pathAccumulate = "/";

#ifdef Q_OS_WIN
        // 盘符处理 Windows
        if (!parts.isEmpty() && parts[0].endsWith(":")) {
            pathAccumulate = parts[0] + "/";
            newPath.append(new BreadcrumbNode(parts[0], pathAccumulate));
            parts.removeFirst();
        }
#endif

        for (const QString& part : parts) {
            if (!pathAccumulate.endsWith("/")) pathAccumulate += "/";
            pathAccumulate += part;
            // ✅ 路径存在性检查
            if (!QFileInfo(pathAccumulate).exists()) {
                setPath(currentPath);
                return;
            }
            newPath.append(new BreadcrumbNode(part, pathAccumulate));
        }
    }

    setPath(newPath);
}


void QBreadcrumbBar::rebuild() {
    clearLayout();

    // if (editMode) return; // 编辑模式下不显示按钮

    for (int i = 0; i < currentPath.size(); ++i) {
        BreadcrumbNode* node = currentPath[i];

        // 面包屑按钮
        QToolButton* btn = new QToolButton(this);
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setStyleSheet("border: none; background: transparent;");
        btn->setText(node->name);

        connect(btn, &QToolButton::clicked, this, [this, i](){
            currentPath = currentPath.mid(0, i+1);
            rebuild();
            emit pathClicked(i, currentPath[i]->fullPath);
        });
        layout->addWidget(btn);

        // 填充子节点
        populateChildren(node);

        if (!node->children.isEmpty()) {
            QToolButton* sepBtn = new QToolButton(this);
            sepBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            sepBtn->setStyleSheet("border: none; background: transparent;");
            sepBtn->setText("/"); // 分隔符
            layout->addWidget(sepBtn);

            QMenu* menu = new QMenu(this);
            for (BreadcrumbNode* child : node->children) {
                QAction* action = menu->addAction(child->name);
                connect(action, &QAction::triggered, this, [this, i, child](){
                    if (!child->fullPath.isEmpty() && QFileInfo(child->fullPath).isFile()) {
                        emit fileClicked(child->fullPath);
                    } else {
                        QList<BreadcrumbNode*> newPath = currentPath.mid(0, i+1);
                        newPath.append(new BreadcrumbNode(child->name, child->fullPath, child->isVirtualRoot));
                        setPath(newPath);
                        emit pathClicked(i+1, child->fullPath);
                    }
                });
            }

            connect(sepBtn, &QToolButton::clicked, this, [sepBtn, menu](bool){
                menu->exec(sepBtn->mapToGlobal(QPoint(0, sepBtn->height())));
            });
        }
    }

    // 添加占位按钮让点击进入编辑模式
    QToolButton* editBtn = new QToolButton(this);
    editBtn->setText(""); // 空白区域
    editBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    editBtn->setStyleSheet("border: none; background: transparent;");
    layout->addWidget(editBtn);
    connect(editBtn, &QToolButton::clicked, this, [this](){
        switchToEditMode();
    });

    layout->addStretch();
}

void QBreadcrumbBar::addMenuItem(int index, const QString& text) {
    if (index < 0 || index >= currentPath.size()-1) return;
    BreadcrumbNode* node = currentPath[index];
    BreadcrumbNode* newChild = new BreadcrumbNode(text);
    node->children.append(newChild);
    rebuild();
}

void QBreadcrumbBar::setAllowEditMode(bool value) { allowEditMode = value; }

void QBreadcrumbBar::populateChildren(BreadcrumbNode* node) {
    BreadcrumbNodeHelper::populateChildren(node, showFiles);
}
