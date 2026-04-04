#ifndef DRAWTOOLPANEL_H
#define DRAWTOOLPANEL_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class DrawToolPanel; }
QT_END_NAMESPACE

class QButtonGroup;

class DrawToolPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DrawToolPanel(QWidget *parent = nullptr);
    ~DrawToolPanel();

    void clearSelection();
    QString currentToolKey() const;

signals:
    void toolSelected(const QString &toolKey);
    void exportRequested();
    void copyRequested();
    void clearRequested();

private:
    void setupConnections();

    Ui::DrawToolPanel *ui;
    QButtonGroup *toolButtonGroup = nullptr;
};

#endif // DRAWTOOLPANEL_H
