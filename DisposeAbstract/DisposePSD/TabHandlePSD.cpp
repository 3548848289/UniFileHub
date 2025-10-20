#include "include/TabHandlePSD.h"
#include "qpsdhandler.h"
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QGraphicsPixmapItem>
#include <QDebug>


TabHandlePSD::TabHandlePSD(const QString& filePath, QWidget *parent)
    : TabAbstract(filePath, parent), scaleValue(1.0)
{
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(this);
    view->setScene(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    view->setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    // 创建控制控件
    controlFrame = new ControlWidPSD(this);

    // 用 QSplitter 放置视图和控件
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(view);
    splitter->addWidget(controlFrame);
    splitter->setSizes({700, 100});  // 高度比例，可调

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    setContentModified(false);


    // 连接信号槽
    connect(controlFrame, &ControlWidPSD::zoomValueChanged, this, [this](int value) {
        // 计算缩放比例
        qreal scaleFactor = value / 100.0;
        // 先重置变换矩阵，然后应用新的缩放
        QTransform transform;
        transform.scale(scaleFactor, scaleFactor);
        view->setTransform(transform);
        scaleValue = scaleFactor;
    });

    // 如果有 PSD 文件，直接加载
    if (!filePath.isEmpty()) {
        // 使用QTimer延迟加载，确保窗口已经完全初始化
        QTimer::singleShot(50, this, [this, filePath]() {
            loadFromFile(filePath);
        });
    }
}


void TabHandlePSD::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("加载错误"), tr("无法打开文件: %1").arg(fileName));
        return;
    }

    QPsdHandler handler;
    handler.setDevice(&file);

    if (!handler.canRead()) {
        QMessageBox::warning(this, tr("加载错误"), tr("不是有效的 PSD 文件。"));
        return;
    }

    QImage image;
    if (!handler.read(&image)) {
        QMessageBox::warning(this, tr("加载错误"), tr("PSD 文件读取失败。"));
        return;
    }

    scene->clear();
    pixItem = scene->addPixmap(QPixmap::fromImage(image));
    scene->setSceneRect(image.rect());
    
    // 使用QTimer延迟适应视图，确保窗口已经完全初始化
    QTimer::singleShot(100, this, [this]() {
        if (pixItem) {
            fitItemInView(pixItem);
            // 更新滑块的值以匹配当前缩放
            if (controlFrame) {
                int zoomValue = static_cast<int>(scaleValue * 100);
                controlFrame->findChild<QSlider*>("zoomSlider")->setValue(zoomValue);
            }
        }
    });
    
    clearModified();
}

void TabHandlePSD::saveToFile(const QString &fileName)
{
    // PSD文件不支持编辑和保存，只提供查看功能
    QMessageBox::information(this, tr("信息"), tr("PSD文件仅支持查看，不支持编辑和保存。"));
}

void TabHandlePSD::ControlWidget(bool judge)
{
    qDebug() << "TabHandlePSD: 控制控件显示:" << judge;

    if (judge && !controlFrame) {
        // 创建控制框架
        controlFrame = new ControlWidPSD(this);
        // 连接信号槽
        connect(controlFrame, &ControlWidPSD::zoomValueChanged, this, [this](int value) {
            // 计算缩放比例
            qreal scaleFactor = value / 100.0;
            // 先重置变换矩阵，然后应用新的缩放
            QTransform transform;
            transform.scale(scaleFactor, scaleFactor);
            view->setTransform(transform);
            scaleValue = scaleFactor;
        });

        // 在布局中添加控制框架
        QVBoxLayout *layout = static_cast<QVBoxLayout*>(this->layout());
        layout->addWidget(controlFrame);
    } else if (!judge && controlFrame) {
        // 移除控制框架
        delete controlFrame;
        controlFrame = nullptr;
    }
}

// 滑块控制缩放，不再需要按钮控制的缩放函数

void TabHandlePSD::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 窗口大小改变时，保持图片适应视图
    if (pixItem && scaleValue == 1.0) {
        fitItemInView(pixItem);
    }
}

bool TabHandlePSD::eventFilter(QObject *watched, QEvent *event)
{
    // 可以在这里添加鼠标滚轮缩放等事件处理
    return QWidget::eventFilter(watched, event);
}

void TabHandlePSD::fitItemInView(QGraphicsItem* item)
{
    if (!item || !view) return;

    QRectF boundingRect = item->boundingRect();
    if (boundingRect.isValid()) {
        view->fitInView(boundingRect, Qt::KeepAspectRatio);
        // 获取当前的变换矩阵以计算实际的缩放值
        scaleValue = view->transform().m11();
    }
}
