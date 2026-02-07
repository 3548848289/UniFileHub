#include "TabManager.h"
#include "../Setting/include/ThemeManager.h"
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

TabManager::TabManager(QTabWidget* parentTabWidget, QObject* parent) : QObject(parent), tabWidget(parentTabWidget)
{
    // 对于构造函数中的初始tabWidget，它对应位置(0,0)
    connect(tabWidget, &QTabWidget::tabCloseRequested,
            this, [this](int index) { closeTab(0, 0, index); });

    // 初始化容器部件和网格布局，设置父对象为主窗口
    if (parentTabWidget) {
        containerWidget = new QWidget(parentTabWidget->parentWidget());
        containerWidget->setObjectName("tabManagerContainer");
        // 设置容器部件为中央部件
        if (auto* mainWindow = qobject_cast<QMainWindow*>(parentTabWidget->parentWidget())) {
            mainWindow->setCentralWidget(containerWidget);
        }
    } else {
        containerWidget = new QWidget();
        containerWidget->setObjectName("tabManagerContainer");
    }
    
    gridLayout = new QGridLayout(containerWidget);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);
    
    // 设置容器部件的大小策略
    containerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // 初始化布局
    initLayout();
}

int TabManager::addTab(TabAbstract* tab, const QString& displayName, const QString& filePath) {
    // 首先尝试通过findActiveWidgetPosition获取真正的活动位置
    auto actualPosition = findActiveWidgetPosition();
    int row = actualPosition.first;
    int col = actualPosition.second;
    
    // 如果实际活动位置与记录的activePosition不一致，更新它
    if (actualPosition != activePosition) {
        activePosition = actualPosition;
    }
    
    // 检查当前布局是否有效
    if (row >= 0 && row < viewTabs.size() && col >= 0 && col < viewTabs[row].size() && viewTabs[row][col]) {
        QTabWidget* currentTabWidget = viewTabs[row][col];
        int index = currentTabWidget->addTab(tab, displayName);
        currentTabWidget->setCurrentIndex(index);

        connect(tab, &TabAbstract::contentModified, this, [currentTabWidget, tab]() {
            int i = currentTabWidget->indexOf(tab);
            if (i >= 0 && !currentTabWidget->tabText(i).endsWith("*")) {
                currentTabWidget->setTabText(i, currentTabWidget->tabText(i) + "*");
            }
        });

        connect(tab, &TabAbstract::contentSaved, this, [currentTabWidget, tab]() {
            int i = currentTabWidget->indexOf(tab);
            if (i >= 0) {
                QString name = QFileInfo(tab->getCurrentFilePath()).fileName();
                currentTabWidget->setTabText(i, name);
            }
        });

        if (!filePath.isEmpty())
            fileTabMap[filePath] = index;

        return index;
    }
    
    // 如果活动位置无效，回退到原始行为
    int index = tabWidget->addTab(tab, displayName);
    tabWidget->setCurrentIndex(index);
    
    connect(tab, &TabAbstract::contentModified, this, [this, tab]() {
        int i = tabWidget->indexOf(tab);
        if (i >= 0 && !tabWidget->tabText(i).endsWith("*")) {
            tabWidget->setTabText(i, tabWidget->tabText(i) + "*");
        }
    });

    connect(tab, &TabAbstract::contentSaved, this, [this, tab]() {
        int i = tabWidget->indexOf(tab);
        if (i >= 0) {
            QString name = QFileInfo(tab->getCurrentFilePath()).fileName();
            tabWidget->setTabText(i, name);
            fileTabMap[tab->getCurrentFilePath()] = i;
        }
    });

    if (!filePath.isEmpty())
        fileTabMap[filePath] = index;

    return index;
}

