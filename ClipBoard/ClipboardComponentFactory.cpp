#include "include/ClipboardComponentFactory.h"

// 初始化静态成员变量
QHash<ClipboardView*, ClipboardController*> ClipboardComponentFactory::m_viewControllerMap;