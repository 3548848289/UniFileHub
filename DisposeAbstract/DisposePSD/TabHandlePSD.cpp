#include "include/TabHandlePSD.h"

#include "qpsdhandler.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QMessageBox>
#include <QSlider>

TabHandlePSD::TabHandlePSD(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent),
      view(new QGraphicsView(this)),
      scene(new QGraphicsScene(this)),
      pixItem(nullptr),
      controlFrame(new ControlWidPSD(this)),
      scaleValue(1.0)
{
    view->setScene(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    view->setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(view);
    splitter->addWidget(controlFrame);
    splitter->setSizes({700, 100});

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    setContentModified(false);

    connect(controlFrame, &ControlWidPSD::zoomValueChanged, this, [this](int value) {
        const qreal scaleFactor = value / 100.0;
        QTransform transform;
        transform.scale(scaleFactor, scaleFactor);
        view->setTransform(transform);
        scaleValue = scaleFactor;
    });
    connect(controlFrame, &ControlWidPSD::exportRequested, this, &TabHandlePSD::exportToPng);

    if (!filePath.isEmpty()) {
        QTimer::singleShot(50, this, [this, filePath]() {
            loadFromFile(filePath);
        });
    }
}

void TabHandlePSD::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("加载失败"), tr("无法打开文件：%1").arg(fileName));
        return;
    }

    QPsdHandler handler;
    handler.setDevice(&file);

    if (!handler.canRead()) {
        QMessageBox::warning(this, tr("加载失败"), tr("这不是有效的 PSD 文件。"));
        return;
    }

    QImage image;
    if (!handler.read(&image)) {
        QMessageBox::warning(this, tr("加载失败"), tr("PSD 文件读取失败。"));
        return;
    }

    scene->clear();
    pixItem = scene->addPixmap(QPixmap::fromImage(image));
    scene->setSceneRect(image.rect());

    QTimer::singleShot(100, this, [this]() {
        if (!pixItem) {
            return;
        }

        fitItemInView(pixItem);
        if (controlFrame) {
            if (QSlider *zoomSlider = controlFrame->findChild<QSlider *>(QStringLiteral("zoomSlider"))) {
                zoomSlider->setValue(static_cast<int>(scaleValue * 100));
            }
        }
    });

    clearModified();
}

void TabHandlePSD::saveToFile(const QString &fileName)
{
    Q_UNUSED(fileName);
    QMessageBox::information(this, tr("提示"), tr("PSD 文件仅支持查看，不支持直接保存编辑内容。"));
}

void TabHandlePSD::ControlWidget(bool judge)
{
    qDebug() << "TabHandlePSD control widget visible:" << judge;

    if (judge && !controlFrame) {
        controlFrame = new ControlWidPSD(this);
        connect(controlFrame, &ControlWidPSD::zoomValueChanged, this, [this](int value) {
            const qreal scaleFactor = value / 100.0;
            QTransform transform;
            transform.scale(scaleFactor, scaleFactor);
            view->setTransform(transform);
            scaleValue = scaleFactor;
        });
        connect(controlFrame, &ControlWidPSD::exportRequested, this, &TabHandlePSD::exportToPng);

        if (QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(this->layout())) {
            layout->addWidget(controlFrame);
        }
    } else if (!judge && controlFrame) {
        delete controlFrame;
        controlFrame = nullptr;
    }
}

void TabHandlePSD::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (pixItem && scaleValue == 1.0) {
        fitItemInView(pixItem);
    }
}

bool TabHandlePSD::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    Q_UNUSED(event);
    return QWidget::eventFilter(watched, event);
}

void TabHandlePSD::fitItemInView(QGraphicsItem *item)
{
    if (!item || !view) {
        return;
    }

    const QRectF boundingRect = item->boundingRect();
    if (boundingRect.isValid()) {
        view->fitInView(boundingRect, Qt::KeepAspectRatio);
        scaleValue = view->transform().m11();
    }
}

void TabHandlePSD::exportToPng()
{
    if (!pixItem || pixItem->pixmap().isNull()) {
        QMessageBox::warning(this, tr("导出 PNG"), tr("当前没有可导出的 PSD 图像。"));
        return;
    }

    const QFileInfo fileInfo(currentFilePath);
    const QString defaultName = fileInfo.completeBaseName().isEmpty()
        ? QStringLiteral("psd-export.png")
        : fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".png";

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("导出 PNG"),
        defaultName,
        tr("PNG 图片 (*.png)"));

    if (filePath.isEmpty()) {
        return;
    }

    if (!filePath.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
        filePath += QStringLiteral(".png");
    }

    if (!pixItem->pixmap().save(filePath, "PNG")) {
        QMessageBox::warning(this, tr("导出 PNG"), tr("PNG 导出失败。"));
        return;
    }

    QMessageBox::information(this, tr("导出 PNG"), tr("PNG 导出成功。"));
}
