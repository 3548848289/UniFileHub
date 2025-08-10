#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMainWindow>
#include <QMessageBox>
#include <QIcon>
#include <QFont>
#include <QStyleFactory>
#include "mainwindow.h"
#include "../../Setting/include/SettingManager.h"
#include <QStyleHints>
int main(int argc, char *argv[]) {

    int font_size = SettingManager::Instance().all_setting_font_size();
    int themeIndex = SettingManager::Instance().all_setting_theme();
    Qt::ColorScheme scheme = static_cast<Qt::ColorScheme>(themeIndex);

    QApplication app(argc, argv);


    // app.setStyle("windows11");
    // qDebug() << QStyleFactory::keys();

    qApp->styleHints()->setColorScheme(scheme);
    QApplication::setQuitOnLastWindowClosed(false);
    QFont font = QApplication::font();
    font.setPointSize(font_size);
    QApplication::setFont(font);

    MainWindow w;
    w.show();

    bool enableTray = SettingManager::Instance().all_setting_fenable_tray();
    if (enableTray) {
        QSystemTrayIcon *trayIcon = new QSystemTrayIcon;
        trayIcon->setIcon(QIcon::fromTheme("utilities-system-monitor"));
        // trayIcon->setIcon(QIcon(":/usedimage/tips.png"));

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

        QObject::connect(trayIcon, &QSystemTrayIcon::activated, [&w](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger) {
                w.show(); w.raise(); w.activateWindow();
            }
        });

        QObject::connect(showAction, &QAction::triggered, [&w]() {
            w.show(); w.raise(); w.activateWindow();
        });

        QObject::connect(settingAction, &QAction::triggered, [&w]() {
            w.showSetting();
        });


        QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

        trayIcon->show();
        trayIcon->showMessage("SmartDesk", "程序已最小化到托盘",
            QIcon::fromTheme("utilities-system-monitor"), 3000);
    }

    return app.exec();
}
