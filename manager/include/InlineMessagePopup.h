#ifndef INLINEMESSAGEPOPUP_H
#define INLINEMESSAGEPOPUP_H

#include <QWidget>
#include <QPointer>

class QLabel;

// 通用悬浮提示条：无边框、自动消失（网盘/登录面板/文件标签/文件备份共用）
// 弹窗直接挂在宿主（面板/对话框）上，宿主为普通 QWidget 不会裁剪子控件
// 用法：作为宿主 QWidget 的子控件，showMessage() 显示
// 注意：若宿主是列表项等会被 QListView/QListWidget 裁剪的容器，请传入所属面板
// 定位模式二选一：
//   setPanelWidget()：弹窗显示在面板内部顶部居中（页面/对话框推荐）
//   setAnchorWidget()：弹窗显示在锚点控件正下方（输入框等小控件）
class InlineMessagePopup : public QWidget
{
    Q_OBJECT

public:
    explicit InlineMessagePopup(QWidget *host, int autoHideMs = 3000);

    // 面板模式：弹窗显示在该面板内部顶部居中（不会被面板边界裁剪）
    void setPanelWidget(QWidget *panel);

    // 锚定模式：弹窗显示在该控件正下方（水平居中对齐锚点）
    void setAnchorWidget(QWidget *widget);

    // 显示提示（自动按错误/成功配色），自动定位并计时隐藏
    void showMessage(const QString &message, bool isError = false);

    // 宿主尺寸变化后重新定位（在宿主 resizeEvent 中调用）
    void reposition();

private:
    void applyTheme(bool isError);

    QWidget *m_host;
    QPointer<QWidget> m_panel;  // 面板模式参照
    QPointer<QWidget> m_anchor;  // 锚定模式参照
    QLabel *m_label;
    int m_autoHideMs;
    int m_serial = 0; // 序号防抖：新消息覆盖旧的隐藏计时
};

#endif // INLINEMESSAGEPOPUP_H
