#include "include/ClipboardMenuBuilder.h"
#include <QAction>

QMenu* ClipboardMenuBuilder::buildMenu(
    ClipboardItem* item,
    std::function<void()> copyCallback,
    std::function<void()> openCallback,
    std::function<void()> previewCallback,
    std::function<void()> openLocationCallback,
    std::function<void()> deleteCallback,
    std::function<void()> pinCallback,
    std::function<void()> cloudSyncCallback,
    std::function<void()> cloudUnsyncCallback)
{
    if (!item) {
        return nullptr;
    }

    QMenu* menu = new QMenu;

    QAction* copyAction = menu->addAction(QStringLiteral("复制到剪切板"));
    QObject::connect(copyAction, &QAction::triggered, [copyCallback]{ if (copyCallback) copyCallback(); });

    // "打开":图片记录用图片编辑器打开,文本记录用文本编辑器打开(由 view 分发)
    bool canOpen = false;
    if (item->type() == ClipboardItemType::Image) {
        canOpen = true;
    } else if (item->type() == ClipboardItemType::File) {
        auto* fileItem = static_cast<CliFile*>(item);
        canOpen = fileItem->isImageFile();
    } else if (item->type() == ClipboardItemType::Text) {
        canOpen = true;
    }

    if (canOpen && openCallback) {
        QAction* openAction = menu->addAction(QStringLiteral("打开"));
        openAction->setToolTip(QStringLiteral("用对应的编辑器打开,可编辑、导出"));
        QObject::connect(openAction, &QAction::triggered, [openCallback]{ if (openCallback) openCallback(); });
    }

    bool canPreview = false;
    if (item->type() == ClipboardItemType::Image) {
        canPreview = true;
    } else if (item->type() == ClipboardItemType::File) {
        auto* fileItem = static_cast<CliFile*>(item);
        canPreview = fileItem->isImageFile();
    } else if (item->type() == ClipboardItemType::Text) {
        auto* textItem = static_cast<CliText*>(item);
        canPreview = FileTypeDetector::isImageFile(textItem->text());
    }

    if (canPreview && previewCallback) {
        QAction* previewAction = menu->addAction(QStringLiteral("预览图片"));
        QObject::connect(previewAction, &QAction::triggered, [previewCallback]{ previewCallback(); });
    }

    if (item->type() == ClipboardItemType::File && openLocationCallback) {
        QAction* openLocationAction = menu->addAction(QStringLiteral("打开文件位置"));
        QObject::connect(openLocationAction, &QAction::triggered, [openLocationCallback]{ openLocationCallback(); });
    }

    const bool canCloudSync = (item->type() == ClipboardItemType::Text
                               || item->type() == ClipboardItemType::Image);
    if (canCloudSync && !item->isCloudItem() && cloudSyncCallback) {
        QAction* cloudAction = menu->addAction(QStringLiteral("云端同步"));
        QObject::connect(cloudAction, &QAction::triggered, [cloudSyncCallback]{ cloudSyncCallback(); });
    }

    if (item->isCloudItem() && cloudUnsyncCallback) {
        QAction* cloudDeleteAction = menu->addAction(QStringLiteral("取消云端同步"));
        QObject::connect(cloudDeleteAction, &QAction::triggered, [cloudUnsyncCallback]{ cloudUnsyncCallback(); });
    }

    if (pinCallback && !item->isCloudItem()) {
        QAction* pinAction = menu->addAction(item->isPinned() ? QStringLiteral("取消置顶") : QStringLiteral("置顶"));
        QObject::connect(pinAction, &QAction::triggered, [pinCallback]{ pinCallback(); });
    }

    if (deleteCallback) {
        QAction* deleteAction = menu->addAction(QStringLiteral("删除"));
        QObject::connect(deleteAction, &QAction::triggered, [deleteCallback]{ deleteCallback(); });
    }

    return menu;
}
