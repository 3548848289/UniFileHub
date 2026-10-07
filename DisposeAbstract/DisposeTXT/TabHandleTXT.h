#ifndef TABHANDLETXT_H
#define TABHANDLETXT_H

#include "../../main/include/TabAbstract.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QSettings>
#include <QSplitter>
#include "ControlWidTXT.h"
#include "PlainTextEdit.h"
#include "LineNumberWidget.h"
#include "SyntaxHighlighter.h"
#include <QString>
#include <QByteArray>

class QTextBrowser;
class QStackedWidget;

class TextTab : public TabAbstract
{
    Q_OBJECT

public:
    explicit TextTab(const QString& filePath, QWidget *parent = nullptr);
    


    void setContent(const QString &text) override;
    QString getContent() const override;
    void loadFromFile(const QString &fileName) override;
    void loadFromInternet(const QByteArray &content) override;
    void saveToFile(const QString &fileName) override;
    void ControlWidget(bool judge){
        isShowControl = judge;
    }
    
    // 获取当前编码名称
    QString getCurrentCodecName() const;
    
    // 设置当前编码名称
    void setCurrentCodecName(const QString& codecName);
    
    // 将编码名称转换为Qt支持的格式
    static QString normalizeCodecName(const QString& codecName);
    
private:
    // 辅助函数：将字节数组根据当前编码转换为字符串
    QString decodeContent(const QByteArray &content) const;
    
    // 辅助函数：将字符串根据当前编码转换为字节数组
    QByteArray encodeContent(const QString &text) const;
    void updateTextStatistics();
    void ensurePreviewWidget();
    void showPreview(bool enabled);

public:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    
    
public slots:
    void findNext(const QString &str, Qt::CaseSensitivity cs);
    void findAll(const QString &str, Qt::CaseSensitivity cs);
    void clearHighlight();
    
    // 处理编码变化
    void onEncodingChanged(const QString& codecName);
    
    // 处理文本滚动
    void onTextScrolled(int value);
    
    // 处理md预览模式切换
    void onMdPreviewToggled(bool enabled);

    // 处理HTML预览模式切换
    void onHtmlPreviewToggled(bool enabled);

    // 处理语法高亮开关
    void onHighlightToggled(bool enabled);
    void onRefreshRequested();
    
    // 右键菜单槽函数


private:
    ControlWidTXT * controlWidtxt;
    QSplitter * splitter;
    PlainTextEdit *textEdit;
    LineNumberWidget *lineNumberWidget;
    QWidget *textContainer;
    QHBoxLayout *textLayout;
    QStackedWidget *textStack; // 文本区/预览区的堆叠容器,占 splitter 的一个槽位
    SyntaxHighlighter *m_syntaxHighlighter;
    QTextBrowser *m_previewEdit = nullptr; // MD/HTML 预览,懒创建;源文本始终留在编辑器中
    QString m_currentCodecName;
};

#endif // TABHANDLETXT_H
