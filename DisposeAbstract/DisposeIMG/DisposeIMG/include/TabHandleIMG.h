#ifndef TABHANDLEIMG_H
#define TABHANDLEIMG_H

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
#include "ControlFrame.h"
#include "PixItem.h"
#include "DrawTool.h"

class QZipReader;

class TabHandleIMG : public TabAbstract
{
    Q_OBJECT

public:
    explicit TabHandleIMG(const QString& filePath, QWidget *parent = nullptr);

    virtual void setContent(const QString &text) override {}
    virtual QString getContent() const override {  return " "; }
    void loadFromFile(const QString &fileName) override;
    void loadFromInternet(const QByteArray &content) override{ }
    void saveToFile(const QString &fileName);
    void ControlWidget(bool judge){
        qDebug() << "TabHandleCSV: Showing control frame!";
    }

    void test();

public slots:
    void showControlFrame(ControlFrame *controlFrame);
    void onTextAdded(const QString &text, const QPointF &position);

protected:
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);
private:
    void updateTransformations(int angle, qreal scale, qreal shear, qreal translate);
    void addTextToImage(const QString &text, const QPointF &position);
    void exportImage(const QString &filePath);
    
    // XMind 支持：从 XMind 文件中提取缩略图
    bool loadXmindThumbnail(const QString &fileName);
    QByteArray extractFileFromZip(const QString &zipPath, const QString &fileNameInZip);


    QGraphicsView *view;
    QGraphicsScene *scene;
    PixItem *pixItem;
    // QGraphicsSvgItem * svgItem;
    ControlFrame *controlFrame;
    DrawTool *drawTool = nullptr;
    bool watermarkMode = false;
    int angle;
    qreal scaleValue;
    qreal shearValue;
    qreal translateValue;
    QGraphicsTextItem *textItem;
    
    // XMind 相关
    bool isXmindFile = false;
};

#endif // TABHANDLEIMG_H
