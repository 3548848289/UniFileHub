#include "TabHandleVideo.h"
#include <QDir>

TabHandleVideo::TabHandleVideo(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent),
    isPlaying(false)
{
    // 初始化成员变量
    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    videoWidget = new QVideoWidget(this);
    videoWidget->hide();  // 一开始隐藏视频控件

    player->setAudioOutput(audioOutput);
    player->setVideoOutput(videoWidget);
    player->setSource(QUrl::fromLocalFile(filePath));

    // 加一个loading提示
    loadingLabel = new QLabel(tr("加载中..."), this);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setStyleSheet("font-size: 18px; color: gray;");

    // 创建控制窗口
    controlwidVideo = new ControlWidVideo(this);

    // 创建分割器
    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(loadingLabel);
    splitter->addWidget(videoWidget);
    splitter->addWidget(controlwidVideo);
    splitter->setSizes({50, 650, 100});  // 控制比例，loading占用较小空间

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    setLayout(layout);

    // 设置信号槽连接
    connect(player, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            loadingLabel->hide();
            videoWidget->show();
            // 加载完成后自动播放
            player->play();
            isPlaying = true;
            controlwidVideo->setPlayButtonText(tr("暂停"));
        } else if (status == QMediaPlayer::InvalidMedia) {
            loadingLabel->setText(tr("加载失败"));
        }
    });

    // 连接播放状态变化信号
    connect(player, &QMediaPlayer::playbackStateChanged, this, [=](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::PlayingState) {
            isPlaying = true;
            controlwidVideo->setPlayButtonText(tr("暂停"));
        } else if (state == QMediaPlayer::PausedState || state == QMediaPlayer::StoppedState) {
            isPlaying = false;
            controlwidVideo->setPlayButtonText(tr("播放"));
        }
    });

    // 连接控制窗口的信号
    connect(controlwidVideo, &ControlWidVideo::playPauseRequested, this, &TabHandleVideo::onPlayPauseButtonClicked);
    connect(controlwidVideo, &ControlWidVideo::sliderMoved, this, &TabHandleVideo::onSliderMoved);

    // 连接播放器的位置和时长变化信号
    connect(player, &QMediaPlayer::positionChanged, this, &TabHandleVideo::updateSlider);
    connect(player, &QMediaPlayer::durationChanged, this, &TabHandleVideo::updateDuration);
}


void TabHandleVideo::setContent(const QString &text)
{

}

QString TabHandleVideo::getContent() const
{
    return QString();
}

void TabHandleVideo::loadFromFile(const QString &fileName)
{
    setCurrentFilePath(fileName);

    // 停止当前播放
    if (player) {
        player->stop();
    }

    // 重新设置视频源
    player->setSource(QUrl::fromLocalFile(fileName));

    // 显示加载提示
    if (loadingLabel) {
        loadingLabel->setText(tr("加载中..."));
        loadingLabel->show();
    }

    // 隐藏视频窗口，等待加载完成后显示
    if (videoWidget) {
        videoWidget->hide();
    }

    setContentModified(false);
}

void TabHandleVideo::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << getContent();
        file.close();
    } else
    {
        QMessageBox::warning(this, tr("错误"), tr("无法保存文件"));
    }
    setContentModified(false);
}


void TabHandleVideo::loadFromInternet(const QByteArray &content)
{
    // 创建临时文件
    QString tempPath = QDir::tempPath() + "/temp_video_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".mp4";
    QFile tempFile(tempPath);

    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(content);
        tempFile.close();

        // 加载临时文件
        loadFromFile(tempPath);
    }
}

void TabHandleVideo::ControlWidget(bool judge)
{
    isShowControl = judge;
    if (controlwidVideo) {
        controlwidVideo->setVisible(judge);
    }
}

void TabHandleVideo::onPlayPauseButtonClicked()
{
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        player->pause();
        isPlaying = false;
        controlwidVideo->setPlayButtonText(tr("播放"));
    } else {
        player->play();
        isPlaying = true;
        controlwidVideo->setPlayButtonText(tr("暂停"));
    }
}

void TabHandleVideo::onSliderMoved(int position)
{
    player->setPosition(position);
}

void TabHandleVideo::updateSlider(qint64 position)
{
    if (controlwidVideo) {
        controlwidVideo->setSliderPosition(static_cast<int>(position));
        // 同时更新时间显示
        controlwidVideo->updateTimeDisplay(static_cast<int>(position), static_cast<int>(player->duration()));
    }
}

void TabHandleVideo::updateDuration(qint64 duration)
{
    if (controlwidVideo) {
        controlwidVideo->setSliderRange(0, static_cast<int>(duration));
        // 同时更新时间显示
        controlwidVideo->updateTimeDisplay(static_cast<int>(player->position()), static_cast<int>(duration));
    }
}


