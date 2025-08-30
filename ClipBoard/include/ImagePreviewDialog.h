#ifndef IMAGEPREVIEWDIALOG_H
#define IMAGEPREVIEWDIALOG_H

#include <QDialog>
#include <QScrollArea>
#include <QLabel>
#include <QPixmap>

// 图片预览对话框（独立窗口，支持滚动查看大图）
class ImagePreviewDialog : public QDialog {
    Q_OBJECT
public:
    // 构造函数：传入要预览的图片
    explicit ImagePreviewDialog(const QPixmap& pixmap, QWidget *parent = nullptr);

private:
    QScrollArea* m_scrollArea; // 滚动区域（支持大图滚动）
    QLabel* m_imageLabel;      // 显示图片的标签
};

#endif // IMAGEPREVIEWDIALOG_H
