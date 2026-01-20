#ifndef TABHANDLETXT_H
#define TABHANDLETXT_H

#include "../../main/include/TabAbstract.h"
#include <QTextEdit>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QSettings>
#include <QSplitter>
#include <QStringDecoder>
#include <QStringEncoder>
#include "ControlWidTXT.h"


#include <Windows.h>
#include <QString>
#include <QByteArray>



// 行号显示组件
class LineNumberWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberWidget(QTextEdit *editor, QWidget *parent = nullptr);
    void updateLineNumbers();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
private:
    QTextEdit *m_textEdit;
};

class TextTab : public TabAbstract
{
    Q_OBJECT

public:
    explicit TextTab(const QString& filePath, QWidget *parent = nullptr);
    QString gbkToQString(const QByteArray &data)
    {
        if (data.isEmpty())
            return QString();

        int len = MultiByteToWideChar(CP_ACP, 0, data.constData(), data.size(), nullptr, 0);
        if (len <= 0)
            return QString();

        std::wstring buf(len, L'\0');
        MultiByteToWideChar(CP_ACP, 0, data.constData(), data.size(), buf.data(), len);
        return QString::fromStdWString(buf);
    }

    QByteArray qStringToGbk(const QString &str)
    {
        if (str.isEmpty())
            return QByteArray();

        int len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)str.utf16(), str.size(), nullptr, 0, nullptr, nullptr);
        if (len <= 0)
            return QByteArray();

        QByteArray buf(len, 0);
        WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)str.utf16(), str.size(), buf.data(), len, nullptr, nullptr);
        return buf;
    }

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

public:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    // 创建自定义右键菜单
    void createCustomContextMenu(const QPoint &pos);
    
public slots:
    void findNext(const QString &str, Qt::CaseSensitivity cs);
    void findAll(const QString &str, Qt::CaseSensitivity cs);
    void clearHighlight();
    
    // 处理编码变化
    void onEncodingChanged(const QString& codecName);
    
    // 处理文本滚动
    void onTextScrolled(int value);
    
    // 右键菜单槽函数
    void pasteWithFormat();
    void pasteAsPlainText();

private:
    ControlWidTXT * controlWidtxt;
    QSplitter * splitter;
    QTextEdit *textEdit;
    LineNumberWidget *lineNumberWidget;
    QWidget *textContainer;
    QHBoxLayout *textLayout;
    SyntaxHighlighter *m_syntaxHighlighter;
    QString m_currentCodecName;  // 当前使用的编码名称
};

#endif // TABHANDLETXT_H