void TabManager::closeTab(int row, int col, int index) {
    // 确保位置和索引有效
    if (row >= 0 && row < viewTabs.size() && col >= 0 && col < viewTabs[row].size() && viewTabs[row][col]) {
        QTabWidget* currentTabWidget = viewTabs[row][col];
        
        // 更新活动位置到当前操作的QTabWidget
        activePosition = {row, col};
        
        if (index >= 0 && index < currentTabWidget->count()) {
            QWidget* widget = currentTabWidget->widget(index);
            if (!widget) return;

            QSignalBlocker blocker(currentTabWidget);  // 阻止 removeTab 触发信号

            if (auto* sendEmailTab = qobject_cast<SendEmail*>(widget)) {
                currentTabWidget->removeTab(index);
                return;
            }

            if (auto* clipboard = qobject_cast<ClipboardView*>(widget)) {
                currentTabWidget->removeTab(index);
                return;
            }

            if (auto* tab = qobject_cast<TabAbstract*>(widget)) {
                if (!tab->confirmClose()) return;
                QString filePath = tab->getCurrentFilePath();
                fileTabMap.remove(filePath);
                currentTabWidget->removeTab(index);
                tab->deleteLater();
            }
            return;
        }
    }

    // 回退到原始tabWidget
    if (index < 0 || index >= tabWidget->count()) return;
    QWidget* widget = tabWidget->widget(index);
    if (!widget) return;

    QSignalBlocker blocker(tabWidget);  // 阻止 removeTab 触发信号

    if (auto* sendEmailTab = qobject_cast<SendEmail*>(widget)) {
        tabWidget->removeTab(index);
        return;
    }

    if (auto* clipboard = qobject_cast<ClipboardView*>(widget)) {
        tabWidget->removeTab(index);
        return;
    }

    if (auto* tab = qobject_cast<TabAbstract*>(widget)) {
        if (!tab->confirmClose()) return;
        QString filePath = tab->getCurrentFilePath();
        fileTabMap.remove(filePath);
        tabWidget->removeTab(index);
        tab->deleteLater();
    }
}





void TabManager::openFile(const QString& filePath) {
    // 重要改进：不再依赖当前焦点状态，而是始终使用保存的activePosition
    // 这样无论从哪里调用，都会在用户最后操作的视图中打开文件
    
    // 直接使用保存的activePosition，不尝试重新查找
    int row = activePosition.first;
    int col = activePosition.second;
    
    // 检查位置有效性
    if (row >= 0 && row < viewTabs.size() && col >= 0 && col < viewTabs[row].size() && viewTabs[row][col]) {
        // 在活动视图中打开文件
        openFileInPosition(filePath, row, col);
        
        // 确保打开文件后，对应的QTabWidget获得焦点
        viewTabs[row][col]->setFocus();
        viewTabs[row][col]->activateWindow();
    } else {
        // 如果位置无效，回退到第一个有效视图
        if (!viewTabs.isEmpty() && !viewTabs[0].isEmpty() && viewTabs[0][0]) {
            row = 0;
            col = 0;
            activePosition = {row, col};
            openFileInPosition(filePath, row, col);
            viewTabs[row][col]->setFocus();
            viewTabs[row][col]->activateWindow();
        }
    }
}

void TabManager::deleteFile(const QString& filePath) {
    if (!fileTabMap.contains(filePath))
        return;

    int index = fileTabMap[filePath];
    if (index < 0 || index >= tabWidget->count())
        return;

    if (auto* tab = qobject_cast<TabAbstract*>(tabWidget->widget(index))) {
        tab->markModified();
    }
}

void TabManager::removePreviewTab() {
    if (currentPreviewTab) {
        // 查找当前预览标签页所在的标签页控件
        QTabWidget* tabWidget = nullptr;
        int index = -1;
        
        // 首先在所有视图中查找
        for (int row = 0; row < viewTabs.size(); ++row) {
            for (int col = 0; col < viewTabs[row].size(); ++col) {
                if (viewTabs[row][col]) {
                    index = viewTabs[row][col]->indexOf(currentPreviewTab);
                    if (index != -1) {
                        tabWidget = viewTabs[row][col];
                        break;
                    }
                }
            }
            if (tabWidget) break;
        }
        
        // 如果在视图中没有找到，在主tabWidget中查找
        if (!tabWidget) {
            index = this->tabWidget->indexOf(currentPreviewTab);
            if (index != -1) {
                tabWidget = this->tabWidget;
            }
        }
        
        // 如果找到了，删除标签页
        if (tabWidget && index != -1) {
            tabWidget->removeTab(index);
            delete currentPreviewTab;
            currentPreviewTab = nullptr;
        }
    }
}

