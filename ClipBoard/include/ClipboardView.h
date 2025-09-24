#ifndef CLIPBOARDVIEW_H
#define CLIPBOARDVIEW_H

#include <QWidget>
#include <QClipboard>
#include <QLabel>
#include <QListWidgetItem>
#include <memory>
#include "ClipboardItem/ClipboardItem.h"
#include "ClipboardHistoryManager.h"
#include "../../Setting/include/SettingManager.h"
#include "ClipboardMonitor.h"


QT_BEGIN_NAMESPACE
namespace Ui { class ClipboardView; }
QT_END_NAMESPACE

class ClipboardView : public QWidget {
    Q_OBJECT

public:
    explicit ClipboardView(QWidget *parent = nullptr);
    ~ClipboardView() override;

private slots:
    void on_clearButton_clicked();
    void on_saveButton_clicked();
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_customContextMenuRequested(const QPoint &pos);

    void copyItem();
    void previewImage();
    void deleteItem();
    void openFileLocation();

    void pinItem();   // 置顶/取消置顶

private:
    Ui::ClipboardView *ui;
    ClipboardMonitor* m_clipboardMonitor;
    ClipboardHistoryManager m_historyManager;
    QListWidgetItem* m_currentRightClickedItem;
    QLabel *m_imagePreviewLabel;

    void initializeListWidget();
    void insertNewItem(ClipboardItem *newItem);
    void refreshUI(); // 从 historyManager 刷新 UI
    ClipboardItem* findItemForListWidgetItem(QListWidgetItem* listItem);
};

#endif // CLIPBOARDVIEW_H
