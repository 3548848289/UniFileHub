#include "ControlWidArchive.h"

ControlWidArchive::ControlWidArchive(QWidget *parent)
    : QWidget(parent)
{
    m_btnExtractHere = new QPushButton(tr("解压全部到此目录"), this);
    m_btnExtractAllTo = new QPushButton(tr("解压全部到…"), this);
    m_btnExtractSelectedTo = new QPushButton(tr("解压选中到…"), this);
    m_btnExtractSelectedTo->setEnabled(false);

    auto *btnRow = new QHBoxLayout();
    btnRow->addWidget(m_btnExtractHere);
    btnRow->addWidget(m_btnExtractAllTo);
    btnRow->addWidget(m_btnExtractSelectedTo);
    btnRow->addStretch();

    m_statusLabel = new QLabel(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(btnRow);
    layout->addWidget(m_statusLabel);
    setLayout(layout);

    connect(m_btnExtractHere, &QPushButton::clicked,
            this, &ControlWidArchive::extractAllHereRequested);
    connect(m_btnExtractAllTo, &QPushButton::clicked,
            this, &ControlWidArchive::extractAllToRequested);
    connect(m_btnExtractSelectedTo, &QPushButton::clicked,
            this, &ControlWidArchive::extractSelectedToRequested);
}

void ControlWidArchive::setStatusText(const QString &text)
{
    m_statusLabel->setText(text);
}

void ControlWidArchive::setSelectionExtractable(bool enabled)
{
    m_btnExtractSelectedTo->setEnabled(enabled);
}
