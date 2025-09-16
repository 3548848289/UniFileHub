#ifndef CLIPBOARDVIEW_H
#define CLIPBOARDVIEW_H

#include <QWidget>
#include <QClipboard>
#include<QLabel>
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
protected:
    // 重写：鼠标离开列表时隐藏预览
    void leaveEvent(QEvent *event) override;
    // 重写：鼠标在列表内移动时，若离开当前项则隐藏预览（避免残留）
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    Ui::ClipboardView *ui;
    QClipboard* m_clipboard;
    dbService& m_dbService;
    int m_initialItemCount;         // 初始加载的历史项数量（用于增量保存）
    QListWidgetItem* m_currentRightClickedItem; // 当前右键选中的项
    std::vector<std::unique_ptr<ClipboardItem>> m_clipboardItems; // 剪贴板项集合（智能指针管理）
    QLabel *m_imagePreviewLabel;
    bool loadImageToPreviewLabel(ClipboardItem* clipboardItem);
    void initializeListWidget();
    void loadHistory();
    void addClipboardItem(std::unique_ptr<ClipboardItem> item);
    void onItemEntered(QListWidgetItem *item);

    ClipboardItem* findItemForListWidgetItem(QListWidgetItem* listItem);
};



#endif // CLIPBOARDVIEW_H
