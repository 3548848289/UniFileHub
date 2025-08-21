#ifndef CLIPBOARDVIEW_H
#define CLIPBOARDVIEW_H

#include <QWidget>
#include <QClipboard>
#include <QListWidgetItem>
#include <QMenu>
#include <QMimeData>
#include <QScrollArea>
#include <QLabel>
#include <QDialog>
#include <QByteArray>
#include <QPixmap>
#include "../manager/include/dbService.h"
#include "../Setting/include/SettingManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ClipboardView; }
QT_END_NAMESPACE

class ImagePreviewDialog : public QDialog {
    Q_OBJECT
public:
    ImagePreviewDialog(const QPixmap& pixmap, QWidget *parent = nullptr);
};

class ClipboardView : public QWidget
{
    Q_OBJECT

public:
    explicit ClipboardView(QWidget *parent = nullptr);
    ~ClipboardView();

private slots:
    void onClipboardChanged();
    void copyItem();
    void deleteItem();
    void previewImage();

    void on_clearButton_clicked();
    void on_saveButton_clicked();
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_customContextMenuRequested(const QPoint &pos);

private:
    Ui::ClipboardView *ui;
    QClipboard *clipboard;
    QListWidgetItem *currentRightClickedItem = nullptr;
    dbService& dbservice;
    int initialItemCount = 0;

    // 新增：处理图片路径的辅助函数
    bool isImageFile(const QString& path);
    QPixmap loadImageFromPath(const QString& path);

    // 原有辅助函数
    void addTextItem(const QString& text);
    void addImageItem(const QPixmap& pixmap);
};

#endif // CLIPBOARDVIEW_H
