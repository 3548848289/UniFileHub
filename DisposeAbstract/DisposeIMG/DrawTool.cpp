#include "include/DrawTool.h"

#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>

namespace {
QRectF normalizedRect(const QPointF &a, const QPointF &b)
{
    const qreal left = qMin(a.x(), b.x());
    const qreal right = qMax(a.x(), b.x());
    const qreal top = qMin(a.y(), b.y());
    const qreal bottom = qMax(a.y(), b.y());
    return QRectF(QPointF(left, top), QPointF(right, bottom));
}
}

DrawTool::DrawTool(QGraphicsView *view, QGraphicsScene *scene, QObject *parent)
    : QObject(parent), view(view), scene(scene)
{
    pen.setWidth(2);
    pen.setColor(QColor(220, 38, 38));
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

bool DrawTool::shouldStartDrawing(const QPointF &scenePos) const
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

void DrawTool::beginShape(const QPointF &scenePos)
{
    if (!scene) {
        return;
    }

    startScenePos = scenePos;
    drawing = true;

    clearInProgress();

    switch (currentShape) {
    case Shape::Rect:
        rectItem = scene->addRect(QRectF(scenePos, scenePos), pen, Qt::NoBrush);
        setupFinishedItem(rectItem);
        break;
    case Shape::Line:
        lineItem = scene->addLine(QLineF(scenePos, scenePos), pen);
        setupFinishedItem(lineItem);
        break;
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

    rectItem = nullptr;
    lineItem = nullptr;
}

void DrawTool::clearInProgress()
{
    if (!scene) {
        return;
    }

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

void DrawTool::setupFinishedItem(QGraphicsItem *item)
{
    if (!item) {
        return;
    }

    item->setZValue(1000);
    item->setFlags(QGraphicsItem::ItemIsMovable
                   | QGraphicsItem::ItemIsSelectable
                   | QGraphicsItem::ItemSendsGeometryChanges);
}

bool DrawTool::handleMousePress(const QPoint &viewPos, Qt::MouseButton button)
{
    if (!enabled || button != Qt::LeftButton) {
        return false;
    }

    const QPointF scenePos = mapToScene(viewPos);
    if (!shouldStartDrawing(scenePos)) {
        return false;
    }

    beginShape(scenePos);
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
        return false;
    }

    endShape(mapToScene(viewPos));
    return true;
}
