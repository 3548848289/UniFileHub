#include "include/ControlFrame.h"
#include "ui/ui_ControlFrame.h"

#include <QPushButton>

ControlFrame::ControlFrame(QWidget *parent)
    : QWidget(parent), ui(new Ui::ControlFrame)
{
    ui->setupUi(this);
    setupConnections();

    const bool isShow = SettingManager::Instance().file_see_img();
    setVisible(isShow);
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
    connect(ui->drawButton, &QPushButton::toggled, this, &ControlFrame::drawPanelToggled);
}

void ControlFrame::setScaleSliderValue(int value)
{
    if (value < ui->scaleSlider->minimum()) {
        value = ui->scaleSlider->minimum();
    }
    if (value > ui->scaleSlider->maximum()) {
        value = ui->scaleSlider->maximum();
    }

    ui->scaleSlider->setValue(value);
}

int ControlFrame::getScaleSliderValue()
{
    return ui->scaleSlider->value();
}

int ControlFrame::getScaleSliderMinimum()
{
    return ui->scaleSlider->minimum();
}

int ControlFrame::getScaleSliderMaximum()
{
    return ui->scaleSlider->maximum();
}
