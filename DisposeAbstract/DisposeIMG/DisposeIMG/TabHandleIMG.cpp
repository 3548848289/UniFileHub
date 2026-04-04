#include "include/TabHandleIMG.h"
#include "include/WaterMark.h"
#include <QHBoxLayout>
#include <QtSvgWidgets/QGraphicsSvgItem>
#include <QtSvg/QSvgRenderer>
#include <QtCore/private/qzipreader_p.h>
#include <QBuffer>
#include <QDir>
#include <QStandardPaths>

TabHandleIMG::TabHandleIMG(const QString& filePath, QWidget *parent)
    : TabAbstract(filePath, parent), angle(0), scaleValue(1), shearValue(0), translateValue(0)
{
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    scene = new QGraphicsScene;
    view = new QGraphicsView;
    view->setScene(scene);
    view->installEventFilter(this);
    if (view->viewport()) {
        view->viewport()->installEventFilter(this);
    }

    controlFrame = new ControlFrame(this);
    splitter->addWidget(view);
    splitter->addWidget(controlFrame);
    splitter->setSizes({700, 100});

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    setLayout(layout);

    setWindowTitle(tr("Graphics Item Transformation"));
    showControlFrame(controlFrame);

    setContentModified(false);

    connect(controlFrame, &ControlFrame::textAdded, this, &TabHandleIMG::onTextAdded);
    connect(controlFrame, &ControlFrame::exportRequested, this, &TabHandleIMG::exportImage);

    drawTool = new DrawTool(view, scene, this);
    connect(controlFrame, &ControlFrame::watermarkModeToggled, this, [this](bool enabled) {
        watermarkMode = enabled;
        if (drawTool) {
            drawTool->setEnabled(false);
        }
    });
    connect(controlFrame, &ControlFrame::drawModeToggled, this, [this](bool enabled) {
        if (drawTool) {
            drawTool->setEnabled(enabled);
        }
        watermarkMode = false;
    });
    connect(controlFrame, &ControlFrame::finishDrawingRequested, this, [this]() {
        if (drawTool) {
            drawTool->finish();
            drawTool->setEnabled(false);
        }
    });
    connect(controlFrame, &ControlFrame::drawShapeChanged, this, [this](const QString &shapeKey) {
        if (!drawTool) return;
        if (shapeKey == "line") {
            drawTool->setShape(DrawTool::Shape::Line);
        } else {
            drawTool->setShape(DrawTool::Shape::Rect);
        }
    });
}


void TabHandleIMG::loadFromFile(const QString &fileName)
{
    scene->clear();
    isXmindFile = false;

    QFileInfo fileInfo(fileName);
    QString suffix = fileInfo.suffix().toLower();

    auto fitItemInView = [this](QGraphicsItem* item) {
        if (!item) return;
        scene->setSceneRect(item->boundingRect());
        item->setPos(0, 0);
        QTimer::singleShot(0, this, [this, item]() {
            view->fitInView(item, Qt::KeepAspectRatio);

            QTimer::singleShot(0, this, [this]() {
                double scaleFactor = view->transform().m11();
                int sliderVal = static_cast<int>(scaleFactor * 50.0);
                controlFrame->setScaleSliderValue(sliderVal);
            });

        });
    };

    // XMind 文件处理
    if (suffix == "xmind") {
        if (loadXmindThumbnail(fileName)) {
            return;
        }
        // 如果加载缩略图失败，继续尝试其他方式
    }

    if (suffix == "svg") {
        QGraphicsSvgItem* svgItem = new QGraphicsSvgItem(fileName);
        if (!svgItem->renderer()->isValid()) {
            QMessageBox::warning(this, tr("加载错误"), tr("SVG 文件无法打开: %1").arg(fileName));
            delete svgItem;
            return;
        }
        scene->addItem(svgItem);
        fitItemInView(svgItem);
    } else {
        QPixmap* pixmap = new QPixmap(fileName);
        if (pixmap->isNull()) {
            QMessageBox::warning(this, tr("加载错误"), tr("%1 文件无法打开").arg(fileName));
            delete pixmap;
            return;
        }
        pixItem = new PixItem(pixmap);
        scene->addItem(pixItem);
        fitItemInView(pixItem);
    }
}

bool TabHandleIMG::loadXmindThumbnail(const QString &fileName)
{
    // XMind 文件是 ZIP 格式，缩略图通常位于 Thumbnails/thumbnail.png
    QStringList possiblePaths = {
        "Thumbnails/thumbnail.png",
        "Thumbnails/thumbnail.jpg",
        "thumbnail.png",
        "thumbnail.jpg"
    };
    
    for (const QString &pathInZip : possiblePaths) {
        QByteArray thumbnailData = extractFileFromZip(fileName, pathInZip);
        if (!thumbnailData.isEmpty()) {
            QPixmap pixmap;
            if (pixmap.loadFromData(thumbnailData)) {
                pixItem = new PixItem(new QPixmap(pixmap));
                scene->addItem(pixItem);
                
                scene->setSceneRect(pixItem->boundingRect());
                pixItem->setPos(0, 0);
                QTimer::singleShot(0, this, [this]() {
                    view->fitInView(pixItem, Qt::KeepAspectRatio);
                    double scaleFactor = view->transform().m11();
                    int sliderVal = static_cast<int>(scaleFactor * 50.0);
                    controlFrame->setScaleSliderValue(sliderVal);
                });
                
                isXmindFile = true;
                return true;
            }
        }
    }
    
    return false;
}

