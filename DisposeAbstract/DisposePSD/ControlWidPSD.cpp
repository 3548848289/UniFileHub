#include "include/ControlWidPSD.h"
#include "ui/ui_ControlWidPSD.h"

ControlWidPSD::ControlWidPSD(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlWidPSD)
{
    ui->setupUi(this);
}

ControlWidPSD::~ControlWidPSD()
{
    delete ui;
}

void ControlWidPSD::on_zoomSlider_valueChanged(int value)
{
    // 更新显示的缩放百分比
    ui->zoomValueLabel->setText(QString::number(value) + QStringLiteral("%"));
    // 发送缩放值变化信号
    emit zoomValueChanged(value);
}
