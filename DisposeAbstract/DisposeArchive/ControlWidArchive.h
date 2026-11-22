#ifndef CONTROLWIDARCHIVE_H
#define CONTROLWIDARCHIVE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

// 归档预览的底部操作栏（参考 ControlWidTXT / ControlFrame 的模式）：
// 类内管控按钮与状态文字的创建，TabHandleArchive 只连接信号。
// 放在 QSplitter 下部，分隔条可上下拖动；可见性由 SettingManager
// 的 file_see/archive 设置控制（设置页开关）。
class ControlWidArchive : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWidArchive(QWidget *parent = nullptr);

    // 底部状态文字（格式/条目数/总大小）
    void setStatusText(const QString &text);
    // 选中项里是否存在可操作文件（控制"解压选中到…"可用性）
    void setSelectionExtractable(bool enabled);

signals:
    void extractAllHereRequested();
    void extractAllToRequested();
    void extractSelectedToRequested();

private:
    QPushButton *m_btnExtractHere = nullptr;
    QPushButton *m_btnExtractAllTo = nullptr;
    QPushButton *m_btnExtractSelectedTo = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif // CONTROLWIDARCHIVE_H
