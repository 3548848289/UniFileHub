#ifndef CLIPBOARDMONITOR_H
#define CLIPBOARDMONITOR_H

#include <QObject>
#include <QClipboard>
#include <QGuiApplication>
#include <memory>
#include "ClipboardItem/ClipboardItem.h"
#include "ClipboardItemFactory.h"

// 专门负责监听系统剪贴板，并发出信号
class ClipboardMonitor : public QObject {
    Q_OBJECT
public:
    explicit ClipboardMonitor(QObject* parent = nullptr);

signals:
    void newItemCaptured(ClipboardItem* item);

private slots:
    void onClipboardChanged();

private:
    QClipboard* m_clipboard;
};

#endif // CLIPBOARDMONITOR_H
