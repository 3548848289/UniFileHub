#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QTextEdit>
#include <QMimeData>

class PlainTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget *parent = nullptr);

protected:
    void insertFromMimeData(const QMimeData *source) override;
};

#endif // PLAINTEXTEDIT_H
