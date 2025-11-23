#ifndef CLIPBOARDCOMPONENTFACTORY_H
#define CLIPBOARDCOMPONENTFACTORY_H

#include <QWidget>
#include <QHash>
#include "ClipboardController.h"
#include "ClipboardView.h"

class ClipboardComponentFactory
{
public:
    // 创建剪切板MVC组件
    static ClipboardView* createClipboardComponent(QWidget* parent = nullptr);
    
    // 获取与View关联的Controller
    static ClipboardController* getController(ClipboardView* view);
    
    // 清理View关联的资源
    static void cleanup(ClipboardView* view);

private:
    // 存储View和Controller的映射关系
    static QHash<ClipboardView*, ClipboardController*> m_viewControllerMap;
};

#endif // CLIPBOARDCOMPONENTFACTORY_H