void TabManager::setPreviewTab(TabAbstract* tab) {
    // 删除旧的预览标签页
    removePreviewTab();
    
    // 设置新的预览标签页
    currentPreviewTab = tab;
    
    // 将预览标签页添加到活动的QTabWidget中
    if (currentPreviewTab) {
        // 首先尝试通过findActiveWidgetPosition获取真正的活动位置
        auto actualPosition = findActiveWidgetPosition();
        int row = actualPosition.first;
        int col = actualPosition.second;
        
        // 检查当前布局是否有效
        if (row >= 0 && row < viewTabs.size() && col >= 0 && col < viewTabs[row].size() && viewTabs[row][col]) {
            QTabWidget* currentTabWidget = viewTabs[row][col];
            QString displayName = QFileInfo(currentPreviewTab->getCurrentFilePath()).fileName() + " (预览)";
            currentTabWidget->addTab(currentPreviewTab, displayName);
            currentTabWidget->setCurrentWidget(currentPreviewTab);
        } else {
            // 如果活动位置无效，回退到原始tabWidget
            QString displayName = QFileInfo(currentPreviewTab->getCurrentFilePath()).fileName() + " (预览)";
            tabWidget->addTab(currentPreviewTab, displayName);
            tabWidget->setCurrentWidget(currentPreviewTab);
        }
    }
}



void TabManager::createNewTab(std::function<TabAbstract*()> tabFactory, const QString& tabName) {
    TabAbstract* newTab = tabFactory();
    addTab(newTab, tabName, QString());
}

void TabManager::openFileFromMemory(const QString &fileName, const QByteArray &fileContent)
{
    TabAbstract* newTab = TabFactory::create(fileName);
    if (!newTab) {
        QMessageBox::warning(nullptr, QObject::tr("错误"), QObject::tr("不支持的文件类型"));
        return;
    }

    newTab->loadFromInternet(fileContent);
    addTab(newTab, fileName, QString()); // filePath 为空，因为是临时加载
}


void TabManager::saveCurrentTab(QWidget *parent)
{
    auto currentTab = getCurrentTab<TabAbstract>();
    if (!currentTab) return;

    QString currentFilePath = currentTab->getCurrentFilePath();

    if (currentFilePath.isEmpty()) {
        QString fileFilter;
        if (dynamic_cast<TextTab*>(currentTab))
            fileFilter = QObject::tr("Text Files (*.txt);;All Files (*)");
        else if (dynamic_cast<TabHandleCSV*>(currentTab))
            fileFilter = QObject::tr("CSV Files (*.csv);;All Files (*)");
        else
            fileFilter = QObject::tr("All Files (*)");

        currentFilePath = QFileDialog::getSaveFileName(parent, QObject::tr("Save File"), "", fileFilter);
        if (currentFilePath.isEmpty())
            return;
        currentTab->setCurrentFilePath(currentFilePath);
        emit fileOpened(currentFilePath);
    }

    currentTab->fileSave();
    
    // 确保标签页名称更新（去除星号标记）
    int row = activePosition.first;
    int col = activePosition.second;
    if (row >= 0 && row < viewTabs.size() && col >= 0 && col < viewTabs[row].size() && viewTabs[row][col]) {
        QTabWidget* currentTabWidget = viewTabs[row][col];
        int index = currentTabWidget->indexOf(currentTab);
        if (index >= 0) {
            QString name = QFileInfo(currentFilePath).fileName();
            currentTabWidget->setTabText(index, name);
        }
    } else {
        // 回退到原始tabWidget
        int index = tabWidget->indexOf(currentTab);
        if (index >= 0) {
            QString name = QFileInfo(currentFilePath).fileName();
            tabWidget->setTabText(index, name);
        }
    }
}


int TabManager::addWidgetTab(QWidget* widget, const QString& displayName) {
    // 优先使用当前活动的QTabWidget（在分割视图中）
    int row = activePosition.first;
    int col = activePosition.second;
    
    if (row >= 0 && row < viewTabs.size() && col >= 0 && col < viewTabs[row].size() && viewTabs[row][col]) {
        QTabWidget* currentTabWidget = viewTabs[row][col];
        int index = currentTabWidget->addTab(widget, displayName);
        currentTabWidget->setCurrentIndex(index);
        return index;
    }
    
    // 回退到原始tabWidget（兼容旧代码）
    int index = tabWidget->addTab(widget, displayName);
    tabWidget->setCurrentIndex(index);
    return index;
}


TabAbstract* TabManager::createTabByFileName(const QString &fileName)
{
    return TabFactory::create(fileName);
}


