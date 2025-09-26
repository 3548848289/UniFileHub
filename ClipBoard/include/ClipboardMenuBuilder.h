#ifndef CLIPBOARDMENUBUILDER_H
#define CLIPBOARDMENUBUILDER_H

#include <QMenu>
#include <functional>
#include "ClipboardItem/ClipboardItem.h"
#include "ClipboardItem/CliText.h"
#include "ClipboardItem/CliImage.h"
#include "ClipboardItem/CliFile.h"
#include "FiletypeDetector.h"

class ClipboardMenuBuilder {
public:
    ClipboardMenuBuilder() = default;

    // 构建菜单，并绑定各个操作回调
    QMenu* buildMenu(
        ClipboardItem* item,
        std::function<void()> copyCallback,
        std::function<void()> previewCallback,
        std::function<void()> openLocationCallback,
        std::function<void()> deleteCallback,
        std::function<void()> pinCallback
        );
};

#endif // CLIPBOARDMENUBUILDER_H
