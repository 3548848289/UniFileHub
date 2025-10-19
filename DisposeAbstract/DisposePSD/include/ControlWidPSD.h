#ifndef CONTROLWIDPSD_H
#define CONTROLWIDPSD_H

#include <QWidget>
#include <QVBoxLayout>
#include <QSlider>
#include <QLabel>

namespace Ui {
class ControlWidPSD;
}

class ControlWidPSD : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWidPSD(QWidget *parent = nullptr);
    ~ControlWidPSD();

private:
    Ui::ControlWidPSD *ui;

signals:
    void zoomValueChanged(int value);

private slots:
    void on_zoomSlider_valueChanged(int value);
};

#endif // CONTROLWIDPSD_H