#include "include/ClipboardMenuBuilder.h"
#include <QAction>

QMenu* ClipboardMenuBuilder::buildMenu(
    ClipboardItem* item,
    std::function<void()> copyCallback,
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

    if (item->type() == ClipboardItemType::Text && !item->isCloudItem() && cloudSyncCallback) {
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
