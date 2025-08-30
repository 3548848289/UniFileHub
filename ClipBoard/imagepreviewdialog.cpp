#include "include/ImagePreviewDialog.h"
#include <QVBoxLayout>
#include <Qt>

ImagePreviewDialog::ImagePreviewDialog(const QPixmap& pixmap, QWidget *parent)
    : QDialog(parent) {
    // 窗口基础设置
    setWindowTitle("图片预览");
    setMinimumSize(600, 400); // 最小尺寸
    setWindowModality(Qt::ApplicationModal); // 模态窗口（阻塞其他操作）

    // 图片标签：居中显示，不自动缩放（保持原图比例）
    m_imageLabel = new QLabel(this);
    m_imageLabel->setPixmap(pixmap);
    m_imageLabel->setScaledContents(false);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    // 滚动区域：包裹图片标签，支持滚动查看大图
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(true); // 允许滚动区域自适应窗口
    m_scrollArea->setAlignment(Qt::AlignCenter);

    // 布局：将滚动区域铺满窗口
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_scrollArea);
    layout->setContentsMargins(0, 0, 0, 0); // 消除边距
    setLayout(layout);
}
