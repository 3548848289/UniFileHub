#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QIcon>
#include <QFont>
#include <QStyleFactory>
#include <QStyleHints>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QDir>
#include <QFileInfo>
#include <QPalette>
#include <QSettings>
#include <QDebug>
#include "mainwindow.h"
#ifdef Q_OS_WIN
// 必须在 mainwindow.h 之后包含：mainwindow.h 经 KodoTerm 拉入的 vterm.h 里有
// 名为 small 的位域成员，而 windows.h 链上的 rpcndr.h 定义了 #define small char
#include <qt_windows.h>
#endif
#include "../Setting/include/SettingManager.h"
#include "../Setting/include/IconManager.h"
#include "../Setting/include/ShellContextMenu.h"
#include "../manager/include/ServerDiscovery.h"
#include "../../PersonalDrive/include/DriveView.h"

#define SERVER_NAME "SmartDesk_Server"

namespace {

//先强制中文
QString appTranslationBaseName()
{
    return QLocale::system().language() == QLocale::Chinese
               ? "UniFileHub_zh_CN"
               : "UniFileHub_zh_CN";
}

QString appTranslationDirectory()
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString baseName = appTranslationBaseName();
    const QStringList candidateDirs = {
        appDir.filePath("translations"),
        appDir.filePath("../translations")
    };

    for (const QString &dirPath : candidateDirs) {
        if (QFileInfo::exists(QDir(dirPath).filePath(baseName + ".qm"))) {
            return QDir(dirPath).absolutePath();
        }
    }

    return appDir.filePath("translations");
}

void installTranslators(QApplication &app)
{
    static QTranslator appTranslator;
    static QTranslator qtTranslator;

    const QString baseName = appTranslationBaseName();
    const QString translationDir = appTranslationDirectory();

    if (appTranslator.load(baseName, translationDir)) {
        app.installTranslator(&appTranslator);
    }

    if (QLocale::system().language() == QLocale::Chinese) {
#if QT_VERSION_MAJOR >= 6
        const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
        const QString qtTranslationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
        if (qtTranslator.load(QLocale("zh_CN"), "qtbase", "_", qtTranslationsPath)) {
            app.installTranslator(&qtTranslator);
        }
    }
}

}

// 解析命令行动作与文件路径：
//   <exe> "%1"              → open（兼容旧版右键命令/拖拽启动）
//   <exe> "backup" "%1" 等  → 对应动作（backup/tag/upload）
static void parseCommandLineAction(QString &action, QString &filePath) {
    const QStringList args = QCoreApplication::arguments();
    action = QStringLiteral("open");
    if (args.count() < 2) {
        return;
    }
    int pathIndex = 1;
    if (args.count() > 2) {
        action = args.at(1);
        pathIndex = 2;
    }
    filePath = args.at(pathIndex);
    QFileInfo fileInfo(filePath);
    if (fileInfo.isRelative()) {
        filePath = fileInfo.absoluteFilePath();
    }
}

bool connectToRunningInstance() {
    QLocalSocket socket;
    socket.connectToServer(SERVER_NAME);
    if (socket.waitForConnected(100)) {
        QString action;
        QString filePath;
        parseCommandLineAction(action, filePath);
        if (filePath.isEmpty()) {
            socket.write("show");
        } else {
            // 单实例协议："动作:文件路径"，动作 ∈ open/backup/tag/upload
            socket.write(action.toUtf8() + ":" + filePath.toUtf8());
        }
        socket.flush();
        socket.waitForBytesWritten(100);
        return true;
    }
    return false;
}

// 把主窗口强制带到前台显示。
// 由资源管理器右键唤起的单实例属于后台进程激活，Windows 前台锁会拦截
// activateWindow，表现为只闪任务栏不显示窗口；先最小化再还原可以绕过限制。
static void forceShowMainWindow(MainWindow *w) {
    const bool wasMaximized = SettingManager::Instance().getWindowMaximized();
    // 窗口已在前台时不必折腾，只补一次激活
    if (w->isVisible() && !w->isMinimized() && w->isActiveWindow()) {
        w->raise();
        w->activateWindow();
        return;
    }

    // 先从托盘/最小化状态恢复可见，保持最大化状态
    w->showNormal();
    if (wasMaximized) {
        w->showMaximized();
    }
    w->raise();
    w->activateWindow();
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(w->winId());
    ShowWindow(hwnd, SW_MINIMIZE);
    ShowWindow(hwnd, wasMaximized ? SW_MAXIMIZE : SW_RESTORE);
    SetForegroundWindow(hwnd);
#endif
}

QLocalServer* createLocalServer(MainWindow *w) {
    QLocalServer::removeServer(SERVER_NAME); // 防止残留
    QLocalServer *server = new QLocalServer(w);
    if (server->listen(SERVER_NAME)) {
        QObject::connect(server, &QLocalServer::newConnection, [server, w]() {
            QLocalSocket *clientConnection = server->nextPendingConnection();
            if (clientConnection) {
                clientConnection->waitForReadyRead(300);
                QByteArray msg = clientConnection->readAll();
                // "show"、空消息（数据未及时到达时的兜底）或 "动作:文件路径"
                const int sep = msg.indexOf(':');
                const bool showOnly = (msg == "show" || msg.isEmpty());
                if (showOnly || sep > 0) {
                    forceShowMainWindow(w);
                    if (sep > 0) {
                        const QString action = QString::fromUtf8(msg.left(sep));
                        const QString filePath = QString::fromUtf8(msg.mid(sep + 1));
                        w->handleExternalAction(action, filePath);
                    }
                }
                clientConnection->disconnectFromServer();
            }
        });
    }
    return server;
}