void TabManager::openSharedCSVTab(SharedView* sharedView)
{
    // TabManager 自己有 createTabByFileName 方法
    TabAbstract* newTab = createTabByFileName("shared.csv"); // 临时名字
    TabHandleCSV* csvTab = qobject_cast<TabHandleCSV*>(newTab);
    if (!csvTab) return;

    addTab(csvTab, "共享文档", QString());
    csvTab->setLinkStatus(true);
    sharedView->bindTab(csvTab);
}

// TabManager.cpp
void TabManager::findInCurrentTab(QWidget* parent)
{
    auto currentTab = getCurrentTab<TabAbstract>();
    if (!currentTab) return;

    if (!currentTab->getFindDialog()) {
        FindDialog* dialog = new FindDialog(parent);
        currentTab->setFindDialog(dialog);

        if (auto csvTab = qobject_cast<TabHandleCSV*>(currentTab)) {
            connect(dialog, &FindDialog::findAll, csvTab, &TabHandleCSV::findAll);
            connect(dialog, &FindDialog::findNext, csvTab, &TabHandleCSV::findNext);
            connect(dialog, &FindDialog::dialogClosed, csvTab, &TabHandleCSV::clearHighlight);
        } else if (auto textTab = qobject_cast<TextTab*>(currentTab)) {
            connect(dialog, &FindDialog::findAll, textTab, &TextTab::findAll);
            connect(dialog, &FindDialog::findNext, textTab, &TextTab::findNext);
            connect(dialog, &FindDialog::dialogClosed, textTab, &TextTab::clearHighlight);
        } else {
            QMessageBox::information(parent, QObject::tr("未开放"),
                                     QObject::tr("当前仅支持 txt/csv 文件查找"));
            delete dialog;
            currentTab->setFindDialog(nullptr);
            return;
        }
    }

    FindDialog* findDialog = currentTab->getFindDialog();
    if (findDialog) {
        findDialog->show();
        findDialog->raise();
        findDialog->activateWindow();
    }
}

void TabManager::initLayout() {
    // 清除现有视图
    for (auto& row : viewTabs) {
        for (auto tab : row) {
            if (tab) {
                gridLayout->removeWidget(tab);
                delete tab;
            }
        }
    }
    viewTabs.clear();
    
    // 设置为默认的1x1布局
    currentLayout = LayoutType::LAYOUT_1X1;
    
    // 使用updateLayout来创建初始布局，确保一致性
    updateLayout();
}

