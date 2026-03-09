#include "ControlWidTXT.h"
#include "ui_ControlWidTXT.h"
#include <QMenu>
#include <QAction>

ControlWidTXT::ControlWidTXT(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlWidTXT),
    m_currentCodecName("UTF-8"),
    m_tabIndent(4),
    m_fontSize(12)
{
    ui->setupUi(this);

    // 更新显示标签
    updateDisplayLabels();

    // 添加提示语
    ui->tabIndentDisplayLabel->setToolTip("点击修改Tab缩进数");
    ui->encodingDisplayLabel->setToolTip("点击修改编码方式");
    ui->fontSizeDisplayLabel->setToolTip("点击修改字体大小");
    ui->statsDisplayLabel->setToolTip("显示当前文本的行数和字符数");
    ui->textExcerptEdit->setToolTip("显示当前文本的摘录");

    // 设置文本摘录编辑框的高度
    ui->textExcerptEdit->setMaximumHeight(100);

    // 连接信号
    connect(ui->tabIndentDisplayLabel, &ClickableLabel::clicked, this, &ControlWidTXT::onTabIndentLabelClicked);
    connect(ui->encodingDisplayLabel, &ClickableLabel::clicked, this, &ControlWidTXT::onEncodingLabelClicked);
    connect(ui->fontSizeDisplayLabel, &ClickableLabel::clicked, this, &ControlWidTXT::onFontSizeLabelClicked);
    connect(ui->mdPreviewCheckBox, &QCheckBox::toggled, this, &ControlWidTXT::mdPreviewToggled);
    connect(ui->htmlPreviewCheckBox, &QCheckBox::toggled, this, &ControlWidTXT::htmlPreviewToggled);
}

ControlWidTXT::~ControlWidTXT()
{
    delete ui;
}

QString ControlWidTXT::getCurrentCodecName() const
{
    return m_currentCodecName;
}

void ControlWidTXT::setCurrentCodecName(const QString &codecName)
{
    m_currentCodecName = codecName;
    updateDisplayLabels();
}

void ControlWidTXT::updateTextStatistics(int lineCount, int charCount)
{
    ui->statsDisplayLabel->setText(QString("%1 行, %2 字").arg(lineCount).arg(charCount));
}

void ControlWidTXT::updateTextExcerpt(const QString &excerpt)
{
    ui->textExcerptEdit->setPlainText(excerpt);
}

void ControlWidTXT::updateDisplayLabels()
{
    ui->tabIndentDisplayLabel->setText(QString("Tab: %1").arg(m_tabIndent));
    ui->encodingDisplayLabel->setText(QString("编码: %1").arg(m_currentCodecName));
    ui->fontSizeDisplayLabel->setText(QString("字体: %1").arg(m_fontSize));
}

void ControlWidTXT::onTabIndentLabelClicked()
{
    QMenu menu(this);
    for (int i = 1; i <= 8; i++) {
        QAction *action = menu.addAction(QString("%1").arg(i));
        action->setCheckable(true);
        action->setChecked(i == m_tabIndent);
    }

    QAction *selectedAction = menu.exec(ui->tabIndentDisplayLabel->mapToGlobal(QPoint(0, ui->tabIndentDisplayLabel->height())));
    if (selectedAction) {
        int newIndent = selectedAction->text().toInt();
        if (newIndent != m_tabIndent) {
            m_tabIndent = newIndent;
            updateDisplayLabels();
            emit tabIndentChanged(newIndent);
        }
    }
}

void ControlWidTXT::onEncodingLabelClicked()
{
    QMenu menu(this);
    QStringList codecs = {"UTF-8", "GBK", "Shift-JIS", "EUC-KR", "ISO-8859-1"};
    for (const QString &codec : codecs) {
        QAction *action = menu.addAction(codec);
        action->setCheckable(true);
        action->setChecked(codec == m_currentCodecName);
    }

    QAction *selectedAction = menu.exec(ui->encodingDisplayLabel->mapToGlobal(QPoint(0, ui->encodingDisplayLabel->height())));
    if (selectedAction) {
        QString newCodec = selectedAction->text();
        if (newCodec != m_currentCodecName) {
            m_currentCodecName = newCodec;
            updateDisplayLabels();
            emit encodingChanged(newCodec);
        }
    }
}

void ControlWidTXT::onFontSizeLabelClicked()
{
    QMenu menu(this);
    for (int size = 8; size <= 24; size++) {
        QAction *action = menu.addAction(QString("%1").arg(size));
        action->setCheckable(true);
        action->setChecked(size == m_fontSize);
    }

    QAction *selectedAction = menu.exec(ui->fontSizeDisplayLabel->mapToGlobal(QPoint(0, ui->fontSizeDisplayLabel->height())));
    if (selectedAction) {
        int newSize = selectedAction->text().toInt();
        if (newSize != m_fontSize) {
            m_fontSize = newSize;
            updateDisplayLabels();
            emit fontSizeChanged(newSize);
        }
    }
}
