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
#include "mainwindow.h"
#include "../../Setting/include/SettingManager.h"
#include "../../Setting/include/IconManager.h"

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
        const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
        if (qtTranslator.load(QLocale("zh_CN"), "qtbase", "_", qtTranslationsPath)) {
            app.installTranslator(&qtTranslator);
        }
    }
}

}

bool connectToRunningInstance() {
    QLocalSocket socket;
    socket.connectToServer(SERVER_NAME);
    if (socket.waitForConnected(100)) {
        QStringList args = QCoreApplication::arguments();
        if (args.count() > 1) {
            QString filePath = args.at(1);
            // 转换为绝对路径
            QFileInfo fileInfo(filePath);
            if (fileInfo.isRelative()) {
                filePath = fileInfo.absoluteFilePath();
            }
            socket.write("open:" + filePath.toUtf8());
        } else {
            socket.write("show");
        }
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
                if (msg.startsWith("open:")) {
                    QString filePath = QString(msg.mid(5)); // 移除"open:"前缀
                    // 显示窗口时保持之前的状态
                    bool wasMaximized = SettingManager::Instance().getWindowMaximized();
                    if (wasMaximized) {
                        w->showMaximized();
                    } else {
                        w->showNormal();
                    }
                    w->raise();
                    w->activateWindow();
                    w->openFileFromCommandLine(filePath);
                } else if (msg == "show") {
                    // 显示窗口时保持之前的状态
                    bool wasMaximized = SettingManager::Instance().getWindowMaximized();
                    if (wasMaximized) {
                        w->showMaximized();
                    } else {
                        w->showNormal();
                    }
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
    installTranslators(app);

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

    // 从设置中加载默认图标颜色到IconManager
    QString iconColor = SettingManager::Instance().all_setting_icon_color();
    IconManager::setDefaultIconColor(QColor(iconColor));

    MainWindow w;

    QStringList args = QCoreApplication::arguments();
    QString filePath;
    if (args.count() > 1) {
        filePath = args.at(1);

        QFileInfo fileInfo(filePath);
        if (fileInfo.isRelative()) {
            filePath = fileInfo.absoluteFilePath();
        }

        if (QFile::exists(filePath)) {
            QTimer::singleShot(100, [&w, filePath]() {
                w.openFileFromCommandLine(filePath);
            });
        }

    }

    w.show();

    // QLocalServer *server = createLocalServer(&w);

    if (SettingManager::Instance().all_setting_fenable_tray()) {
        createTray(&w, app);
    }

    return app.exec();
}