void TabManager::updateLayout() {
    // 清除现有的布局项
    while (!gridLayout->isEmpty()) {
        QLayoutItem *item = gridLayout->takeAt(0);
        if (item) {
            if (item->widget()) {
                // 保留QTabWidget但从布局中移除
                item->widget()->hide();
            }
            delete item;
        }
    }

    // 保存所有现有的QTabWidget指针
    QVector<QVector<QTabWidget*>> oldViewTabs = viewTabs;
    
    int rows = 0;
    int cols = 0;

    switch (currentLayout) {
    case LayoutType::LAYOUT_1X1:
        rows = 1;
        cols = 1;
        break;
    case LayoutType::LAYOUT_1X2:
        rows = 1;
        cols = 2;
        break;
    case LayoutType::LAYOUT_2X1:
        rows = 2;
        cols = 1;
        break;
    case LayoutType::LAYOUT_2X2:
        rows = 2;
        cols = 2;
        break;
    }

    // 调整viewTabs数组大小并初始化QTabWidget
    viewTabs.resize(rows);
    for (int i = 0; i < rows; ++i) {
        viewTabs[i].resize(cols);
        for (int j = 0; j < cols; ++j) {
            // 尝试从旧布局中复用QTabWidget
            if (i < oldViewTabs.size() && j < oldViewTabs[i].size() && oldViewTabs[i][j]) {
                viewTabs[i][j] = oldViewTabs[i][j];
            } else {
                // 创建新的QTabWidget，确保使用containerWidget作为父对象
                viewTabs[i][j] = new QTabWidget(containerWidget);
                viewTabs[i][j]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

                viewTabs[i][j]->setTabsClosable(true);
                viewTabs[i][j]->setObjectName(QString("tabWidget_%1_%2").arg(i).arg(j));
                
                // 设置右键菜单策略
                viewTabs[i][j]->setContextMenuPolicy(Qt::CustomContextMenu);
                
                // 连接信号，跟踪当前活动标签
                connect(viewTabs[i][j], &QTabWidget::currentChanged, this, [this, i, j]() {
                    if (viewTabs[i][j]->currentWidget()) {
                        activePosition = {i, j};
                    }
                });
                
                // 添加鼠标点击事件处理，确保用户点击时更新活动位置
                viewTabs[i][j]->installEventFilter(this);
                viewTabs[i][j]->tabBar()->installEventFilter(this);

                // 连接关闭标签信号，使用lambda传递正确的QTabWidget
                connect(viewTabs[i][j], &QTabWidget::tabCloseRequested, this, [this, i, j](int index) {
                    // 直接调用新的closeTab方法，传递正确的位置信息
                    closeTab(i, j, index);
                });

                // 连接右键菜单信号
                connect(viewTabs[i][j], &QTabWidget::customContextMenuRequested, 
                        this, [this, i, j](const QPoint &pos) {
                    // 更新活动位置
                    activePosition = {i, j};
                    
                    QTabWidget* currentTabWidget = viewTabs[i][j];
                    int tabIndex = currentTabWidget->tabBar()->tabAt(pos);
                    if (tabIndex == -1) return; // 点击空白处无效

                    QMenu menu;
                    QAction *closeCurrent = menu.addAction("关闭当前");
                    QAction *closeOthers = menu.addAction("关闭其他");
                    QAction *closeAll = menu.addAction("关闭全部");
                    
                    // 添加"打开文件所在位置"选项
                    QAction *openContainingFolder = menu.addAction("打开文件所在位置");
                    // 添加"在文件系统打开"选项
                    QAction *openInFileSystem = menu.addAction("在文件系统打开");

                    QAction *selectedAction = menu.exec(currentTabWidget->tabBar()->mapToGlobal(pos));
                    if (!selectedAction) return;

                    if (selectedAction == closeCurrent) {
                        closeTab(i, j, tabIndex);
                    } else if (selectedAction == closeOthers) {
                        for (int idx = currentTabWidget->count() - 1; idx >= 0; --idx) {
                            if (idx != tabIndex)
                                closeTab(i, j, idx);
                        }
                    } else if (selectedAction == closeAll) {
                        for (int idx = currentTabWidget->count() - 1; idx >= 0; --idx) {
                            closeTab(i, j, idx);
                        }
                    } else if (selectedAction == openContainingFolder) {
                        // 打开文件所在位置
                        QWidget* widget = currentTabWidget->widget(tabIndex);
                        if (auto* tab = qobject_cast<TabAbstract*>(widget)) {
                            QString filePath = tab->getCurrentFilePath();
                            if (!filePath.isEmpty()) {
                                QFileInfo fileInfo(filePath);
                                QString folderPath = fileInfo.absolutePath();
                                QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
                            }
                        }
                    } else if (selectedAction == openInFileSystem) {
                        // 在文件系统打开
                        QWidget* widget = currentTabWidget->widget(tabIndex);
                        if (auto* tab = qobject_cast<TabAbstract*>(widget)) {
                            QString filePath = tab->getCurrentFilePath();
                            if (!filePath.isEmpty()) {
                                emit openInFileSystemRequested(filePath);
                            }
                        }
                    }
                });
            }
        }
    }
    
    // 先清除gridLayout中的所有旧部件
    while (QLayoutItem* item = gridLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            // 保存旧的分割器以便稍后删除
            delete widget;
        }
        delete item;
    }
    
    // 确保所有QTabWidget可见
    for (auto& tabRow : viewTabs) {
        for (auto tabWidget : tabRow) {
            if (tabWidget) {
                tabWidget->show();
            }
        }
    }
    
    // 使用QSplitter实现可调整大小的分割视图
    QWidget* layoutWidget = nullptr;
    
    switch (currentLayout) {
    case LayoutType::LAYOUT_1X1:
        // 单个视图，直接添加
        layoutWidget = viewTabs[0][0];
        break;
        
    case LayoutType::LAYOUT_1X2: {
        // 水平分割
        QSplitter* splitter = new QSplitter(Qt::Horizontal, containerWidget);
        splitter->setObjectName("horizontalSplitter");
        splitter->addWidget(viewTabs[0][0]);
        splitter->addWidget(viewTabs[0][1]);
        // 设置初始分割比例
        QList<int> sizes;
        sizes << 1 << 1;
        splitter->setSizes(sizes);
        // 启用手柄移动
        splitter->setOpaqueResize(true);
        layoutWidget = splitter;
        break;
    }
    
    case LayoutType::LAYOUT_2X1: {
        // 垂直分割
        QSplitter* splitter = new QSplitter(Qt::Vertical, containerWidget);
        splitter->setObjectName("verticalSplitter");
        splitter->addWidget(viewTabs[0][0]);
        splitter->addWidget(viewTabs[1][0]);
        // 设置初始分割比例
        QList<int> sizes;
        sizes << 1 << 1;
        splitter->setSizes(sizes);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 1);
        // 启用手柄移动
        splitter->setOpaqueResize(true);
        layoutWidget = splitter;
        break;
    }
    
    case LayoutType::LAYOUT_2X2: {
        // 网格分割，需要嵌套QSplitter
        QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, containerWidget);
        mainSplitter->setObjectName("mainSplitter");
        
        // 左侧垂直分割器
        QSplitter* leftSplitter = new QSplitter(Qt::Vertical, mainSplitter);
        leftSplitter->setObjectName("leftSplitter");
        leftSplitter->addWidget(viewTabs[0][0]);
        leftSplitter->addWidget(viewTabs[1][0]);
        QList<int> leftSizes;
        leftSizes << 1 << 1;
        leftSplitter->setSizes(leftSizes);
        
        // 右侧垂直分割器
        QSplitter* rightSplitter = new QSplitter(Qt::Vertical, mainSplitter);
        rightSplitter->setObjectName("rightSplitter");
        rightSplitter->addWidget(viewTabs[0][1]);
        rightSplitter->addWidget(viewTabs[1][1]);
        QList<int> rightSizes;
        rightSizes << 1 << 1;
        rightSplitter->setSizes(rightSizes);
        
        // 将左右分割器添加到主分割器
        mainSplitter->addWidget(leftSplitter);
        mainSplitter->addWidget(rightSplitter);
        QList<int> mainSizes;
        mainSizes << 1 << 1;
        mainSplitter->setSizes(mainSizes);
        
        // 启用手柄移动
        mainSplitter->setOpaqueResize(true);
        leftSplitter->setOpaqueResize(true);
        rightSplitter->setOpaqueResize(true);
        
        layoutWidget = mainSplitter;
        break;
    }
    }
    
    // 将布局部件添加到网格布局
    if (layoutWidget) {
        gridLayout->addWidget(layoutWidget, 0, 0);
        layoutWidget->show();
        layoutWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    // 设置活动位置为第一个有效视图
    if (rows > 0 && cols > 0 && viewTabs[0][0]) {
        activePosition = {0, 0};
    }

    // 确保容器部件可见并设置大小策略
    containerWidget->show();
    containerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    gridLayout->invalidate();
    containerWidget->updateGeometry();
}

