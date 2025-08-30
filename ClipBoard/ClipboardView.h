#ifndef CLIPBOARDVIEW_H
#define CLIPBOARDVIEW_H

#include <QWidget>
#include <QClipboard>
#include <QListWidgetItem>
#include <vector>
#include <memory>
#include "ClipboardItem.h"
#include "../manager/include/dbService.h" // 数据库服务（外部依赖）
#include "../Setting/include/SettingManager.h" // 配置管理（外部依赖）

// 前置声明UI类（由Qt Designer生成）
QT_BEGIN_NAMESPACE
namespace Ui { class ClipboardView; }
QT_END_NAMESPACE

// 主剪贴板视图类（负责UI交互、数据管理）
class ClipboardView : public QWidget {
    Q_OBJECT

public:
    ClipboardView(QWidget *parent = nullptr);
    ~ClipboardView() override;

private slots:
    // 剪贴板数据变化触发
    void onClipboardChanged();
    // 按钮点击事件
    void on_clearButton_clicked();
    void on_saveButton_clicked();
    // 列表项双击事件（复制到剪贴板）
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    // 列表右键菜单触发
    void on_listWidget_customContextMenuRequested(const QPoint &pos);

    // 右键菜单动作
    void copyItem();       // 复制项到剪贴板
    void previewImage();   // 预览图片
    void deleteItem();     // 删除选中项
    void openFileLocation();// 打开文件所在目录

private:
    Ui::ClipboardView *ui;                 // UI对象
    QClipboard* m_clipboard;               // 系统剪贴板
    dbService& m_dbService;                // 数据库服务（单例）
    int m_initialItemCount;                // 初始加载的历史项数量（用于增量保存）
    QListWidgetItem* m_currentRightClickedItem; // 当前右键选中的项
    std::vector<std::unique_ptr<ClipboardItem>> m_clipboardItems; // 剪贴板项集合（智能指针管理）

    // 初始化列表控件（设置样式、选择模式等）
    void initializeListWidget();
    // 加载历史记录（从数据库读取并通过工厂类创建项）
    void loadHistory();
    // 添加剪贴板项（去重 + UI更新）
    void addClipboardItem(std::unique_ptr<ClipboardItem> item);
    // 根据列表项找到对应的ClipboardItem（通过UserRole存储的指针）
    ClipboardItem* findItemForListWidgetItem(QListWidgetItem* listItem);
};

#endif // CLIPBOARDVIEW_H
