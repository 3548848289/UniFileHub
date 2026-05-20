#include "include/ControlWidPSD.h"
#include "ui/ui_ControlWidPSD.h"

ControlWidPSD::ControlWidPSD(QWidget *parent)
    : QWidget(parent),
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
    ui->zoomValueLabel->setText(QString::number(value) + QStringLiteral("%"));
    emit zoomValueChanged(value);
}

void ControlWidPSD::on_exportButton_clicked()
{
    emit exportRequested();
}
