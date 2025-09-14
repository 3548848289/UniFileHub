#ifndef TABMANAGER_H
#define TABMANAGER_H

#include <QTabWidget>
#include <QTabBar>
#include <QMap>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include "TabAbstract.h"
#include "TabFactory.h"
#include "../../OnlineDoc/include/WOnlineDoc.h"
#include "../../EmailService/SendEmail.h"
#include "../../ClipBoard/include/ClipboardView.h"
#include "FindDialog.h"


class TabManager : public QObject {
    Q_OBJECT

signals:
    void fileOpened(const QString& filePath);

public slots:
    void openFile(const QString& filePath);
    void deleteFile(const QString& filePath);
public:
    explicit TabManager(QTabWidget* parentTabWidget, QObject* parent = nullptr);

    int addTab(TabAbstract* tab, const QString& displayName, const QString& filePath);
    void closeTab(int index);
    void createNewTab(std::function<TabAbstract*()> tabFactory, const QString& tabName);

    template<typename T>
    T* getCurrentTab();

    QTabWidget* getTabWidget() const { return tabWidget; }

    void saveCurrentTab(QWidget *parent);
    void openFileFromMemory(const QString &fileName, const QByteArray &fileContent);
    void openSharedCSVTab(SharedView *sharedView);
    void findInCurrentTab(QWidget *parent);
    int addWidgetTab(QWidget *widget, const QString &displayName);
private:
    TabAbstract* createTabByFileName(const QString& fileName);
    // FindDialog* findDialog;
    QTabWidget* tabWidget;
    QMap<QString, int> fileTabMap;
};

#endif // TABMANAGER_H
