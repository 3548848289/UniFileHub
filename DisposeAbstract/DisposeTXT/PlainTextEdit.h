#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QPlainTextEdit>
#include <QMimeData>

class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget *parent = nullptr);

    // LineNumberWidget 需要的布局接口,在 QPlainTextEdit 中是 protected
    using QPlainTextEdit::firstVisibleBlock;
    using QPlainTextEdit::blockBoundingRect;
    using QPlainTextEdit::blockBoundingGeometry;
    using QPlainTextEdit::contentOffset;

protected:
    void insertFromMimeData(const QMimeData *source) override;
};

#endif // PLAINTEXTEDIT_H