void TabManager::setLayoutType(LayoutType type) {
    if (currentLayout != type) {
        currentLayout = type;
        updateLayout();
    }
}

LayoutType TabManager::getCurrentLayoutType() const {
    return currentLayout;
}

void TabManager::openFileInPosition(const QString& filePath, int row, int col) {
    // 检查位置是否有效
    if (row < 0 || row >= viewTabs.size() || col < 0 || col >= viewTabs[row].size()) {
        qDebug() << "Invalid view position";
        return;
    }
    
    QTabWidget* targetTabWidget = viewTabs[row][col];
    if (!targetTabWidget) {
        qDebug() << "Target tab widget does not exist";
        return;
    }
    
    // 检查文件是否已在其他标签中打开
    for (auto& tabRow : viewTabs) {
        for (auto tabWidget : tabRow) {
            if (tabWidget) {
                for (int i = 0; i < tabWidget->count(); ++i) {
                    if (TabAbstract* tab = qobject_cast<TabAbstract*>(tabWidget->widget(i))) {
                        if (tab->getCurrentFilePath() == filePath) {
                            // 如果已打开，激活该标签
                            tabWidget->setCurrentWidget(tab);
                            activePosition = {row, col};
                            return;
                        }
                    }
                }
            }
        }
    }
    
    // 创建新标签并加载文件
    TabAbstract* newTab = TabFactory::create(filePath);
    if (!newTab) {
        QMessageBox::warning(nullptr, QObject::tr("错误"), QObject::tr("不支持的文件类型"));
        return;
    }
    
    newTab->loadFromFile(filePath);
    QString baseName = QFileInfo(filePath).fileName();
    
    // 添加到目标标签页
    int index = targetTabWidget->addTab(newTab, baseName);
    targetTabWidget->setCurrentIndex(index);
    activePosition = {row, col};
    
    // 连接信号
    connect(newTab, &TabAbstract::contentModified, this, [=]() {
        int i = targetTabWidget->indexOf(newTab);
        if (i >= 0 && !targetTabWidget->tabText(i).endsWith("*")) {
            targetTabWidget->setTabText(i, targetTabWidget->tabText(i) + "*");
        }
    });
    
    connect(newTab, &TabAbstract::contentSaved, this, [=]() {
        int i = targetTabWidget->indexOf(newTab);
        if (i >= 0) {
            QString name = QFileInfo(newTab->getCurrentFilePath()).fileName();
            targetTabWidget->setTabText(i, name);
        }
    });
    
    emit fileOpened(filePath);
}

