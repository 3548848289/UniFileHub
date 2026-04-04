#include "include/DrawTool.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>

namespace {
QRectF normalizedRect(const QPointF &a, const QPointF &b)
{
    const qreal left = qMin(a.x(), b.x());
    const qreal right = qMax(a.x(), b.x());
    const qreal top = qMin(a.y(), b.y());
    const qreal bottom = qMax(a.y(), b.y());
    return QRectF(QPointF(left, top), QPointF(right, bottom));
}
} // namespace

DrawTool::DrawTool(QGraphicsView *view, QGraphicsScene *scene, QObject *parent)
    : QObject(parent), view(view), scene(scene)
{
    pen.setWidth(2);
    pen.setColor(QColor(220, 38, 38)); // red-ish, visible on images
    pen.setCosmetic(true);
}

void DrawTool::setEnabled(bool newEnabled)
{
    enabled = newEnabled;
    if (!enabled) {
        finish();
    }
}

void DrawTool::setShape(Shape shape)
{
    currentShape = shape;
}

void DrawTool::finish()
{
    drawing = false;
    clearInProgress();
}

QPointF DrawTool::mapToScene(const QPoint &viewPos) const
{
    if (!view) {
        return QPointF();
    }
    return view->mapToScene(viewPos);
}

void DrawTool::beginShape(const QPointF &scenePos)
{
    if (!scene) {
        return;
    }

    startScenePos = scenePos;
    drawing = true;

    clearInProgress();

    switch (currentShape) {
    case Shape::Rect: {
        rectItem = scene->addRect(QRectF(scenePos, scenePos), pen, Qt::NoBrush);
        rectItem->setZValue(1000);
        break;
    }
    case Shape::Line: {
        lineItem = scene->addLine(QLineF(scenePos, scenePos), pen);
        lineItem->setZValue(1000);
        break;
    }
    }
}

void DrawTool::updateShape(const QPointF &scenePos)
{
    if (!drawing) {
        return;
    }
    switch (currentShape) {
    case Shape::Rect:
        if (rectItem) {
            rectItem->setRect(normalizedRect(startScenePos, scenePos));
        }
        break;
    case Shape::Line:
        if (lineItem) {
            lineItem->setLine(QLineF(startScenePos, scenePos));
        }
        break;
    }
}

void DrawTool::endShape(const QPointF &scenePos)
{
    if (!drawing) {
        return;
    }
    updateShape(scenePos);
    drawing = false;

    // Keep the finished item in scene; only clear the "in progress" pointers.
    rectItem = nullptr;
    lineItem = nullptr;
}

void DrawTool::clearInProgress()
{
    // If there is an in-progress item, remove it (only happens when switching modes mid-draw).
    if (scene) {
        if (rectItem) {
            scene->removeItem(rectItem);
            delete rectItem;
            rectItem = nullptr;
        }
        if (lineItem) {
            scene->removeItem(lineItem);
            delete lineItem;
            lineItem = nullptr;
        }
    }
}

bool DrawTool::handleMousePress(const QPoint &viewPos, Qt::MouseButton button)
{
    if (!enabled || button != Qt::LeftButton) {
        return false;
    }
    beginShape(mapToScene(viewPos));
    return true;
}

bool DrawTool::handleMouseMove(const QPoint &viewPos, Qt::MouseButtons buttons)
{
    if (!enabled || !drawing) {
        return false;
    }
    if (!(buttons & Qt::LeftButton)) {
        return false;
    }
    updateShape(mapToScene(viewPos));
    return true;
}

bool DrawTool::handleMouseRelease(const QPoint &viewPos, Qt::MouseButton button)
{
    if (!enabled || button != Qt::LeftButton) {
        return false;
    }
    if (!drawing) {
        return true;
    }
    endShape(mapToScene(viewPos));
    return true;
}

