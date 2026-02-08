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
#include <QtCore5Compat/QTextCodec>
#include "ControlWidTXT.h"
#include "PlainTextEdit.h"
#include "LineNumberWidget.h"
#include "SyntaxHighlighter.h"
#include <QString>
#include <QByteArray>
#include <QStringDecoder>
#include <QStringEncoder>

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
    
    // 右键菜单槽函数


private:
    ControlWidTXT * controlWidtxt;
    QSplitter * splitter;
    PlainTextEdit *textEdit;
    LineNumberWidget *lineNumberWidget;
    QWidget *textContainer;
    QHBoxLayout *textLayout;
    SyntaxHighlighter *m_syntaxHighlighter;
    QString m_currentCodecName;
    QString m_originalPlainText; // 保存原始纯文本内容
};

#endif // TABHANDLETXT_H