void TabManager::openFileInActiveView(const QString& filePath) {
    openFileInPosition(filePath, activePosition.first, activePosition.second);
}

QPair<int, int> TabManager::getActiveViewPosition() const {
    return activePosition;
}

void TabManager::setActiveViewPosition(int row, int col) {
    // 检查位置是否有效
    if (row >= 0 && row < viewTabs.size() && col >= 0 && col < viewTabs[row].size()) {
        activePosition = {row, col};
        if (viewTabs[row][col]) {
            viewTabs[row][col]->setFocus();
        }
    }
}

bool TabManager::eventFilter(QObject *obj, QEvent *event) {
    // 处理鼠标点击事件，更新活动位置
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        for (int i = 0; i < viewTabs.size(); ++i) {
            for (int j = 0; j < viewTabs[i].size(); ++j) {
                if ((viewTabs[i][j] && obj == viewTabs[i][j]) || (viewTabs[i][j] && obj == viewTabs[i][j]->tabBar())) {
                    // 更新活动位置
                    activePosition = {i, j};

                    // 清除所有QTabWidget的选中样式
                    for (auto& tabRow : viewTabs) {
                        for (auto tabWidget : tabRow) {
                            if (tabWidget) {
                                // 重置为默认样式
                                tabWidget->setStyleSheet("");
                            }
                        }
                    }

                    // 使用主题管理器中的主颜色作为活动标签的高亮边框
                    viewTabs[i][j]->setStyleSheet(
                        "QTabWidget::pane { border: 2px solid " + ThemeManager::Instance().secondaryColor().name() + "; }"
                        );

                    // 确保该视图获得焦点
                    viewTabs[i][j]->setFocus();
                    return QObject::eventFilter(obj, event);
                }
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

QPair<int, int> TabManager::findActiveWidgetPosition() {
    // 查找当前有焦点的部件所在位置
    QWidget* activeWidget = QApplication::focusWidget();
    while (activeWidget) {
        for (int i = 0; i < viewTabs.size(); ++i) {
            for (int j = 0; j < viewTabs[i].size(); ++j) {
                if (viewTabs[i][j] && (activeWidget == viewTabs[i][j] || viewTabs[i][j]->isAncestorOf(activeWidget))) {
                    return {i, j};
                }
            }
        }
        activeWidget = activeWidget->parentWidget();
    }
    return {0, 0}; // 默认返回第一个位置
}

template<typename T>
T* TabManager::getCurrentTab() {
    // 首先尝试通过findActiveWidgetPosition获取真正的活动位置
    auto actualPosition = findActiveWidgetPosition();
    int row = actualPosition.first;
    int col = actualPosition.second;
    
    // 如果实际活动位置与记录的activePosition不一致，更新它
    if (actualPosition != activePosition) {
        activePosition = actualPosition;
    }
    
    if (row >= 0 && row < viewTabs.size() && col >= 0 && col < viewTabs[row].size() && viewTabs[row][col]) {
        QWidget* currentWidget = viewTabs[row][col]->currentWidget();
        T* currentTab = qobject_cast<T*>(currentWidget);
        if (currentTab) {
            return currentTab;
        }
    }
    
    // 回退到从原始tabWidget获取
    QWidget* currentWidget = tabWidget->currentWidget();
    T* currentTab = qobject_cast<T*>(currentWidget);
    if (!currentTab) {
        QMessageBox::warning(nullptr, QObject::tr("错误"), QObject::tr("当前页签是无效的"));
    }
    return currentTab;
}