// ================= 托盘初始化 =================
QSystemTrayIcon* createTray(MainWindow *w, QApplication &app) {
    QSystemTrayIcon *trayIcon = new QSystemTrayIcon(w);
    trayIcon->setIcon(QIcon("://conf/UniFileHub.png"));
    trayIcon->setToolTip("UniFileHub");

    QMenu *trayMenu = new QMenu;
    QAction *showAction = new QAction("显示主窗口", trayMenu);
    QAction *settingAction = new QAction("设置", trayMenu);
    QAction *quitAction = new QAction("退出", trayMenu);

    trayMenu->addAction(showAction);
    trayMenu->addSeparator();
    trayMenu->addAction(settingAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);

    QObject::connect(trayIcon, &QSystemTrayIcon::activated, [w](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (!w->isVisible()) {
                // 如果窗口不可见，显示窗口（保持原状态）
                // 先获取保存的最大化状态
                bool wasMaximized = SettingManager::Instance().getWindowMaximized();
                if (wasMaximized) {
                    w->showMaximized();
                } else {
                    w->showNormal();
                }
                w->raise();
                w->activateWindow();
            } else {
                // 如果窗口可见，切换显示状态
                if (w->isMinimized()) {
                    // 如果窗口最小化，恢复之前的状态
                    bool wasMaximized = SettingManager::Instance().getWindowMaximized();
                    if (wasMaximized) {
                        w->showMaximized();
                    } else {
                        w->showNormal();
                    }
                    w->raise();
                    w->activateWindow();
                } else {
                    // 如果窗口正常或最大化，保存当前状态后最小化
                    SettingManager::Instance().setWindowMaximized(w->isMaximized());
                    w->showMinimized();
                }
            }
        }
    });
    QObject::connect(showAction, &QAction::triggered, [w]() {
        if (!w->isVisible() || w->isMinimized()) {
            // 如果窗口不可见或最小化，显示窗口（保持原状态）
            bool wasMaximized = SettingManager::Instance().getWindowMaximized();
            if (wasMaximized) {
                w->showMaximized();
            } else {
                w->showNormal();
            }
            w->raise();
            w->activateWindow();
        }
    });
    QObject::connect(settingAction, &QAction::triggered, [w]() {
        w->showSetting();
    });
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    trayIcon->show();
    trayIcon->showMessage("UniFileHub", "程序已最小化到托盘",
                          QIcon("://conf/UniFileHub.png"), 3000);

    return trayIcon;
}





int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    // 从资源管理器启动时工作目录不确定，固定到 exe 目录，
    // 保证 ./SmartDesk.db 等相对路径的落点与正常启动一致
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    // 按设置同步资源管理器右键菜单（勾选的注册，未勾选的清理残留）
    ShellContextMenu::applySettings();

    installTranslators(app);
    DriveView::clearExternalDragTempFiles();

    if (connectToRunningInstance()) {
        return 0;
    }

    int font_size = SettingManager::Instance().all_setting_font_size();
    int themeIndex = SettingManager::Instance().all_setting_theme();
#if QT_VERSION_MAJOR >= 6
    Qt::ColorScheme scheme = static_cast<Qt::ColorScheme>(themeIndex);

    qApp->styleHints()->setColorScheme(scheme);
#else
    Q_UNUSED(themeIndex);
#endif
    if (SettingManager::Instance().all_setting_fenable_tray()) {
        QApplication::setQuitOnLastWindowClosed(false);
    } else {
        QApplication::setQuitOnLastWindowClosed(true);
    }

    QFont font = QApplication::font();
    font.setPointSize(font_size);
    QApplication::setFont(font);

    // 从设置中加载默认图标颜色到IconManager
    QString iconColor = SettingManager::Instance().all_setting_icon_color();
    IconManager::setDefaultIconColor(QColor(iconColor));

    MainWindow w;

    QString action;
    QString filePath;
    parseCommandLineAction(action, filePath);
    if (!filePath.isEmpty()) {
        qDebug() << "Command line action:" << action << "path:" << filePath;

        QTimer::singleShot(100, [&w, action, filePath]() {
            qDebug() << "Handling external action:" << action << filePath;
            w.handleExternalAction(action, filePath);
        });
    }

    w.show();

    QLocalServer *server = createLocalServer(&w);

    Q_UNUSED(server);

    // 启动时局域网广播寻找服务端，找到后自动更新各服务地址；
    // 找不到则保持 settings.ini 里手动配置的地址不变
    ServerDiscovery* discovery = new ServerDiscovery(2000, &app);
    QObject::connect(discovery, &ServerDiscovery::discovered, &w,
                     [](const QString& ip) {
                         qDebug() << "Server discovered at" << ip;
                     });
    discovery->start();

    if (SettingManager::Instance().all_setting_fenable_tray()) {
        createTray(&w, app);
    }

    return app.exec();
}
