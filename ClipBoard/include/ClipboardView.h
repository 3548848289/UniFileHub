#ifndef CLIPBOARDVIEW_H
#define CLIPBOARDVIEW_H

#include <QLabel>
#include <QListWidgetItem>
#include <QWidget>
#include "ClipboardItem/ClipboardItem.h"
#include "ClipboardItemDelegate.h"

class ClipboardController;
class InlineMessagePopup;
class QPropertyAnimation;

QT_BEGIN_NAMESPACE
namespace Ui { class ClipboardView; }
QT_END_NAMESPACE

class ClipboardView : public QWidget {
    Q_OBJECT

public:
    explicit ClipboardView(ClipboardController* controller, QWidget *parent = nullptr);
    ~ClipboardView() override;
    void refreshCloudItems();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void onItemAdded(ClipboardItem* item);
    void onItemRemoved(ClipboardItem* item);
    void onModelCleared();
    void onItemPinnedChanged(ClipboardItem* item);
    void refreshAllItems();

private slots:
    void on_clearButton_clicked();
    void on_clearCloudButton_clicked();
    void on_saveButton_clicked();
    void on_refreshButton_clicked();
    void on_quickSyncButton_clicked();
    void on_quickCopyButton_clicked();
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_customContextMenuRequested(const QPoint &pos);
    void on_lineEdit_editingFinished();
    void on_lineEdit_textChanged(const QString &text);
    void on_lineEdit_returnPressed();
    void on_showPinnedCheckBox_toggled(bool checked);
    void copyItem();
    void previewImage();
    void openItem();
    void deleteItem();
    void openFileLocation();
    void pinItem();
    void syncItemToCloud();
    void unsyncItemFromCloud();
    void filterItemsByType(ClipboardItemType type);
    void on_typeComboBox_currentIndexChanged(int index);

private:
    Ui::ClipboardView *ui;
    ClipboardController* m_controller;
    QListWidgetItem* m_currentRightClickedItem;
    QLabel *m_imagePreviewLabel;
    QPropertyAnimation* m_smoothScrollAnimation;
    int m_smoothScrollTarget;
    // 快速同步/快速复制的悬浮提示条
    InlineMessagePopup *m_quickToast;

    void initializeListWidget();
    void initializeSmoothScrolling();
    void smoothScrollBy(int delta);
    void insertNewItem(ClipboardItem *newItem);
    void addItemToListWidget(ClipboardItem *item);
    void applyItemFont(QListWidgetItem *listItem) const;
    bool shouldDisplayItem(ClipboardItem *item) const;
    ClipboardItemType currentFilterType() const;
    ClipboardItem* findItemForListWidgetItem(QListWidgetItem* listItem);
    QListWidgetItem* findListWidgetItemForClipboardItem(ClipboardItem* item);
    void collapseWindow();
    void copyItemAndMaybeCollapse(bool shouldCollapse);
    void updateSequenceNumbers();

signals:
    // 图片记录请求打开:图片路径(纯位图记录为临时 PNG)+ tab 显示名
    void openImageRequested(const QString &imagePath, const QString &tabName);
    // 文本记录请求打开:文本内容(主窗口用文本编辑器打开)
    void openTextRequested(const QString &textContent);
};

#endif // CLIPBOARDVIEW_H
