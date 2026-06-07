#ifndef CLIPBOARDMENUBUILDER_H
#define CLIPBOARDMENUBUILDER_H

#include <QMenu>
#include <functional>
#include "ClipboardItem/ClipboardItem.h"
#include "ClipboardItem/CliFile.h"
#include "ClipboardItem/CliImage.h"
#include "ClipboardItem/CliText.h"
#include "FileTypeDetector.h"

class ClipboardMenuBuilder {
public:
    ClipboardMenuBuilder() = default;

    QMenu* buildMenu(
        ClipboardItem* item,
        std::function<void()> copyCallback,
        std::function<void()> previewCallback,
        std::function<void()> openLocationCallback,
        std::function<void()> deleteCallback,
        std::function<void()> pinCallback,
        std::function<void()> cloudSyncCallback,
        std::function<void()> cloudUnsyncCallback
    );
};

#endif // CLIPBOARDMENUBUILDER_H
