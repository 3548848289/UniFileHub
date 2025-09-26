#include "include/ClipboardMonitor.h"

ClipboardMonitor::ClipboardMonitor(QObject* parent)
    : QObject(parent), m_clipboard(QGuiApplication::clipboard()) {
    connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardMonitor::onClipboardChanged);
}

void ClipboardMonitor::onClipboardChanged() {
    const QMimeData* mimeData = m_clipboard->mimeData();
    if (!mimeData) return;

    std::unique_ptr<ClipboardItem> newItem =
        ClipboardItemFactory::createFromMimeData(mimeData);

    if (newItem) {
        emit newItemCaptured(newItem.release()); // release 所有权，交给接收方
    }

}
