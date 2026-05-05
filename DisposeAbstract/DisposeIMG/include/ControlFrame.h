#ifndef CONTROLFRAME_H
#define CONTROLFRAME_H

#include <QWidget>

#include "../../Setting/include/SettingManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ControlFrame; }
QT_END_NAMESPACE

class ControlFrame : public QWidget
{
    Q_OBJECT

public:
    explicit ControlFrame(QWidget *parent = nullptr);
    ~ControlFrame();

    void setScalePercentValue(int value);
    int getScalePercentValue() const;
    int getScalePercentMinimum() const;
    int getScalePercentMaximum() const;
    void setRotateSliderValue(int value);
    int getRotateSliderValue();
    int getRotateSliderMinimum();
    int getRotateSliderMaximum();

signals:
    void rotateChanged(int value);
    void scaleChanged(int value);
    void resetRequested();
    void drawPanelToggled(bool visible);

private:
    void setupConnections();

    Ui::ControlFrame *ui;
};

#endif // CONTROLFRAME_H
