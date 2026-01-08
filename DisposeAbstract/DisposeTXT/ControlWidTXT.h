#ifndef CONTROLWIDTXT_H
#define CONTROLWIDTXT_H

#include <QWidget>
#include <QApplication>
#include <QTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QTextDocument>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QComboBox>
#include "../../Setting/include/SettingManager.h"

class CppHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    CppHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {}

protected:
    void highlightBlock(const QString &text) override
    {
        // 设置默认格式
        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(Qt::white);
        setFormat(0, text.length(), defaultFormat);

        // 关键字高亮
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(Qt::blue);  // 设置蓝色
        QRegularExpression keywordRegex("\\b(class|struct|public|private|protected|if|else|for|while|return|int|float|double|char|void)\\b");
        QRegularExpressionMatchIterator keywordIterator = keywordRegex.globalMatch(text);
        while (keywordIterator.hasNext()) {
            QRegularExpressionMatch match = keywordIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), keywordFormat);
        }

        // 字符串高亮
        QTextCharFormat stringFormat;
        stringFormat.setForeground(Qt::green);  // 设置绿色
        QRegularExpression stringRegex("\"([^\"]*)\"");
        QRegularExpressionMatchIterator stringIterator = stringRegex.globalMatch(text);
        while (stringIterator.hasNext()) {
            QRegularExpressionMatch match = stringIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), stringFormat);
        }

        // 注释高亮
        QTextCharFormat commentFormat;
        commentFormat.setForeground(Qt::darkGreen);  // 设置深绿色
        QRegularExpression commentRegex("//[^\n]*");
        QRegularExpressionMatchIterator commentIterator = commentRegex.globalMatch(text);
        while (commentIterator.hasNext()) {
            QRegularExpressionMatch match = commentIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), commentFormat);
        }
    }
};


QT_BEGIN_NAMESPACE
namespace Ui { class ControlWidTXT; }
QT_END_NAMESPACE

class ControlWidTXT : public QWidget
{
    Q_OBJECT
public:
    explicit ControlWidTXT(QWidget *parent = nullptr);
    ~ControlWidTXT();
    
    // 获取当前编码名称
    QString getCurrentCodecName() const;
    
    // 设置当前编码名称
    void setCurrentCodecName(const QString& codecName);

signals:
    // 编码方式改变信号
    void encodingChanged(const QString& codecName);

private slots:
    // 处理编码选择变化
    void onEncodingComboBoxCurrentIndexChanged(const QString& codecName);

private:
    Ui::ControlWidTXT *ui;  // UI 类的指针
    QString m_currentCodecName;  // 当前选择的编码名称
};

#endif // CONTROLWIDTXT_H
