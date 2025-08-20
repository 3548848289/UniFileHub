#include "TabHandleVideo.h"

TabHandleVideo::TabHandleVideo(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent)
{
    QVideoWidget *videoWidget = new QVideoWidget(this);
    videoWidget->hide();  // 一开始隐藏视频控件

    QMediaPlayer *player = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);

    player->setAudioOutput(audioOutput);
    player->setVideoOutput(videoWidget);
    player->setSource(QUrl::fromLocalFile(filePath));

    // 加一个loading提示
    QLabel *loadingLabel = new QLabel(tr("加载中..."), this);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setStyleSheet("font-size: 18px; color: gray;");

    connect(player, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            loadingLabel->hide();
            videoWidget->show();
            player->play();
        } else if (status == QMediaPlayer::InvalidMedia) {
            loadingLabel->setText(tr("加载失败"));
        }
    });

    controlwidVideo = new ControlWidVideo(this);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(loadingLabel);
    splitter->addWidget(videoWidget);
    splitter->addWidget(controlwidVideo);
    splitter->setSizes({50, 650, 100});  // 控制比例，loading占用较小空间

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    setLayout(layout);
}


void TabHandleVideo::setContent(const QString &text)
{

}

QString TabHandleVideo::getContent() const
{
}

void TabHandleVideo::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        setContent(in.readAll());
        file.close();
    } else {
        QMessageBox::warning(this, tr("错误"), tr("无法打开文件"));
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

}


