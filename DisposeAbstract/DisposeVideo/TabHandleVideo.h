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
#include <QVideoSink>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
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
    void ControlWidget(bool judge){
        isShowControl = judge;
    }

private:
    QSplitter* splitter;

    ControlWidVideo * controlwidVideo;

};

#endif // TABHANDLEVIDEO_H
