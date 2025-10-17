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

public slots:
    // 设置滑块范围
    void setSliderRange(int min, int max);
    // 更新滑块位置
    void setSliderPosition(int position);
    // 更新播放/暂停按钮文本
    void setPlayButtonText(const QString &text);

private slots:
    void on_pushButton_clicked();
    void on_horizontalSlider_sliderMoved(int position);
    void on_horizontalSlider_sliderPressed();
    void on_horizontalSlider_sliderReleased();

signals:
    void playPauseRequested();
    void sliderMoved(int position);
    void sliderPressed();
    void sliderReleased();

private:
    Ui::ControlWidVideo *ui;
    bool isSliding; // 标记是否正在拖动滑块
};

#endif // CONTROLWIDVIDEO_H
