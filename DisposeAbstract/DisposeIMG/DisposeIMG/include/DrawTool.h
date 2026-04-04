#ifndef DRAWTOOL_H
#define DRAWTOOL_H

#include <QObject>
#include <QPen>
#include <QPointer>

class QGraphicsScene;
class QGraphicsView;
class QGraphicsItem;
class QGraphicsRectItem;
class QGraphicsLineItem;

class DrawTool : public QObject
{
    Q_OBJECT

public:
    enum class Shape {
        Rect,
        Line
    };
    Q_ENUM(Shape)

    explicit DrawTool(QGraphicsView *view, QGraphicsScene *scene, QObject *parent = nullptr);

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled; }

    void setShape(Shape shape);
    Shape shape() const { return currentShape; }

    void finish();

    bool handleMousePress(const QPoint &viewPos, Qt::MouseButton button);
    bool handleMouseMove(const QPoint &viewPos, Qt::MouseButtons buttons);
    bool handleMouseRelease(const QPoint &viewPos, Qt::MouseButton button);

private:
    QPointF mapToScene(const QPoint &viewPos) const;
    void beginShape(const QPointF &scenePos);
    void updateShape(const QPointF &scenePos);
    void endShape(const QPointF &scenePos);
    void clearInProgress();

    QPointer<QGraphicsView> view;
    QPointer<QGraphicsScene> scene;

    bool enabled = false;
    bool drawing = false;
    Shape currentShape = Shape::Rect;
    QPointF startScenePos;

    QPen pen;

    QGraphicsRectItem *rectItem = nullptr;
    QGraphicsLineItem *lineItem = nullptr;
};

#endif // DRAWTOOL_H

