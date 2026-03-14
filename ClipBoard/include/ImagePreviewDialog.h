#ifndef IMAGEPREVIEWDIALOG_H
#define IMAGEPREVIEWDIALOG_H

#include <QDialog>
#include <QScrollArea>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>

// 图片预览对话框（独立窗口，图片按比例缩放以适应固定窗口大小）
class ImagePreviewDialog : public QDialog {
    Q_OBJECT
public:
    // 构造函数：传入要预览的图片
    explicit ImagePreviewDialog(const QPixmap& pixmap, QWidget *parent = nullptr);

protected:
    // 重写resize事件，在窗口大小变化时重新计算图片缩放
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateScaledPixmap(); // 更新缩放后的图片

    QScrollArea* m_scrollArea; // 滚动区域
    QLabel* m_imageLabel;      // 显示图片的标签
    QPixmap m_originalPixmap;  // 保存原始图片数据
};

#endif // IMAGEPREVIEWDIALOG_H
