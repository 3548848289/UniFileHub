#include "ControlWidArchive.h"
#include <QFontMetrics>

ControlWidArchive::ControlWidArchive(QWidget *parent)
    : QWidget(parent)
{
    // 文件属性在上，按钮在下（整个控件位于可拖动的底部栏内）
    m_statusLabel = new QLabel(this);

    // 短文本按钮：12pt 字体下避免窗口偏窄时文字被裁切，完整语义放 tooltip
    m_btnExtractHere = new QPushButton(tr("解压全部"), this);
    m_btnExtractHere->setToolTip(tr("将全部条目解压到压缩包所在目录"));
    m_btnExtractAllTo = new QPushButton(tr("解压到…"), this);
    m_btnExtractAllTo->setToolTip(tr("选择目标目录，解压全部条目"));
    m_btnExtractSelectedTo = new QPushButton(tr("解压选中"), this);
    m_btnExtractSelectedTo->setToolTip(tr("将选中的条目解压到指定目录"));
    m_btnExtractSelectedTo->setEnabled(false);

    // 三个按钮统一取"最长文字实际像素宽 + 余量"作为宽度下限，
    // 布局再挤也不会裁掉按钮文字（宽度不足时优先压缩的是右侧空白）
    const QFontMetrics fm(m_btnExtractHere->font());
    int minW = 0;
    for (QPushButton *b : { m_btnExtractHere, m_btnExtractAllTo, m_btnExtractSelectedTo })
        minW = qMax(minW, fm.horizontalAdvance(b->text()));
    minW += 56; // 按钮左右内边距与边框余量
    for (QPushButton *b : { m_btnExtractHere, m_btnExtractAllTo, m_btnExtractSelectedTo })
        b->setMinimumWidth(minW);

    auto *btnRow = new QHBoxLayout();
    btnRow->addWidget(m_btnExtractHere);
    btnRow->addWidget(m_btnExtractAllTo);
    btnRow->addWidget(m_btnExtractSelectedTo);
    btnRow->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_statusLabel);
    layout->addLayout(btnRow);

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
