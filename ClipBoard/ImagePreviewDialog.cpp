#include "include/ImagePreviewDialog.h"
#include <QVBoxLayout>
#include <Qt>

ImagePreviewDialog::ImagePreviewDialog(const QPixmap& pixmap, QWidget *parent)
    : QDialog(parent), m_originalPixmap(pixmap) {
    // 窗口基础设置
    setWindowTitle("图片预览");
    // 设置窗口标志：显示最大化按钮，允许调整大小
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    resize(800, 600); // 设置初始大小
    setWindowModality(Qt::ApplicationModal); // 模态窗口（阻塞其他操作）

    // 图片标签：居中显示
    m_imageLabel = new QLabel(this);
    m_imageLabel->setScaledContents(true);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    // 滚动区域：包裹图片标签
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignCenter);

    // 布局：将滚动区域铺满窗口
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_scrollArea);
    layout->setContentsMargins(10, 10, 10, 10); // 设置适当边距
    setLayout(layout);

    // 计算并设置缩放后的图片
    updateScaledPixmap();
}

void ImagePreviewDialog::updateScaledPixmap()
{
    if (m_originalPixmap.isNull()) return;

    // 获取可用显示区域（减去边距）
    int margin = 20; // 左右/上下边距总和
    int availableWidth = width() - margin;
    int availableHeight = height() - margin;

    // 按比例缩放图片以适应窗口，保持宽高比
    QPixmap scaledPixmap = m_originalPixmap.scaled(
        availableWidth,
        availableHeight,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    m_imageLabel->setPixmap(scaledPixmap);
    m_imageLabel->setFixedSize(scaledPixmap.size());
}

void ImagePreviewDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    updateScaledPixmap();
}
