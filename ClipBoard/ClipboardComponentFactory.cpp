#include "include/ClipboardComponentFactory.h"

// 初始化静态成员变量
QHash<ClipboardView*, ClipboardController*> ClipboardComponentFactory::m_viewControllerMap;

ClipboardView *ClipboardComponentFactory::createClipboardComponent(QWidget *parent)
{
    // 1. 创建Controller
    ClipboardController* controller = new ClipboardController(parent);

    // 2. 创建View并连接到Controller
    ClipboardView* view = new ClipboardView(controller, parent);

    // 存储View和Controller的映射关系，便于后续获取Controller
    m_viewControllerMap[view] = controller;

    // 将Controller的父对象设为View，这样当View被销毁时，Controller也会被销毁
    // 避免内存泄漏
    if (parent == nullptr && view != nullptr) {
        controller->setParent(view);
    }

    return view;
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
