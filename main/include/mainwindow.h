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
#include "finddialog.h"
#include "TabAbstract.h"
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
#include "../../EmailService/SendEmail.h"
#include "../../ClipBoard/ClipboardView.h"


// 映射表，可以放在 cpp 文件顶部，或函数内 static
using TabFactory = std::function<TabAbstract*(const QString&)>;

static const QMap<QString, TabFactory> tabFactories = {
    // --- 文本/代码类 ---
    { "txt",   [](const QString& f){ return new TextTab(f); } },
    { "cpp",   [](const QString& f){ return new TextTab(f); } },
    { "c",     [](const QString& f){ return new TextTab(f); } },
    { "h",     [](const QString& f){ return new TextTab(f); } },
    { "hpp",   [](const QString& f){ return new TextTab(f); } },
    { "cc",    [](const QString& f){ return new TextTab(f); } },
    { "java",  [](const QString& f){ return new TextTab(f); } },
    { "py",    [](const QString& f){ return new TextTab(f); } },
    { "js",    [](const QString& f){ return new TextTab(f); } },
    { "ts",    [](const QString& f){ return new TextTab(f); } },
    { "html",  [](const QString& f){ return new TextTab(f); } },
    { "htm",   [](const QString& f){ return new TextTab(f); } },
    { "css",   [](const QString& f){ return new TextTab(f); } },
    { "xml",   [](const QString& f){ return new TextTab(f); } },
    { "qrc",   [](const QString& f){ return new TextTab(f); } },
    { "ini",   [](const QString& f){ return new TextTab(f); } },
    { "json",  [](const QString& f){ return new TextTab(f); } },
    { "yaml",  [](const QString& f){ return new TextTab(f); } },
    { "yml",   [](const QString& f){ return new TextTab(f); } },
    { "md",    [](const QString& f){ return new TextTab(f); } },
    { "log",   [](const QString& f){ return new TextTab(f); } },
    { "bat",   [](const QString& f){ return new TextTab(f); } },
    { "sh",    [](const QString& f){ return new TextTab(f); } },

    // --- 表格/数据类 ---
    { "csv",   [](const QString& f){ return new TabHandleCSV(f); } },
    { "tsv",   [](const QString& f){ return new TabHandleCSV(f); } },  // TSV 可复用 CSV 处理
    { "xlsx",  [](const QString& f){ return new TabHandleXLSX(f); } },
    { "xls",   [](const QString& f){ return new TabHandleXLSX(f); } },
    { "ods",   [](const QString& f){ return new TabHandleXLSX(f); } },

    // --- 图片类 ---
    { "png",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "jpg",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "jpeg",  [](const QString& f){ return new TabHandleIMG(f); } },
    { "bmp",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "svg",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "gif",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "webp",  [](const QString& f){ return new TabHandleIMG(f); } },
    { "tiff",  [](const QString& f){ return new TabHandleIMG(f); } },
    { "ico",   [](const QString& f){ return new TabHandleIMG(f); } },

    // --- 视频类 ---
    { "mp4",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "avi",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "mov",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "wmv",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "mkv",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "flv",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "webm",  [](const QString& f){ return new TabHandleVideo(f); } },
    };



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

private slots:
    void on_actionopen_triggered();
    void on_actionsave_triggered();
    void on_actionclose_triggered();
    void on_actionfind_triggered();
    void handleFileDownload(const QString &fileName, const QByteArray &fileContent);
    void handleFilePathSent();
    void openFile(const QString &filePath);
    void showUserInfoDialog();

private:
    Ui::MainWindow *ui;
    int currentIndex = 0;
    QMap<QString, int> fileTabMap;

    QPushButton *loginButton;
    QTabWidget *tabWidget;
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
    void createNewTab(std::function<TabAbstract*()> tabFactory, const QString &tabName);
    TabAbstract* createTabByFileName(const QString &fileName);
    template<typename T>
    T* getCurrentTab();
    void closeTab(int index);
    void onTabContextMenuRequested(const QPoint &pos);
    int addTab(TabAbstract *tab, const QString &displayName, const QString &filePath);
};

#endif // MAINWINDOW_H
