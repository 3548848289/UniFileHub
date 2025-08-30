#ifndef CLIPBOARDVIEW_H
#define CLIPBOARDVIEW_H

#include <QWidget>
#include <QClipboard>
#include <QListWidgetItem>
#include <vector>
#include <memory>
#include "ClipboardItem/ClipboardItem.h"
#include "../../manager/include/dbService.h"
#include "../../Setting/include/SettingManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ClipboardView; }
QT_END_NAMESPACE

class ClipboardView : public QWidget {
    Q_OBJECT

public:
    ClipboardView(QWidget *parent = nullptr);
    ~ClipboardView() override;

private slots:
    // 剪贴板数据变化触发
    void onClipboardChanged();

    void on_clearButton_clicked();
    void on_saveButton_clicked();
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_customContextMenuRequested(const QPoint &pos);

    void copyItem();
    void previewImage();
    void deleteItem();
    void openFileLocation();

private:
    Ui::ClipboardView *ui;
    QClipboard* m_clipboard;
    dbService& m_dbService;
    int m_initialItemCount;         // 初始加载的历史项数量（用于增量保存）
    QListWidgetItem* m_currentRightClickedItem; // 当前右键选中的项
    std::vector<std::unique_ptr<ClipboardItem>> m_clipboardItems; // 剪贴板项集合（智能指针管理）

    void initializeListWidget();
    void loadHistory();
    void addClipboardItem(std::unique_ptr<ClipboardItem> item);

    ClipboardItem* findItemForListWidgetItem(QListWidgetItem* listItem);
};

#endif // CLIPBOARDVIEW_H
