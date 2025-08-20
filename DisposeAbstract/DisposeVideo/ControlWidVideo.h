#ifndef CONTROLWIDVIDEO_H
#define CONTROLWIDVIDEO_H

#include <QWidget>
namespace Ui {
class ControlWidVideo;
}

class ControlWidVideo : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWidVideo(QWidget *parent = nullptr);
    ~ControlWidVideo();

private:
    Ui::ControlWidVideo *ui;
};

#endif // CONTROLWIDVIDEO_H
