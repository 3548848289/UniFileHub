#include "include/DrawToolPanel.h"

#include "ui/ui_DrawToolPanel.h"

#include <QButtonGroup>
#include <QPushButton>

DrawToolPanel::DrawToolPanel(QWidget *parent)
    : QWidget(parent), ui(new Ui::DrawToolPanel)
{
    ui->setupUi(this);

    toolButtonGroup = new QButtonGroup(this);
    toolButtonGroup->setExclusive(true);
    toolButtonGroup->addButton(ui->rectButton);
    toolButtonGroup->addButton(ui->lineButton);
    toolButtonGroup->addButton(ui->watermarkButton);

    setupConnections();
}

DrawToolPanel::~DrawToolPanel()
{
    delete ui;
}

void DrawToolPanel::setupConnections()
{
    connect(ui->rectButton, &QPushButton::clicked, this, [this]() {
        emit toolSelected(QStringLiteral("rect"));
    });
    connect(ui->lineButton, &QPushButton::clicked, this, [this]() {
        emit toolSelected(QStringLiteral("line"));
    });
    connect(ui->watermarkButton, &QPushButton::clicked, this, [this]() {
        emit toolSelected(QStringLiteral("watermark"));
    });
    connect(ui->exportButton, &QPushButton::clicked, this, &DrawToolPanel::exportRequested);
}

void DrawToolPanel::clearSelection()
{
    if (!toolButtonGroup) {
        return;
    }

    toolButtonGroup->setExclusive(false);
    ui->rectButton->setChecked(false);
    ui->lineButton->setChecked(false);
    ui->watermarkButton->setChecked(false);
    toolButtonGroup->setExclusive(true);
}

QString DrawToolPanel::currentToolKey() const
{
    if (ui->rectButton->isChecked()) {
        return QStringLiteral("rect");
    }
    if (ui->lineButton->isChecked()) {
        return QStringLiteral("line");
    }
    if (ui->watermarkButton->isChecked()) {
        return QStringLiteral("watermark");
    }
    return QString();
}
