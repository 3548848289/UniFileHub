#include "include/TabHandleIMG.h"

#include "include/WaterMark.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QGraphicsItem>
#include <QHBoxLayout>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QClipboard>
#include <QWheelEvent>
#include <QtCore/private/qzipreader_p.h>
#include <QtSvg/QSvgRenderer>
#include <QtSvgWidgets/QGraphicsSvgItem>

TabHandleIMG::TabHandleIMG(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent)
{
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(this);
    view->setScene(scene);
    view->installEventFilter(this);
    if (view->viewport()) {
        view->viewport()->installEventFilter(this);
    }

    controlFrame = new ControlFrame(this);
    drawToolPanel = new DrawToolPanel(this);
    drawToolPanel->hide();
    drawTool = new DrawTool(view, scene, this);

    setupEditorLayout();
    setupToolConnections();

    setWindowTitle(tr("Graphics Item Transformation"));
    showControlFrame(controlFrame);
    setContentModified(false);
}

void TabHandleIMG::setupEditorLayout()
{
    QSplitter *splitter = new QSplitter(Qt::Vertical, this);

    editorArea = new QWidget(this);
    QHBoxLayout *editorLayout = new QHBoxLayout(editorArea);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(12);
    editorLayout->addWidget(view, 1);
    editorLayout->addWidget(drawToolPanel, 0, Qt::AlignTop);

    splitter->addWidget(editorArea);
    splitter->addWidget(controlFrame);
    splitter->setSizes({700, 120});

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);
    setLayout(layout);
}

void TabHandleIMG::setupToolConnections()
{
    connect(controlFrame, &ControlFrame::drawPanelToggled, this, &TabHandleIMG::setToolPanelVisible);
    connect(drawToolPanel, &DrawToolPanel::toolSelected, this, &TabHandleIMG::applyToolSelection);
    connect(drawToolPanel, &DrawToolPanel::exportRequested, this, &TabHandleIMG::requestExport);
    connect(drawToolPanel, &DrawToolPanel::copyRequested, this, &TabHandleIMG::copyToClipboard);
    connect(drawToolPanel, &DrawToolPanel::clearRequested, this, &TabHandleIMG::clearOverlayItems);
}

void TabHandleIMG::setToolPanelVisible(bool visible)
{
    if (drawToolPanel) {
        drawToolPanel->setVisible(visible);
    }

    if (visible) {
        return;
    }

    if (drawToolPanel) {
        drawToolPanel->clearSelection();
    }
    updateToolMode(ToolMode::None);
}

void TabHandleIMG::applyToolSelection(const QString &toolKey)
{
    if (toolKey == QStringLiteral("select")) {
        updateToolMode(ToolMode::Select);
        return;
    }
    if (toolKey == QStringLiteral("rect")) {
        updateToolMode(ToolMode::Rect);
        return;
    }
    if (toolKey == QStringLiteral("line")) {
        updateToolMode(ToolMode::Line);
        return;
    }
    if (toolKey == QStringLiteral("watermark")) {
        updateToolMode(ToolMode::Watermark);
        return;
    }

    updateToolMode(ToolMode::None);
}

void TabHandleIMG::updateToolMode(ToolMode mode)
{
    currentToolMode = mode;
    watermarkMode = (mode == ToolMode::Watermark);

    if (drawTool) {
        drawTool->finish();
        drawTool->setEnabled(false);
        if (mode == ToolMode::Rect) {
            drawTool->setShape(DrawTool::Shape::Rect);
            drawTool->setEnabled(true);
        } else if (mode == ToolMode::Line) {
            drawTool->setShape(DrawTool::Shape::Line);
            drawTool->setEnabled(true);
        }
    }

    setSceneItemsInteractive(mode == ToolMode::Select);
}

void TabHandleIMG::setSceneItemsInteractive(bool interactive)
{
    if (!scene) {
        return;
    }

    const QList<QGraphicsItem *> items = scene->items();
    for (QGraphicsItem *item : items) {
        if (!item || item == contentItem) {
            continue;
        }

        QGraphicsItem::GraphicsItemFlags flags = item->flags();
        flags.setFlag(QGraphicsItem::ItemIsMovable, interactive);
        flags.setFlag(QGraphicsItem::ItemIsSelectable, interactive);
        flags.setFlag(QGraphicsItem::ItemSendsGeometryChanges, interactive);
        item->setFlags(flags);
    }
}

void TabHandleIMG::requestExport()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Image"),
        QString(),
        tr("Images (*.png *.jpg *.bmp)"));
    if (!filePath.isEmpty()) {
        exportImage(filePath);
    }
}

void TabHandleIMG::copyToClipboard()
{
    const QImage image = renderCompositeImage();
    if (image.isNull()) {
        return;
    }

    if (QClipboard *clipboard = QGuiApplication::clipboard()) {
        clipboard->setImage(image);
    }
}

