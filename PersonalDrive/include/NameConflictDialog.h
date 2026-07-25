#ifndef NAMECONFLICTDIALOG_H
#define NAMECONFLICTDIALOG_H

#include <QDialog>
#include <QString>

class QPushButton;
class QLineEdit;
class QWidget;

enum class NameConflictAction {
    Cancel,
    Overwrite,
    AutoRename,
    CustomName
};

struct NameConflictChoice {
    NameConflictAction action = NameConflictAction::Cancel;
    QString customName;
};

class NameConflictDialog : public QDialog
{
public:
    static NameConflictChoice getChoice(QWidget *parent,
                                        const QString &fileName,
                                        const QString &detailText,
                                        const QString &overwriteText,
                                        const QString &autoRenameText,
                                        const QString &customNameText);

private:
    explicit NameConflictDialog(const QString &fileName,
                                const QString &detailText,
                                const QString &overwriteText,
                                const QString &autoRenameText,
                                const QString &customNameText,
                                QWidget *parent = nullptr);

    QPushButton *createActionButton(const QString &text, const QString &objectName);
    NameConflictChoice choice() const;

    QLineEdit *m_customEdit = nullptr;
};

#endif // NAMECONFLICTDIALOG_H
