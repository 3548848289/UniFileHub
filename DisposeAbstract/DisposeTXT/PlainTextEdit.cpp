#include "PlainTextEdit.h"

PlainTextEdit::PlainTextEdit(QWidget *parent) : QTextEdit(parent)
{
}

void PlainTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (source->hasText()) {
        insertPlainText(source->text());
    }
}