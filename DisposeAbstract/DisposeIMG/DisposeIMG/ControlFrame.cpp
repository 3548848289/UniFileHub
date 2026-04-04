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

    connect(ui->watermarkButton, &QPushButton::toggled, this, [this](bool enabled) {
        emit watermarkModeToggled(enabled);
        if (enabled) {
            ui->drawButton->setChecked(false);
        }
    });
    connect(ui->drawButton, &QPushButton::toggled, this, [this](bool enabled) {
        emit drawModeToggled(enabled);
        if (enabled) {
            ui->watermarkButton->setChecked(false);
        }
    });
    connect(ui->finishDrawButton, &QPushButton::clicked, this, &ControlFrame::finishDrawingRequested);
    connect(ui->shapeComboBox, &QComboBox::currentTextChanged, this, [this](const QString &) {
        emit drawShapeChanged(currentDrawShapeKey());
    });
}

void ControlFrame::setScaleSliderValue(int value)
{
    if (value < ui->scaleSlider->minimum())
        value = ui->scaleSlider->minimum();
    if (value > ui->scaleSlider->maximum())
        value = ui->scaleSlider->maximum();

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

void ControlFrame::on_exportButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Image"), "", tr("Images (*.png *.jpg *.bmp)"));
    if (!filePath.isEmpty()) {
        emit exportRequested(filePath);
    }
}

QString ControlFrame::currentDrawShapeKey() const
{
    // Use stable keys for code, independent from UI translation.
    const QVariant key = ui->shapeComboBox->currentData();
    if (key.isValid()) {
        return key.toString();
    }
    return QStringLiteral("rect");
}