QByteArray TabHandleIMG::extractFileFromZip(const QString &zipPath, const QString &fileNameInZip)
{
    QZipReader zipReader(zipPath);
    if (!zipReader.exists()) {
        return QByteArray();
    }
    
    QByteArray fileData = zipReader.fileData(fileNameInZip);
    zipReader.close();
    
    return fileData;
}



void TabHandleIMG::saveToFile(const QString &fileName) {
    exportImage(fileName);
}

void TabHandleIMG::test()
{
    // controlFrame->show();
    qDebug() << "TabHandleCSV: Showing control frame!";
}

void TabHandleIMG::onTextAdded(const QString &text, const QPointF &position) {
    addTextToImage(text, position);
}



void TabHandleIMG::showControlFrame(ControlFrame *controlFrame)
{
        connect(controlFrame, &ControlFrame::rotateChanged, this, [=](int value) {
            angle = value;
            updateTransformations(value, scaleValue, shearValue, translateValue);
        });
        connect(controlFrame, &ControlFrame::scaleChanged, this, [=](int value) {
            scaleValue = value / 50.0;
            updateTransformations(angle, scaleValue, shearValue, translateValue);
        });
        connect(controlFrame, &ControlFrame::shearChanged, this, [=](int value) {
            shearValue = value / 10.0;
            updateTransformations(angle, scaleValue, shearValue, translateValue);
        });
        connect(controlFrame, &ControlFrame::translateChanged, this, [=](int value) {
            translateValue = value;
            updateTransformations(angle, scaleValue, shearValue, translateValue);
        });
}

void TabHandleIMG::updateTransformations(int angle, qreal scale, qreal shear, qreal translate)
{
    view->resetTransform();
    view->rotate(angle);
    view->scale(scale, scale);
    view->shear(shear, 0);
    view->translate(translate, translate);
}

// void TabHandleIMG::updateTransformations(int angle, qreal scale, qreal shear, qreal translate)
// {
//     QTransform transform;
//     transform.translate(translate, translate);
//     transform.shear(shear, 0);
//     transform.scale(scale, scale);
//     transform.rotate(angle);

//     if (pixItem)
//         pixItem->setTransform(transform);
// }


void TabHandleIMG::addTextToImage(const QString &text, const QPointF &position) {
    WaterMark* textItem = new WaterMark(text);
    QPointF offset(0, -12);
    textItem->setFont(QFont("Arial", 12));

    textItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
    textItem->setPos(position + offset);
    scene->addItem(textItem);

    setContentModified(true);
}

void TabHandleIMG::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (view && scene && !scene->items().isEmpty()) {
        view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

bool TabHandleIMG::eventFilter(QObject* watched, QEvent* event)
{
    const bool isViewTarget = (watched == view || watched == view->viewport());

    if (isViewTarget && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (drawTool && drawTool->isEnabled()) {
            return drawTool->handleMousePress(mouseEvent->pos(), mouseEvent->button());
        }
        if (watermarkMode && mouseEvent->button() == Qt::LeftButton) {
            const QPointF scenePos = view->mapToScene(mouseEvent->pos());
            bool ok = false;
            const QString text = QInputDialog::getText(
                this, tr("输入文字"), tr("请输入水印文字:"), QLineEdit::Normal, "", &ok);
            if (ok && !text.isEmpty()) {
                addTextToImage(text, scenePos);
            }
            return true;
        }
    } else if (isViewTarget && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            // 获取当前缩放滑块的值
            int currentValue = controlFrame->getScaleSliderValue();
            // 根据滚轮方向调整缩放值
            int delta = wheelEvent->angleDelta().y() > 0 ? 5 : -5;
            int newValue = currentValue + delta;
            // 确保值在滑块范围内
            if (newValue < controlFrame->getScaleSliderMinimum())
                newValue = controlFrame->getScaleSliderMinimum();
            if (newValue > controlFrame->getScaleSliderMaximum())
                newValue = controlFrame->getScaleSliderMaximum();
            // 设置新的缩放值
            controlFrame->setScaleSliderValue(newValue);
            // 更新缩放值
            scaleValue = newValue / 50.0;
            updateTransformations(angle, scaleValue, shearValue, translateValue);
            return true;
        }
    } else if (isViewTarget && event->type() == QEvent::MouseMove) {
        if (drawTool && drawTool->isEnabled()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            return drawTool->handleMouseMove(mouseEvent->pos(), mouseEvent->buttons());
        }
    } else if (isViewTarget && event->type() == QEvent::MouseButtonRelease) {
        if (drawTool && drawTool->isEnabled()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            return drawTool->handleMouseRelease(mouseEvent->pos(), mouseEvent->button());
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TabHandleIMG::exportImage(const QString &filePath) {
    QRectF sceneRect = scene->itemsBoundingRect();
    QRect imageRect = sceneRect.toAlignedRect();
    QImage image(imageRect.size(), QImage::Format_ARGB32);
    image.fill(Qt::white);

    // 将 scene 渲染到 QImage
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(-sceneRect.topLeft());
    scene->render(&painter);
    painter.end();

    if (!image.save(filePath)) {
        QMessageBox::warning(this, tr("保存失败"), tr("无法保存图像到 %1").arg(filePath));
    }
    setContentModified(false);
}
