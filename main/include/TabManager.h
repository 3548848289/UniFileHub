#ifndef TABMANAGER_H
#define TABMANAGER_H

#include <QTabWidget>
#include <QTabBar>
#include <QMap>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QGridLayout>
#include <QStackedLayout>
#include <QSplitter>
#include <QMainWindow>
#include "TabAbstract.h"
#include "TabFactory.h"
#include "../../OnlineDoc/include/WOnlineDoc.h"
#include "../../EmailService/SendEmail.h"
#include "../../ClipBoard/include/ClipboardView.h"
#include "FindDialog.h"

// 布局类型枚举
enum class LayoutType {
    LAYOUT_1X1, // 单视图
    LAYOUT_1X2, // 水平分割
    LAYOUT_2X1, // 垂直分割
    LAYOUT_2X2  // 网格分割
};  


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

    // 事件过滤器
    bool eventFilter(QObject *obj, QEvent *event) override;

    QTabWidget* getTabWidget() const { return tabWidget; }
    QWidget* getContainerWidget() const { return containerWidget; }

    void saveCurrentTab(QWidget *parent);
    void openFileFromMemory(const QString &fileName, const QByteArray &fileContent);
    void openSharedCSVTab(SharedView *sharedView);
    void findInCurrentTab(QWidget *parent);
    int addWidgetTab(QWidget *widget, const QString &displayName);
public:
    // 设置布局类型
    void setLayoutType(LayoutType type);
    // 获取当前布局类型
    LayoutType getCurrentLayoutType() const;
    // 在指定位置打开文件
    void openFileInPosition(const QString& filePath, int row, int col);
    // 在当前活动视图中打开文件
    void openFileInActiveView(const QString& filePath);
    // 获取活动视图索引
    QPair<int, int> getActiveViewPosition() const;
    // 设置活动视图
    void setActiveViewPosition(int row, int col);

private:
    TabAbstract* createTabByFileName(const QString& fileName);
    // 初始化布局
    void initLayout();
    // 更新布局
    void updateLayout();
    // 查找活动窗口所在的位置
    QPair<int, int> findActiveWidgetPosition();
    
    // FindDialog* findDialog;
    QTabWidget* tabWidget;
    QMap<QString, int> fileTabMap;
    
    // 布局相关成员
    LayoutType currentLayout = LayoutType::LAYOUT_1X1;
    QWidget* containerWidget; // 主容器部件
    QGridLayout* gridLayout; // 网格布局
    QVector<QVector<QTabWidget*>> viewTabs; // 多视图的标签页
    QPair<int, int> activePosition = {0, 0}; // 当前活动视图位置
};

#endif // TABMANAGER_H
