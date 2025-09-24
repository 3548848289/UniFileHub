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
#include "mainwindow.h"
#include "../../Setting/include/SettingManager.h"

#define SERVER_NAME "SmartDesk_Server"

bool connectToRunningInstance() {
    QLocalSocket socket;
    socket.connectToServer(SERVER_NAME);
    if (socket.waitForConnected(100)) {
        socket.write("show");
        socket.flush();
        socket.waitForBytesWritten(100);
        return true;
    }
    return false;
}

QLocalServer* createLocalServer(MainWindow *w) {
    QLocalServer::removeServer(SERVER_NAME); // 防止残留
    QLocalServer *server = new QLocalServer(w);
    if (server->listen(SERVER_NAME)) {
        QObject::connect(server, &QLocalServer::newConnection, [server, w]() {
            QLocalSocket *clientConnection = server->nextPendingConnection();
            if (clientConnection) {
                clientConnection->waitForReadyRead(100);
                QByteArray msg = clientConnection->readAll();
                if (msg == "show") {
                    w->showNormal();
                    w->raise();
                    w->activateWindow();
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
    trayIcon->setIcon(QIcon::fromTheme("utilities-system-monitor"));
    trayIcon->setToolTip("SmartDesk");

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
            w->showNormal();
            w->raise();
            w->activateWindow();
        }
    });
    QObject::connect(showAction, &QAction::triggered, [w]() {
        w->showNormal();
        w->raise();
        w->activateWindow();
    });
    QObject::connect(settingAction, &QAction::triggered, [w]() {
        w->showSetting();
    });
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    trayIcon->show();
    trayIcon->showMessage("SmartDesk", "程序已最小化到托盘",
                          QIcon::fromTheme("utilities-system-monitor"), 3000);

    return trayIcon;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // if (connectToRunningInstance()) {
    //     return 0;
    // }

    int font_size = SettingManager::Instance().all_setting_font_size();
    int themeIndex = SettingManager::Instance().all_setting_theme();
    Qt::ColorScheme scheme = static_cast<Qt::ColorScheme>(themeIndex);

    qApp->styleHints()->setColorScheme(scheme);

    if (SettingManager::Instance().all_setting_fenable_tray()) {
        QApplication::setQuitOnLastWindowClosed(false);
    } else {
        QApplication::setQuitOnLastWindowClosed(true);
    }

    QFont font = QApplication::font();
    font.setPointSize(font_size);
    QApplication::setFont(font);

    MainWindow w;
    w.show();

    // QLocalServer *server = createLocalServer(&w);
    if (SettingManager::Instance().all_setting_fenable_tray()) {
        createTray(&w, app);
    }

    return app.exec();
}
