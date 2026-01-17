#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QColor>
#include <QString>
#include <QStyleHints>
#include <QApplication>

class ThemeManager
{
public:
    // 获取单例实例的静态方法
    static ThemeManager& Instance()
    {
        static ThemeManager instance;
        return instance;
    }

    // 禁止拷贝构造和赋值
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    // 获取当前颜色方案（明/暗）
    bool isDarkMode() const;

    // 获取主颜色（图标颜色）
    QColor primaryColor() const;
    
    // 设置主颜色（图标颜色）
    void setPrimaryColor(const QColor& color);
    
    // 获取辅助颜色
    QColor secondaryColor() const;
    
    // 设置辅助颜色
    void setSecondaryColor(const QColor& color);
    
    // 获取边框颜色（基于辅助颜色和当前主题）
    QColor borderColor() const;
    
    // 生成统一的样式表
    QString styleSheet() const;
    
    // 生成特定组件的样式表
    QString borderStyleSheet() const;
    
private:
    ThemeManager();
    ~ThemeManager();
    
    QColor m_primaryColor;
    QColor m_secondaryColor;
};

#endif // THEMEMANAGER_H