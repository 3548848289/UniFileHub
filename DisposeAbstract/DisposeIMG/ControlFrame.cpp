#include "include/ControlFrame.h"
#include "ui/ui_ControlFrame.h"

ControlFrame::ControlFrame(QWidget *parent)
    : QWidget(parent), ui(new Ui::ControlFrame)
{
    ui->setupUi(this);
    setupConnections();
    bool isShow = SettingManager::Instance().file_see_img();
    this->setVisible(isShow);
}

ControlFrame::~ControlFrame()
{
    delete ui;
}

void ControlFrame::setupConnections()
{
    connect(ui->rotateSlider, &QSlider::valueChanged, this, &ControlFrame::rotateChanged);
    connect(ui->scaleSlider, &QSlider::valueChanged, this, &ControlFrame::scaleChanged);
    connect(ui->shearSlider, &QSlider::valueChanged, this, &ControlFrame::shearChanged);
    connect(ui->translateSlider, &QSlider::valueChanged, this, &ControlFrame::translateChanged);
}

void ControlFrame::setScaleSliderValue(int value)
{
    if (value < ui->scaleSlider->minimum())
        value = ui->scaleSlider->minimum();
    if (value > ui->scaleSlider->maximum())
        value = ui->scaleSlider->maximum();

    ui->scaleSlider->setValue(value);
}

void ControlFrame::on_exportButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Image"), "", tr("Images (*.png *.jpg *.bmp)"));
    if (!filePath.isEmpty()) {
        emit exportRequested(filePath);
    }
}

