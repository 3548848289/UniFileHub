#ifndef CONTROLWIDARCHIVE_H
#define CONTROLWIDARCHIVE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>

// 归档预览的底部操作栏（参考 ControlWidTXT / ControlFrame 的模式）：
// 类内管控文件属性文字、解压按钮与解压进度条，TabHandleArchive 只连接信号。
// 放在 QSplitter 下部，分隔条可上下拖动；可见性由 SettingManager
// 的 file_see/archive 设置控制（设置页开关）。
class ControlWidArchive : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWidArchive(QWidget *parent = nullptr);

    // 文件属性文字（格式/条目数/总大小），显示在按钮上方
    void setStatusText(const QString &text);
    // 临时提示（如"解压成功"）：与解压进度条共用底部一行，
    // 空闲时顶替进度条显示，忙碌时显示在进度条旁边，msecs 毫秒后消失
    void flashStatus(const QString &text, int msecs = 3000);
    // 选中项里是否存在可操作文件（控制"解压选中"可用性）
    void setSelectionExtractable(bool enabled);

    // 进入/退出解压忙碌态：进度条与取消按钮出现，解压按钮禁用
    void setBusy(bool busy, const QString &text = {});
    // 更新解压进度（done/total 字节，currentName 当前条目）
    void showProgress(quint64 done, quint64 total, const QString &currentName);

signals:
    void extractAllHereRequested();
    void extractAllToRequested();
    void extractSelectedToRequested();
    void cancelRequested();

private:
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_btnExtractHere = nullptr;
    QPushButton *m_btnExtractAllTo = nullptr;
    QPushButton *m_btnExtractSelectedTo = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_flashLabel = nullptr;  // 临时提示，与进度条共用一行
    QPushButton *m_btnCancel = nullptr;
    bool m_busy = false;             // 是否处于解压忙碌态
    int m_flashSerial = 0;           // 提示序号：新的忙碌/提示作废旧提示的隐藏计时
};

#endif // CONTROLWIDARCHIVE_H
