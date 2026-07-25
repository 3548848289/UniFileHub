#include "include/NameConflictDialog.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

NameConflictChoice NameConflictDialog::getChoice(QWidget *parent,
                                                 const QString &fileName,
                                                 const QString &detailText,
                                                 const QString &overwriteText,
                                                 const QString &autoRenameText,
                                                 const QString &customNameText)
{
    NameConflictDialog dialog(fileName,
                              detailText,
                              overwriteText,
                              autoRenameText,
                              customNameText,
                              parent);
    if (dialog.exec() != QDialog::Accepted) {
        return {};
    }

    return dialog.choice();
}

NameConflictDialog::NameConflictDialog(const QString &fileName,
                                       const QString &detailText,
                                       const QString &overwriteText,
                                       const QString &autoRenameText,
                                       const QString &customNameText,
                                       QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("同名文件"));
    setModal(true);
    setMinimumWidth(430);
    setProperty("conflictAction", int(NameConflictAction::Cancel));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("已检测到同名文件"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto *detail = new QLabel(detailText, this);
    detail->setWordWrap(true);
    detail->setObjectName(QStringLiteral("detailLabel"));

    auto *nameLabel = new QLabel(QStringLiteral("当前文件名：%1").arg(fileName), this);
    nameLabel->setWordWrap(true);
    nameLabel->setObjectName(QStringLiteral("fileNameLabel"));

    m_customEdit = new QLineEdit(fileName, this);
    m_customEdit->setPlaceholderText(QStringLiteral("新文件名"));
    m_customEdit->setMinimumHeight(36);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    auto *overwriteButton = createActionButton(overwriteText, QStringLiteral("overwriteButton"));
    auto *autoRenameButton = createActionButton(autoRenameText, QStringLiteral("autoRenameButton"));
    auto *customButton = createActionButton(customNameText, QStringLiteral("customButton"));

    buttonLayout->addWidget(overwriteButton);
    buttonLayout->addWidget(autoRenameButton);
    buttonLayout->addWidget(customButton);

    layout->addWidget(title);
    layout->addWidget(detail);
    layout->addWidget(nameLabel);
    layout->addWidget(m_customEdit);
    layout->addLayout(buttonLayout);

    setStyleSheet(QStringLiteral(
        "QDialog { background: #ffffff; border-radius: 10px; }"
        "QLabel { color: #1f2937; font-size: 14px; }"
        "QLabel#detailLabel { color: #6b7280; line-height: 1.4; }"
        "QLabel#fileNameLabel { background: #f8fafc; border: 1px solid #e5e7eb; border-radius: 6px; padding: 9px 10px; color: #374151; }"
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 7px 10px; font-size: 14px; color: #111827; }"
        "QLineEdit:focus { border-color: #3b82f6; }"
        "QPushButton { border: 1px solid #d1d5db; border-radius: 6px; background: #ffffff; color: #374151; padding: 7px 12px; font-weight: 600; }"
        "QPushButton:hover { background: #f3f4f6; }"
        "QPushButton#overwriteButton { background: #fee2e2; border-color: #fecaca; color: #b91c1c; }"
        "QPushButton#overwriteButton:hover { background: #fecaca; }"
        "QPushButton#autoRenameButton { background: #eff6ff; border-color: #bfdbfe; color: #1d4ed8; }"
        "QPushButton#autoRenameButton:hover { background: #dbeafe; }"
        "QPushButton#customButton { background: #ecfdf5; border-color: #bbf7d0; color: #047857; }"
        "QPushButton#customButton:hover { background: #d1fae5; }"
    ));

    connect(overwriteButton, &QPushButton::clicked, this, [this]() {
        setProperty("conflictAction", int(NameConflictAction::Overwrite));
        accept();
    });

    connect(autoRenameButton, &QPushButton::clicked, this, [this]() {
        setProperty("conflictAction", int(NameConflictAction::AutoRename));
        accept();
    });

    connect(customButton, &QPushButton::clicked, this, [this]() {
        const QString customName = m_customEdit ? m_customEdit->text().trimmed() : QString();
        if (customName.isEmpty()) {
            if (m_customEdit) {
                m_customEdit->setFocus();
                m_customEdit->setStyleSheet(QStringLiteral("border: 1px solid #ef4444; border-radius: 6px; padding: 7px 10px;"));
            }
            return;
        }

        setProperty("conflictAction", int(NameConflictAction::CustomName));
        setProperty("customName", customName);
        accept();
    });
}

QPushButton *NameConflictDialog::createActionButton(const QString &text, const QString &objectName)
{
    auto *button = new QPushButton(text, this);
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(36);
    return button;
}

NameConflictChoice NameConflictDialog::choice() const
{
    NameConflictChoice result;
    result.action = NameConflictAction(property("conflictAction").toInt());
    result.customName = property("customName").toString();
    return result;
}
