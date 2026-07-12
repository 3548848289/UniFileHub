#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <functional>
#include <QWidget>
#include <QDockWidget>
#include <QStackedWidget>
#include <QSplitter>
#include <QWidgetAction>
#include <QCheckBox>

#include "RecentFilesManager.h"
#include "WidgetFunctional.h"
#include "FindDialog.h"
#include "TabAbstract.h"
#include "TabManager.h" // 包含LayoutType
#include "../../DisposeAbstract/DisposeTXT/TabHandleTXT.h"
#include "../../DisposeAbstract/DisposeCSV/TabHandleCSV.h"
#include "../../DisposeAbstract/DisposeIMG/include/TabHandleIMG.h"
#include "../../DisposeAbstract/DisposeXLSX/TabHandleXLSX.h"
#include "../../DisposeAbstract/DisposeVideo/TabHandleVideo.h"
#include "../../DisposeAbstract/DisposeCSV/ControlWidCSV.h"
#include "../../DisposeAbstract/DisposeIMG/include/ControlFrame.h"
#include "../../OnlineDoc/include/WOnlineDoc.h"
#include "../../Setting/include/Setting.h"
#include "../../TagSchedule/include/ScheduleWid.h"
#include "../../FileSystem/include/FileSystem.h"
#include "../../FileBackup/include/FileBackupView.h"
#include "../../manager/include/dbService.h"
#include "../../Resources/ThirdParty/KodoTerm/include/KodoTerm/KodoTerm.hpp"

#include "TabManager.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void showUserInfo();

public slots:
    void showSetting();
    void openFileFromCommandLine(const QString& filePath);

private slots:
    void on_actionopen_triggered();
    void on_actionsave_triggered();
    void on_actionclose_triggered();
    void on_actionTerminal_triggered();
    void handleFileDownload(const QString &fileName, const QByteArray &fileContent);
    void handleFilePathSent();
    void showUserInfoDialog();

    void on_actionfind_triggered();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    int currentIndex = 0;

    // QMap<QString, int> fileTabMap;
    QTabWidget *tabWidget;
    TabManager* tabManager;

    QPushButton *loginButton;
    RecentFilesManager *recentFilesManager;

    FileSystem * file_system;
    FileBackupView * file_backup_view;
    WOnlineDoc* wonlinedoc;
    WidgetFunctional* widgetfunc;
    Setting * setiing;
    ScheduleWid * schedule_wid;
    FindDialog *findDialog;

    void initCoreWidgets();
    void initConnect();
    void initMemubarLayout();
    void onTabContextMenuRequested(const QPoint &pos);
    void togglePanel(QWidget *target);
};

#endif // MAINWINDOW_H
