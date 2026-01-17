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
#include "mainwindow.h"
#include "../../Setting/include/SettingManager.h"
#include "../../Setting/include/IconManager.h"

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


#include "mainwindow.h"
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QTimer>
#include <QMessageBox>

// 注册表关联函数
void dectionRegedit(const QString& strClassName,
                    const QString& strAppPath,
                    const QString& strExt,
                    const QString& strExtDescri)
{
    QString strBaseUrl("HKEY_CURRENT_USER\\Software\\Classes");
    QSettings setting(strBaseUrl, QSettings::NativeFormat);

    QString nativeAppPath = QDir::toNativeSeparators(strAppPath);

    // 注册 Shell Open 命令
    setting.setValue("/" + strClassName + "/Shell/Open/Command/.", "\"" + nativeAppPath + "\" \"%1\"");
    // 文件描述
    setting.setValue("/" + strClassName + "/.", strExtDescri);
    // 默认图标
    setting.setValue("/" + strClassName + "/DefaultIcon/.", nativeAppPath + ",0");
    // 关联扩展名
    setting.setValue("/" + strExt + "/OpenWithProgIds/" + strClassName, "");
    // 保存修改
    setting.sync();
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

    // 从设置中加载默认图标颜色到IconManager
    QString iconColor = SettingManager::Instance().all_setting_icon_color();
    IconManager::setDefaultIconColor(QColor(iconColor));

    MainWindow w;
    QString strAppPath = QApplication::applicationFilePath(); // 当前 exe
    QString strClassName("SmartDeskFile"); // 类名，自定义即可
    QString strExt(".txt"); // 测试 txt 文件
    QString strExtDescri("SmartDesk 文本文件");

    dectionRegedit(strClassName, strAppPath, strExt, strExtDescri);

    QStringList args = QCoreApplication::arguments();
    QString filePath;
    if (args.count() > 1) {
        filePath = args.at(1); // args[0] 是程序自身路径，args[1] 是文件路径
        // 打开日志文件（追加模式）
        QFile logFile("cmdline_debug.txt");
        if (logFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&logFile);
            out << "Command line argument: " << filePath << "\n";
            out << "Current working directory: " << QDir::currentPath() << "\n";

            // 转换为绝对路径
            QFileInfo fileInfo(filePath);
            if (fileInfo.isRelative()) {
                filePath = fileInfo.absoluteFilePath();
                out << "Converted to absolute path: " << filePath << "\n";
            }

            if (QFile::exists(filePath)) {
                out << "File exists, opening...\n";
                QTimer::singleShot(100, [&w, filePath]() {
                    w.openFileFromCommandLine(filePath);
                });
            } else {
                out << "File does not exist: " << filePath << "\n";
                QMessageBox::warning(nullptr, "错误", QString("文件不存在: %1").arg(filePath));
            }
            logFile.close();
        }
    }


    w.show();

    // QLocalServer *server = createLocalServer(&w);
    if (SettingManager::Instance().all_setting_fenable_tray()) {
        createTray(&w, app);
    }

    return app.exec();
}
