#include "TabHandleVideo.h"
#include <QDateTime>
#include <QDir>

TabHandleVideo::TabHandleVideo(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent)
    , isPlaying(false)
{
    player = new QMediaPlayer(this);
#if QT_VERSION_MAJOR >= 6
    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
#endif

    videoWidget = new QVideoWidget(this);
    videoWidget->hide();

    player->setVideoOutput(videoWidget);
#if QT_VERSION_MAJOR >= 6
    player->setSource(QUrl::fromLocalFile(filePath));
#else
    player->setMedia(QMediaContent(QUrl::fromLocalFile(filePath)));
#endif

    loadingLabel = new QLabel(tr("加载中..."), this);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setStyleSheet("font-size: 18px; color: gray;");

    controlwidVideo = new ControlWidVideo(this);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(loadingLabel);
    splitter->addWidget(videoWidget);
    splitter->addWidget(controlwidVideo);
    splitter->setSizes({50, 650, 100});

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    setLayout(layout);

    connect(player, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            loadingLabel->hide();
            videoWidget->show();
            player->play();
            isPlaying = true;
            controlwidVideo->setPlayButtonText(tr("暂停"));
        } else if (status == QMediaPlayer::InvalidMedia) {
            loadingLabel->setText(tr("加载失败"));
        }
    });

#if QT_VERSION_MAJOR >= 6
    connect(player, &QMediaPlayer::playbackStateChanged, this, [=](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::PlayingState) {
            isPlaying = true;
            controlwidVideo->setPlayButtonText(tr("暂停"));
        } else if (state == QMediaPlayer::PausedState || state == QMediaPlayer::StoppedState) {
            isPlaying = false;
            controlwidVideo->setPlayButtonText(tr("播放"));
        }
    });
#else
    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state) {
        if (state == QMediaPlayer::PlayingState) {
            isPlaying = true;
            controlwidVideo->setPlayButtonText(tr("暂停"));
        } else if (state == QMediaPlayer::PausedState || state == QMediaPlayer::StoppedState) {
            isPlaying = false;
            controlwidVideo->setPlayButtonText(tr("播放"));
        }
    });
#endif

    connect(controlwidVideo, &ControlWidVideo::playPauseRequested, this, &TabHandleVideo::onPlayPauseButtonClicked);
    connect(controlwidVideo, &ControlWidVideo::sliderMoved, this, &TabHandleVideo::onSliderMoved);
    connect(player, &QMediaPlayer::positionChanged, this, &TabHandleVideo::updateSlider);
    connect(player, &QMediaPlayer::durationChanged, this, &TabHandleVideo::updateDuration);
}

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
    setCurrentFilePath(fileName);

    if (player) {
        player->stop();
    }

#if QT_VERSION_MAJOR >= 6
    player->setSource(QUrl::fromLocalFile(fileName));
#else
    player->setMedia(QMediaContent(QUrl::fromLocalFile(fileName)));
#endif

    if (loadingLabel) {
        loadingLabel->setText(tr("加载中..."));
        loadingLabel->show();
    }

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
    } else {
        QMessageBox::warning(this, tr("错误"), tr("无法保存文件"));
    }
    setContentModified(false);
}

void TabHandleVideo::loadFromInternet(const QByteArray &content)
{
    const QString tempPath = QDir::tempPath() + "/temp_video_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".mp4";
    QFile tempFile(tempPath);

    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(content);
        tempFile.close();
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
#if QT_VERSION_MAJOR >= 6
    const bool playing = (player->playbackState() == QMediaPlayer::PlayingState);
#else
    const bool playing = (player->state() == QMediaPlayer::PlayingState);
#endif

    if (playing) {
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
        controlwidVideo->updateTimeDisplay(static_cast<int>(position), static_cast<int>(player->duration()));
    }
}

void TabHandleVideo::updateDuration(qint64 duration)
{
    if (controlwidVideo) {
        controlwidVideo->setSliderRange(0, static_cast<int>(duration));
        controlwidVideo->updateTimeDisplay(static_cast<int>(player->position()), static_cast<int>(duration));
    }
}
