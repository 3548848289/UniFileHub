#include "ControlWidArchive.h"
#include <QFontMetrics>

ControlWidArchive::ControlWidArchive(QWidget *parent)
    : QWidget(parent)
{
    // 文件属性在上，按钮与进度在下（整个控件位于可拖动的底部栏内）
    m_statusLabel = new QLabel(this);

    // 短文本按钮：12pt 字体下避免窗口偏窄时文字被裁切，完整语义放 tooltip；
    // 三个按钮统一取"最长文字实际像素宽 + 余量"作为宽度下限
    m_btnExtractHere = new QPushButton(tr("解压全部"), this);
    m_btnExtractHere->setToolTip(tr("将全部条目解压到压缩包所在目录"));
    m_btnExtractAllTo = new QPushButton(tr("解压到…"), this);
    m_btnExtractAllTo->setToolTip(tr("选择目标目录，解压全部条目"));
    m_btnExtractSelectedTo = new QPushButton(tr("解压选中"), this);
    m_btnExtractSelectedTo->setToolTip(tr("将选中的条目解压到指定目录"));
    m_btnExtractSelectedTo->setEnabled(false);

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

    // 解压进度行：进度条 + 取消按钮，忙碌态才显示；
    // 临时提示（"解压成功"等）也共用这一行：空闲时替代进度条显示
    m_progressBar = new QProgressBar(this);
    m_progressBar->setTextVisible(true);
    m_btnCancel = new QPushButton(tr("取消"), this);
    m_flashLabel = new QLabel(this);
    m_flashLabel->hide();

    auto *progressRow = new QHBoxLayout();
    progressRow->addWidget(m_progressBar, 1);
    progressRow->addWidget(m_flashLabel, 1);
    progressRow->addWidget(m_btnCancel);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_statusLabel);
    layout->addLayout(btnRow);
    layout->addLayout(progressRow);

    m_progressBar->hide();
    m_btnCancel->hide();

    connect(m_btnExtractHere, &QPushButton::clicked,
            this, &ControlWidArchive::extractAllHereRequested);
    connect(m_btnExtractAllTo, &QPushButton::clicked,
            this, &ControlWidArchive::extractAllToRequested);
    connect(m_btnExtractSelectedTo, &QPushButton::clicked,
            this, &ControlWidArchive::extractSelectedToRequested);
    connect(m_btnCancel, &QPushButton::clicked,
            this, &ControlWidArchive::cancelRequested);
}

void ControlWidArchive::setStatusText(const QString &text)
{
    m_statusLabel->setText(text);
}

void ControlWidArchive::flashStatus(const QString &text, int msecs)
{
    // 提示显示在解压进度条所在的那一行：空闲时顶替进度条，
    // 忙碌时不打断进度条（提示显示在进度条旁边），msecs 后消失
    ++m_flashSerial;
    const int serial = m_flashSerial;

    if (!m_busy) {
        m_progressBar->hide();
        m_btnCancel->hide();
    }
    m_flashLabel->setText(text);
    m_flashLabel->show();

    QTimer::singleShot(msecs, this, [this, serial] {
        if (serial == m_flashSerial) {
            m_flashLabel->hide();
        }
    });
}

void ControlWidArchive::setSelectionExtractable(bool enabled)
{
    m_btnExtractSelectedTo->setEnabled(enabled && !m_progressBar->isVisible());
}

void ControlWidArchive::setBusy(bool busy, const QString &text)
{
    m_busy = busy;
    // 忙碌状态切换时作废旧提示的隐藏计时并收起提示，避免残留
    ++m_flashSerial;
    m_flashLabel->hide();

    m_progressBar->setVisible(busy);
    m_btnCancel->setVisible(busy);
    // 忙碌期间禁用解压按钮，防止并发解压
    m_btnExtractHere->setEnabled(!busy);
    m_btnExtractAllTo->setEnabled(!busy);
    if (busy) {
        m_progressBar->setRange(0, 0); // 忙碌指示，等待第一次进度
        m_progressBar->setFormat(text.isEmpty() ? tr("正在解压…") : text);
    } else {
        m_progressBar->reset();
    }
}

void ControlWidArchive::showProgress(quint64 done, quint64 total, const QString &currentName)
{
    if (total > 0) {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(static_cast<int>(done * 100 / total));
        const QString nameShort = QFontMetrics(m_progressBar->font())
                                      .elidedText(currentName, Qt::ElideMiddle,
                                                  m_progressBar->width() * 3 / 5);
        m_progressBar->setFormat(tr("正在解压 %1（%p%）").arg(nameShort));
    } else {
        m_progressBar->setRange(0, 0); // 总量未知：忙碌滚动
    }
}
