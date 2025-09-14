#include "TabManager.h"

TabManager::TabManager(QTabWidget* parentTabWidget, QObject* parent)
    : QObject(parent), tabWidget(parentTabWidget)
{
    connect(tabWidget, &QTabWidget::tabCloseRequested,
            this, [this](int index) { closeTab(index); });
}

int TabManager::addTab(TabAbstract* tab, const QString& displayName, const QString& filePath) {
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

void TabManager::closeTab(int index) {
    if (index < 0 || index >= tabWidget->count()) return;
    QWidget* widget = tabWidget->widget(index);
    if (!widget) return;

    QSignalBlocker blocker(tabWidget);  // 阻止 removeTab 触发信号

    if (auto* sendEmailTab = qobject_cast<SendEmail*>(widget)) {
        tabWidget->removeTab(index);
        // sendEmailTab->deleteLater();
        return;
    }

    if (auto* clipboard = qobject_cast<ClipboardView*>(widget)) {
        tabWidget->removeTab(index);
        // clipboard->deleteLater();
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
    if (fileTabMap.contains(filePath)) {
        int existingIndex = fileTabMap[filePath];
        tabWidget->setCurrentIndex(existingIndex);
        return;
    }

    TabAbstract* newTab = TabFactory::create(filePath);
    if (!newTab) {
        // QMessageBox::warning(nullptr, QObject::tr("错误"), QObject::tr("不支持的文件类型"));
        return;
    }

    newTab->loadFromFile(filePath);
    QString baseName = QFileInfo(filePath).fileName();
    addTab(newTab, baseName, filePath);

    emit fileOpened(filePath);

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
}


int TabManager::addWidgetTab(QWidget* widget, const QString& displayName) {
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

    currentTab->getFindDialog()->show();
    currentTab->getFindDialog()->raise();
    currentTab->getFindDialog()->activateWindow();

}


template<typename T>
T* TabManager::getCurrentTab() {
    QWidget* currentWidget = tabWidget->currentWidget();
    T* currentTab = qobject_cast<T*>(currentWidget);
    if (!currentTab) {
        QMessageBox::warning(nullptr, QObject::tr("错误"), QObject::tr("当前页签是无效的"));
    }
    return currentTab;
}
