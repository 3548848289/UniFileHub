#ifndef CLIPBOARDVIEW_H
#define CLIPBOARDVIEW_H

#include <QLabel>
#include <QListWidgetItem>
#include <QWidget>
#include "ClipboardItem/ClipboardItem.h"
#include "ClipboardItemDelegate.h"

class ClipboardController;

QT_BEGIN_NAMESPACE
namespace Ui { class ClipboardView; }
QT_END_NAMESPACE

class ClipboardView : public QWidget {
    Q_OBJECT

public:
    explicit ClipboardView(ClipboardController* controller, QWidget *parent = nullptr);
    ~ClipboardView() override;
    void refreshCloudItems();

public slots:
    void onItemAdded(ClipboardItem* item);
    void onItemRemoved(ClipboardItem* item);
    void onModelCleared();
    void onItemPinnedChanged(ClipboardItem* item);
    void refreshAllItems();

private slots:
    void on_clearButton_clicked();
    void on_saveButton_clicked();
    void on_refreshButton_clicked();
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_customContextMenuRequested(const QPoint &pos);
    void on_lineEdit_editingFinished();
    void on_lineEdit_textChanged(const QString &text);
    void on_lineEdit_returnPressed();
    void copyItem();
    void previewImage();
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

    void initializeListWidget();
    void insertNewItem(ClipboardItem *newItem);
    ClipboardItem* findItemForListWidgetItem(QListWidgetItem* listItem);
    QListWidgetItem* findListWidgetItemForClipboardItem(ClipboardItem* item);
    void copyItemAndCollapseWindow();
    void updateSequenceNumbers();
};

#endif // CLIPBOARDVIEW_H
