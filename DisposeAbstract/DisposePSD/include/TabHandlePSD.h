#ifndef TABHANDLEPSD_H
#define TABHANDLEPSD_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QMouseEvent>
#include <QEvent>
#include <QLineEdit>
#include <QTimer>
#include <QInputDialog>
#include <QtSvgWidgets/QGraphicsSvgItem>
#include "../../main/include/TabAbstract.h"
#include "ControlWidPSD.h"

class TabHandlePSD : public TabAbstract
{
    Q_OBJECT

public:
    explicit TabHandlePSD(const QString& filePath, QWidget *parent = nullptr);

    virtual void setContent(const QString &text) override {}
    virtual QString getContent() const override { return " "; }
    void loadFromFile(const QString &fileName) override;
    void loadFromInternet(const QByteArray &content) override{}    
    void saveToFile(const QString &fileName) override;
    void ControlWidget(bool judge) override;

public slots:
    // 缩放控制通过滑块实现，不再需要单独的按钮控制槽函数

protected:
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);
private:
    void fitItemInView(QGraphicsItem* item);
    void registerPSDHandler();
    void exportToPng();

    QGraphicsView *view;
    QGraphicsScene *scene;
    QGraphicsPixmapItem *pixItem;
    ControlWidPSD *controlFrame;
    qreal scaleValue;
};

#endif // TABHANDLEPSD_H
