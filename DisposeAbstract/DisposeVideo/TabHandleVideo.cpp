#include "TabHandleVideo.h"
#include <QDir>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
// #include <QGraphicsVideoItem>
// #include <QVideoSink>
// #include <QMediaPlayer>
// #include <QVideoWidget>
// #include <QAudioOutput>

TabHandleVideo::TabHandleVideo(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent)
// , isPlaying(false)
{
    // 使用占位控件代替视频显示和控制
    QLabel* placeholderLabel = new QLabel(tr("视频功能暂不可用"), this);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setStyleSheet("font-size: 18px; color: gray;");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(placeholderLabel);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    // 初始化成员变量为空，避免访问野指针
    // player = nullptr;
    // audioOutput = nullptr;
    // videoWidget = nullptr;
    // controlwidVideo = nullptr;
}

// 以下方法只保留空实现
void TabHandleVideo::setContent(const QString &text)
{
    Q_UNUSED(text);
}

QString TabHandleVideo::getContent() const
{
    return QString();
}

void TabHandleVideo::loadFromFile(const QString &fileName)
{
    Q_UNUSED(fileName);
}

void TabHandleVideo::saveToFile(const QString &fileName)
{
    Q_UNUSED(fileName);
}

void TabHandleVideo::loadFromInternet(const QByteArray &content)
{
    Q_UNUSED(content);
}

void TabHandleVideo::ControlWidget(bool judge)
{
    Q_UNUSED(judge);
}

void TabHandleVideo::onPlayPauseButtonClicked()
{
}

void TabHandleVideo::onSliderMoved(int position)
{
    Q_UNUSED(position);
}

void TabHandleVideo::updateSlider(qint64 position)
{
    Q_UNUSED(position);
}

void TabHandleVideo::updateDuration(qint64 duration)
{
    Q_UNUSED(duration);
}