void TabHandleIMG::clearOverlayItems()
{
    if (!scene) {
        return;
    }

    if (drawTool) {
        drawTool->finish();
    }

    QList<QGraphicsItem *> itemsToDelete;
    const QList<QGraphicsItem *> items = scene->items();
    for (QGraphicsItem *item : items) {
        if (!item || item == contentItem) {
            continue;
        }
        itemsToDelete.append(item);
    }

    for (QGraphicsItem *item : itemsToDelete) {
        scene->removeItem(item);
        delete item;
    }

    if (!itemsToDelete.isEmpty()) {
        setContentModified(true);
    }

    setSceneItemsInteractive(currentToolMode == ToolMode::Select);
}

QImage TabHandleIMG::renderCompositeImage() const
{
    if (!scene) {
        return QImage();
    }

    const QRectF sceneRect = scene->itemsBoundingRect();
    if (sceneRect.isEmpty()) {
        return QImage();
    }

    const QRect imageRect = sceneRect.toAlignedRect();
    QImage image(imageRect.size(), QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(-sceneRect.topLeft());
    scene->render(&painter);
    painter.end();

    return image;
}

int TabHandleIMG::defaultWatermarkPointSize() const
{
    QRectF targetRect;
    if (contentItem) {
        targetRect = contentItem->boundingRect();
    } else if (scene) {
        targetRect = scene->itemsBoundingRect();
    }

    const qreal minDimension = qMin(targetRect.width(), targetRect.height());
    if (minDimension <= 0) {
        return 18;
    }

    return qBound(12, qRound(minDimension * 0.04), 72);
}

void TabHandleIMG::loadFromFile(const QString &fileName)
{
    scene->clear();
    contentItem = nullptr;
    pixItem = nullptr;
    isXmindFile = false;

    const QFileInfo fileInfo(fileName);
    const QString suffix = fileInfo.suffix().toLower();

    auto fitItemInView = [this](QGraphicsItem *item) {
        if (!item) {
            return;
        }

        scene->setSceneRect(item->sceneBoundingRect());
        item->setPos(0, 0);
        QTimer::singleShot(0, this, [this]() {
            if (!scene || !view || scene->sceneRect().isEmpty()) {
                return;
            }
            view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
            QTimer::singleShot(0, this, [this]() {
                if (!view || !controlFrame) {
                    return;
                }
                const double scaleFactor = view->transform().m11();
                const int sliderVal = static_cast<int>(scaleFactor * 50.0);
                controlFrame->setScaleSliderValue(sliderVal);
            });
        });
    };

    if (suffix == QStringLiteral("xmind") && loadXmindThumbnail(fileName)) {
        setSceneItemsInteractive(currentToolMode == ToolMode::Select);
        return;
    }

    if (suffix == QStringLiteral("svg")) {
        QGraphicsSvgItem *svgItem = new QGraphicsSvgItem(fileName);
        if (!svgItem->renderer()->isValid()) {
            QMessageBox::warning(this, tr("加载错误"), tr("SVG 文件无法打开: %1").arg(fileName));
            delete svgItem;
            return;
        }
        contentItem = svgItem;
        scene->addItem(svgItem);
        fitItemInView(svgItem);
        setSceneItemsInteractive(currentToolMode == ToolMode::Select);
        return;
    }

    QPixmap pixmap(fileName);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, tr("加载错误"), tr("%1 文件无法打开").arg(fileName));
        return;
    }

    pixItem = new PixItem(&pixmap);
    contentItem = pixItem;
    scene->addItem(pixItem);
    fitItemInView(pixItem);
    setSceneItemsInteractive(currentToolMode == ToolMode::Select);
}

bool TabHandleIMG::loadXmindThumbnail(const QString &fileName)
{
    const QStringList possiblePaths = {
        QStringLiteral("Thumbnails/thumbnail.png"),
        QStringLiteral("Thumbnails/thumbnail.jpg"),
        QStringLiteral("thumbnail.png"),
        QStringLiteral("thumbnail.jpg")
    };

    for (const QString &pathInZip : possiblePaths) {
        const QByteArray thumbnailData = extractFileFromZip(fileName, pathInZip);
        if (thumbnailData.isEmpty()) {
            continue;
        }

        QPixmap pixmap;
        if (!pixmap.loadFromData(thumbnailData)) {
            continue;
        }

        pixItem = new PixItem(&pixmap);
        contentItem = pixItem;
        scene->addItem(pixItem);
        scene->setSceneRect(pixItem->sceneBoundingRect());
        pixItem->setPos(0, 0);
        QTimer::singleShot(0, this, [this]() {
            if (!scene || !view || scene->sceneRect().isEmpty()) {
                return;
            }
            view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
            if (!controlFrame) {
                return;
            }
            const double scaleFactor = view->transform().m11();
            const int sliderVal = static_cast<int>(scaleFactor * 50.0);
            controlFrame->setScaleSliderValue(sliderVal);
        });

        isXmindFile = true;
        return true;
    }

    return false;
}

