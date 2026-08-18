#include "include/ClipboardComponentFactory.h"

// 初始化静态成员变量
QHash<ClipboardView*, ClipboardController*> ClipboardComponentFactory::m_viewControllerMap;

ClipboardView *ClipboardComponentFactory::createClipboardComponent(QWidget *parent)
{
    ClipboardController* controller = createClipboardController(parent);
    return createClipboardComponent(controller, parent);
}

ClipboardView *ClipboardComponentFactory::createClipboardComponent(ClipboardController *controller, QWidget *parent)
{
    if (!controller) {
        controller = createClipboardController(parent);
    } else if (parent && !controller->parent()) {
        controller->setParent(parent);
    }

    ClipboardView* view = new ClipboardView(controller, parent);

    m_viewControllerMap[view] = controller;
    QObject::connect(view, &QObject::destroyed, [view]() {
        ClipboardComponentFactory::cleanup(view);
    });
    return view;
}

ClipboardController *ClipboardComponentFactory::createClipboardController(QWidget *parent)
{
    return new ClipboardController(parent);
}

ClipboardController *ClipboardComponentFactory::getController(ClipboardView *view)
{
    auto it = m_viewControllerMap.find(view);
    if (it != m_viewControllerMap.end()) {
        return it.value();
    }
    return nullptr;
}

void ClipboardComponentFactory::cleanup(ClipboardView *view)
{
    auto it = m_viewControllerMap.find(view);
    if (it != m_viewControllerMap.end()) {
        // 如果Controller的父对象不是View，手动删除
        ClipboardController* controller = it.value();
        if (controller->parent() != view) {
            delete controller;
        }
        m_viewControllerMap.erase(it);
    }
}
