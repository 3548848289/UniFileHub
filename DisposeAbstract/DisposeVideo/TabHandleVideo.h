#ifndef TABHANDLEVIDEO_H
#define TABHANDLEVIDEO_H

#include "../../main/include/TabAbstract.h"
#include <QFile>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsVideoItem>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QtGlobal>
#if QT_VERSION_MAJOR >= 6
#include <QAudioOutput>
#else
#include <QMediaContent>
#endif
#include "ControlWidVideo.h"

class TabHandleVideo : public TabAbstract
{
    Q_OBJECT

public:
    explicit TabHandleVideo(const QString& filePath, QWidget *parent = nullptr);

    void setContent(const QString &text) override;
    QString getContent() const override;
    void loadFromFile(const QString &fileName) override;
    void loadFromInternet(const QByteArray &content) override;
    void saveToFile(const QString &fileName) override;
    void ControlWidget(bool judge) override;

private slots:
    void onPlayPauseButtonClicked();
    void onSliderMoved(int position);
    void updateSlider(qint64 position);
    void updateDuration(qint64 duration);

private:
    QSplitter* splitter;
    ControlWidVideo * controlwidVideo;
    QMediaPlayer *player;
    QVideoWidget *videoWidget;
#if QT_VERSION_MAJOR >= 6
    QAudioOutput *audioOutput;
#endif
    QLabel *loadingLabel;
    bool isPlaying;

};

#endif // TABHANDLEVIDEO_H
