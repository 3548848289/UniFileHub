#ifndef CONTROLWIDDB_H
#define CONTROLWIDDB_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace Ui {
class ControlWidDB;
}

class ControlWidDB : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWidDB(QWidget *parent = nullptr);
    ~ControlWidDB();

private slots:
    void onExecuteButtonClicked();
    void onRefreshButtonClicked();

signals:
    void executeQuery(const QString &query);
    void refreshRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
private:
    Ui::ControlWidDB *ui;
    QLineEdit *queryEdit;
    QPushButton *executeButton;
    QPushButton *refreshButton;
    QVBoxLayout *mainLayout;
};

#endif // CONTROLWIDDB_H
