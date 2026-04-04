#ifndef CONTROLFRAME_H
#define CONTROLFRAME_H

#include <QSlider>
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

    void setScaleSliderValue(int value);
    int getScaleSliderValue();
    int getScaleSliderMinimum();
    int getScaleSliderMaximum();

signals:
    void rotateChanged(int value);
    void scaleChanged(int value);
    void shearChanged(int value);
    void translateChanged(int value);
    void drawPanelToggled(bool visible);

private:
    void setupConnections();

    Ui::ControlFrame *ui;
};

#endif // CONTROLFRAME_H
