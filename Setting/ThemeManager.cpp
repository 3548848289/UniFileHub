#include "include/ThemeManager.h"
#include "include/SettingManager.h"

ThemeManager::ThemeManager()
{
    // 初始化颜色
    m_primaryColor = QColor(SettingManager::Instance().all_setting_icon_color());
    m_secondaryColor = QColor(SettingManager::Instance().all_setting_secondary_icon_color());
}

ThemeManager::~ThemeManager()
{
}

bool ThemeManager::isDarkMode() const
{
    // 优先使用设置中的主题配置
    int theme = SettingManager::Instance().all_setting_theme();
    if (theme == 1) {
        return false; // 浅色模式
    } else if (theme == 2) {
        return true; // 深色模式
    }
    // 跟随系统
    return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

QColor ThemeManager::primaryColor() const
{
    return m_primaryColor;
}

void ThemeManager::setPrimaryColor(const QColor& color)
{
    m_primaryColor = color;
}

QColor ThemeManager::secondaryColor() const
{
    return m_secondaryColor;
}

void ThemeManager::setSecondaryColor(const QColor& color)
{
    m_secondaryColor = color;
}

QColor ThemeManager::borderColor() const
{
    QColor secondary = secondaryColor();
    
    // 根据当前主题调整边框颜色的透明度
    if (isDarkMode()) {
        // 深色模式下增加透明度
        secondary.setAlphaF(0.3);
    } else {
        // 浅色模式下降低透明度
        secondary.setAlphaF(0.5);
    }
    
    return secondary;
}

QString ThemeManager::styleSheet() const
{
    QColor border = borderColor();
    QString borderColorStr = border.name(QColor::HexArgb);
    
    // 生成通用样式表
    QString stylesheet = QString(
        "/* 通用边框样式 */\n"
        "QWidget {\n"
        "    border-color: %1;\n"
        "}\n"
        "\n"
        "/* 按钮样式 */\n"
        "QPushButton {\n"
        "    border: 1px solid %1;\n"
        "    border-radius: 4px;\n"
        "    padding: 4px 8px;\n"
        "}\n"
        "\n"
        "/* 输入框样式 */\n"
        "QLineEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {\n"
        "    border: 1px solid %1;\n"
        "    border-radius: 4px;\n"
        "    padding: 2px 4px;\n"
        "}\n"
        "\n"
        "/* 标签页样式 */\n"
        "QTabWidget::pane {\n"
        "    border: 1px solid %1;\n"
        "    border-radius: 4px;\n"
        "}\n"
        "\n"
        "/* 分割器样式 */\n"
        "QSplitter::handle {\n"
        "    background-color: %1;\n"
        "}\n"
        "\n"
        "/* 列表视图样式 */\n"
        "QListView, QTreeView, QTableView {\n"
        "    border: 1px solid %1;\n"
        "    border-radius: 4px;\n"
        "}"
    ).arg(borderColorStr);
    
    return stylesheet;
}

QString ThemeManager::borderStyleSheet() const
{
    QColor border = borderColor();
    QString borderColorStr = border.name(QColor::HexArgb);
    
    // 生成仅包含边框的样式表
    return QString("border: 1px solid %1; border-radius: 4px;").arg(borderColorStr);
}