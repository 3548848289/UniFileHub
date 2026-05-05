#include "include/ControlFrame.h"
#include "ui/ui_ControlFrame.h"

#include <limits>
#include <QLineEdit>
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
    connect(ui->rotateSlider, &QSlider::valueChanged, this, [this](int value) {
        ui->rotateValueLabel->setText(QString::number(value) + QChar(0x00B0));
        emit rotateChanged(value);
    });
    connect(ui->scaleEdit, &QLineEdit::editingFinished, this, [this]() {
        setScalePercentValue(getScalePercentValue());
        emit scaleChanged(getScalePercentValue());
    });
    connect(ui->resetButton, &QPushButton::clicked, this, [this]() {
        setRotateSliderValue(0);
        setScalePercentValue(100);
        emit resetRequested();
    });
    connect(ui->drawButton, &QPushButton::toggled, this, &ControlFrame::drawPanelToggled);

    ui->rotateValueLabel->setText(QString::number(ui->rotateSlider->value()) + QChar(0x00B0));
}

void ControlFrame::setScalePercentValue(int value)
{
    if (value < getScalePercentMinimum()) {
        value = getScalePercentMinimum();
    }
    if (value > getScalePercentMaximum()) {
        value = getScalePercentMaximum();
    }

    ui->scaleEdit->setText(QString::number(value) + "%");
}

int ControlFrame::getScalePercentValue() const
{
    QString text = ui->scaleEdit->text().trimmed();
    text.remove('%');

    bool ok = false;
    int value = text.toInt(&ok);
    if (!ok) {
        value = 100;
    }
    if (value < getScalePercentMinimum()) {
        value = getScalePercentMinimum();
    }
    if (value > getScalePercentMaximum()) {
        value = getScalePercentMaximum();
    }

    return value;
}

int ControlFrame::getScalePercentMinimum() const
{
    return 1;
}

int ControlFrame::getScalePercentMaximum() const
{
    return std::numeric_limits<int>::max();
}

void ControlFrame::setRotateSliderValue(int value)
{
    if (value < ui->rotateSlider->minimum()) {
        value = ui->rotateSlider->minimum();
    }
    if (value > ui->rotateSlider->maximum()) {
        value = ui->rotateSlider->maximum();
    }

    ui->rotateSlider->setValue(value);
}

int ControlFrame::getRotateSliderValue()
{
    return ui->rotateSlider->value();
}

int ControlFrame::getRotateSliderMinimum()
{
    return ui->rotateSlider->minimum();
}

int ControlFrame::getRotateSliderMaximum()
{
    return ui->rotateSlider->maximum();
}