QByteArray TabHandleIMG::extractFileFromZip(const QString &zipPath, const QString &fileNameInZip)
{
    QZipReader zipReader(zipPath);
    if (!zipReader.exists()) {
        return QByteArray();
    }

    const QByteArray fileData = zipReader.fileData(fileNameInZip);
    zipReader.close();
    return fileData;
}

void TabHandleIMG::saveToFile(const QString &fileName)
{
    exportImage(fileName);
}

void TabHandleIMG::test()
{
    qDebug() << "TabHandleIMG: Showing control frame!";
}

void TabHandleIMG::showControlFrame(ControlFrame *controlFrame)
{
    connect(controlFrame, &ControlFrame::rotateChanged, this, [this](int value) {
        angle = value;
        updateTransformations(angle, scaleValue, shearValue, translateValue);
    });
    connect(controlFrame, &ControlFrame::scaleChanged, this, [this](int value) {
        scaleValue = value / 50.0;
        updateTransformations(angle, scaleValue, shearValue, translateValue);
    });
    connect(controlFrame, &ControlFrame::shearChanged, this, [this](int value) {
        shearValue = value / 10.0;
        updateTransformations(angle, scaleValue, shearValue, translateValue);
    });
    connect(controlFrame, &ControlFrame::translateChanged, this, [this](int value) {
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

void TabHandleIMG::addTextToImage(const QString &text, const QPointF &position)
{
    WaterMark *item = new WaterMark(text);
    item->setFont(QFont(QStringLiteral("Arial"), defaultWatermarkPointSize()));
    item->setPos(position + QPointF(0, -12));
    scene->addItem(item);

    setSceneItemsInteractive(currentToolMode == ToolMode::Select);
    setContentModified(true);
}

void TabHandleIMG::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (view && scene && !scene->items().isEmpty()) {
        view->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

bool TabHandleIMG::shouldHandleToolClick(const QPointF &scenePos) const
{
    if (!scene) {
        return false;
    }

    const QList<QGraphicsItem *> items = scene->items(scenePos);
    for (QGraphicsItem *item : items) {
        if (!item) {
            continue;
        }
        if (item->flags().testFlag(QGraphicsItem::ItemIsMovable)
            || item->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
            return false;
        }
    }

    return true;
}

bool TabHandleIMG::eventFilter(QObject *watched, QEvent *event)
{
    const bool isViewTarget = (watched == view || watched == view->viewport());

    if (isViewTarget && event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPointF scenePos = view->mapToScene(mouseEvent->pos());

        if (drawTool && drawTool->isEnabled()) {
            return drawTool->handleMousePress(mouseEvent->pos(), mouseEvent->button());
        }

        if (watermarkMode && mouseEvent->button() == Qt::LeftButton && shouldHandleToolClick(scenePos)) {
            bool ok = false;
            const QString text = QInputDialog::getText(
                this,
                tr("输入水印"),
                tr("请输入水印内容"),
                QLineEdit::Normal,
                QString(),
                &ok);
            if (ok && !text.isEmpty()) {
                addTextToImage(text, scenePos);
            }
            return true;
        }
    } else if (isViewTarget && event->type() == QEvent::Wheel) {
        auto *wheelEvent = static_cast<QWheelEvent *>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            const int currentValue = controlFrame->getScaleSliderValue();
            const int delta = wheelEvent->angleDelta().y() > 0 ? 5 : -5;
            int newValue = currentValue + delta;
            if (newValue < controlFrame->getScaleSliderMinimum()) {
                newValue = controlFrame->getScaleSliderMinimum();
            }
            if (newValue > controlFrame->getScaleSliderMaximum()) {
                newValue = controlFrame->getScaleSliderMaximum();
            }
            controlFrame->setScaleSliderValue(newValue);
            scaleValue = newValue / 50.0;
            updateTransformations(angle, scaleValue, shearValue, translateValue);
            return true;
        }
    } else if (isViewTarget && event->type() == QEvent::MouseMove) {
        if (drawTool && drawTool->isEnabled()) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            return drawTool->handleMouseMove(mouseEvent->pos(), mouseEvent->buttons());
        }
    } else if (isViewTarget && event->type() == QEvent::MouseButtonRelease) {
        if (drawTool && drawTool->isEnabled()) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            const bool handled = drawTool->handleMouseRelease(mouseEvent->pos(), mouseEvent->button());
            if (handled) {
                setSceneItemsInteractive(currentToolMode == ToolMode::Select);
                setContentModified(true);
            }
            return handled;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void TabHandleIMG::exportImage(const QString &filePath)
{
    const QImage image = renderCompositeImage();
    if (image.isNull()) {
        return;
    }

    if (!image.save(filePath)) {
        QMessageBox::warning(this, tr("保存失败"), tr("无法保存图像到 %1").arg(filePath));
        return;
    }

    setContentModified(false);
}

