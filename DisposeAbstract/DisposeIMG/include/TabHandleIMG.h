#ifndef TABHANDLEIMG_H
#define TABHANDLEIMG_H

#include <QDebug>
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

#include "../../main/include/TabAbstract.h"
#include "ControlFrame.h"
#include "DrawTool.h"
#include "DrawToolPanel.h"
#include "PixItem.h"

class QGraphicsItem;
class QZipReader;

class TabHandleIMG : public TabAbstract
{
    Q_OBJECT

public:
    explicit TabHandleIMG(const QString &filePath, QWidget *parent = nullptr);

    void setContent(const QString &text) override { Q_UNUSED(text); }
    QString getContent() const override { return QStringLiteral(" "); }
    void loadFromFile(const QString &fileName) override;
    void loadFromInternet(const QByteArray &content) override { Q_UNUSED(content); }
    void saveToFile(const QString &fileName);
    void ControlWidget(bool judge)
    {
        Q_UNUSED(judge);
        qDebug() << "TabHandleIMG: Showing control frame!";
    }

    void test();

public slots:
    void showControlFrame(ControlFrame *controlFrame);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class ToolMode {
        None,
        Select,
        Rect,
        Line,
        Watermark
    };

    void setupEditorLayout();
    void setupToolConnections();
    void setToolPanelVisible(bool visible);
    void applyToolSelection(const QString &toolKey);
    void updateToolMode(ToolMode mode);
    void setSceneItemsInteractive(bool interactive);
    void requestExport();
    void copyToClipboard();
    void clearOverlayItems();
    QImage renderCompositeImage() const;
    int defaultWatermarkPointSize() const;
    bool shouldHandleToolClick(const QPointF &scenePos) const;
    void updateTransformations(int angle, qreal scale);
    void addTextToImage(const QString &text, const QPointF &position);
    void exportImage(const QString &filePath);

    bool loadXmindThumbnail(const QString &fileName);
    QByteArray extractFileFromZip(const QString &zipPath, const QString &fileNameInZip);

    QGraphicsView *view = nullptr;
    QGraphicsScene *scene = nullptr;
    QGraphicsItem *contentItem = nullptr;
    PixItem *pixItem = nullptr;
    QWidget *editorArea = nullptr;
    ControlFrame *controlFrame = nullptr;
    DrawToolPanel *drawToolPanel = nullptr;
    DrawTool *drawTool = nullptr;
    bool watermarkMode = false;
    ToolMode currentToolMode = ToolMode::None;
    int angle = 0;
    qreal scaleValue = 1.0;
    bool isXmindFile = false;
};

#endif // TABHANDLEIMG_H
