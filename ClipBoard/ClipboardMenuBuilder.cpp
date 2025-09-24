#include "include/ClipboardMenuBuilder.h"
#include <QAction>

QMenu* ClipboardMenuBuilder::buildMenu(
    ClipboardItem* item,
    std::function<void()> copyCallback,
    std::function<void()> previewCallback,
    std::function<void()> openLocationCallback,
    std::function<void()> deleteCallback,
    std::function<void()> pinCallback   // 新增参数
    )
{
    if (!item) return nullptr;

    QMenu* menu = new QMenu;

    // 复制
    QAction* copyAction = menu->addAction("复制到剪贴板");
    QObject::connect(copyAction, &QAction::triggered, [copyCallback]{ if(copyCallback) copyCallback(); });

    // 图片预览
    bool canPreview = false;
    if (item->type() == ClipboardItemType::Image) canPreview = true;
    else if (item->type() == ClipboardItemType::File) {
        auto* fileItem = static_cast<CliFile*>(item);
        canPreview = fileItem->isImageFile();
    } else if (item->type() == ClipboardItemType::Text) {
        auto* textItem = static_cast<CliText*>(item);
        canPreview = FileTypeDetector::isImageFile(textItem->text());
    }
    if (canPreview && previewCallback) {
        QAction* previewAction = menu->addAction("预览图片");
        QObject::connect(previewAction, &QAction::triggered, [previewCallback]{ previewCallback(); });
    }

    // 打开文件位置
    if (item->type() == ClipboardItemType::File && openLocationCallback) {
        QAction* openLocationAction = menu->addAction("打开文件位置");
        QObject::connect(openLocationAction, &QAction::triggered, [openLocationCallback]{ openLocationCallback(); });
    }

    // === 新增：置顶/取消置顶 ===
    if (pinCallback) {
        QAction* pinAction = menu->addAction(item->isPinned() ? "取消置顶" : "置顶");
        QObject::connect(pinAction, &QAction::triggered, [pinCallback]{ pinCallback(); });
    }

    // 删除
    if (deleteCallback) {
        QAction* deleteAction = menu->addAction("删除");
        QObject::connect(deleteAction, &QAction::triggered, [deleteCallback]{ deleteCallback(); });
    }

    return menu;
}
