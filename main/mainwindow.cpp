#include "include/mainwindow.h"
#include "ui/ui_mainwindow.h"
#include "../manager/include/dbService.h"

void MainWindow::initCoreWidgets() {
    loginButton = new QPushButton(this);
    loginButton->setFixedSize(26, 26);
    loginButton->setStyleSheet("border: none; border-radius: 13px;");
    loginButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::UserAvailable));
    loginButton->setIconSize(loginButton->size());

    ui->menubar->setCornerWidget(loginButton, Qt::TopRightCorner);
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    tabManager = new TabManager(tabWidget, this);

    connect(tabWidget, &QTabWidget::customContextMenuRequested, this, &MainWindow::onTabContextMenuRequested);
    file_system = new FileSystem(this);
    file_backup_view = new FileBackupView(this);
    wonlinedoc = new WOnlineDoc(this);
    schedule_wid= new ScheduleWid(this);
    widgetfunc = new WidgetFunctional(this);

    ui->stackedWidget->setObjectName("pWidget");
    ui->stackedWidget->setStyleSheet("QWidget#pWidget { border: 1px solid #808080; }");
    ui->stackedWidget->addWidget(file_system);
    ui->stackedWidget->addWidget(file_backup_view);
    ui->stackedWidget->addWidget(wonlinedoc);
    ui->stackedWidget->addWidget(schedule_wid);
    ui->stackedWidget->setCurrentWidget(file_system);
}

void MainWindow::initConnect() {
    connect(widgetfunc, &WidgetFunctional::showFiletag, this, [this] {
        ui->stackedWidget->setCurrentWidget(file_system); });

    connect(widgetfunc, &WidgetFunctional::showFilebackup, this, [this] {
        ui->stackedWidget->setCurrentWidget(file_backup_view); });

    connect(widgetfunc, &WidgetFunctional::showwOnlinedoc, this, [this] {
        ui->stackedWidget->setCurrentWidget(wonlinedoc); });

    connect(widgetfunc, &WidgetFunctional::showWSchedule, this, [this] {
        ui->stackedWidget->setCurrentWidget(schedule_wid); });

    connect(widgetfunc, &WidgetFunctional::sendEmailForm, this, [this](SendEmail *form) {
        int newIndex = tabManager->addWidgetTab(form, "Email"); // 通过 TabManager 添加
        form->show();
        tabWidget->setCurrentIndex(newIndex);
        ui->stackedWidget->setCurrentWidget(file_system);
    });
    connect(widgetfunc, &WidgetFunctional::showClipboard, this, [this](ClipboardView* clipboard) {
        int newIndex = tabManager->addWidgetTab(clipboard, "剪贴板"); // 通过 TabManager 添加
        clipboard->show();
        tabWidget->setCurrentIndex(newIndex);
        ui->stackedWidget->setCurrentWidget(file_system);
    });

    connect(widgetfunc, &WidgetFunctional::showDraw, this, [=] {
        QWidget *currentWidget = tabWidget->currentWidget();
        TabHandleIMG *tabHandleImg = qobject_cast<TabHandleIMG*>(currentWidget);
        if (tabHandleImg) {
            tabHandleImg->test();
        } else {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(this, "无效的文件类型",
                                         "当前文件不是图片文件，是不是选取图片打开。", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes)
                on_actionopen_triggered();
            else
                qDebug() << "用户取消打开图片操作";
        }
    });


    connect(loginButton, &QPushButton::clicked, this, &MainWindow::showUserInfoDialog);

    // connect(tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
    //     tabManager->closeTab(index);  // 只用 index，不用 currentIndex
    // });


    connect(ui->actionhelp, &QAction::triggered, this, [this]() {
        tabManager->openFile(":/conf/help.txt");
    });

    connect(ui->actiondownload, &QAction::triggered, this, [this]() {
        ui->stackedWidget->setCurrentWidget(wonlinedoc);
    });

    connect(ui->actiontxt_file, &QAction::triggered, this, [this]() {
        tabManager->createNewTab([]() { return new TextTab(""); }, "New Text Tab");
    });

    connect(ui->actionscv_file, &QAction::triggered, this, [this]() {
        tabManager->createNewTab([]() { return new TabHandleCSV(""); }, "New CSV Tab");
    });

    connect(ui->actionxlsx_file, &QAction::triggered, this, [this]() {
        tabManager->createNewTab([]() { return new TabHandleXLSX(""); }, "New XLSX Tab");
    });

    connect(ui->actionshe, &QAction::triggered, this, &MainWindow::showSetting);

    connect(ui->actionh1, &QAction::triggered, this, [this]() {
        int currentIndex = tabWidget->currentIndex();
        QWidget *widget = tabWidget->widget(currentIndex);
        if (dynamic_cast<TabAbstract*>(widget)) {
            TabAbstract *tab = qobject_cast<TabAbstract*>(widget);
            //to do
            // tab->showControl();
        }
    });

    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        currentIndex = index;
    });

    connect(wonlinedoc->shared_view, &SharedView::filePathSent, this, &MainWindow::handleFilePathSent);
    connect(wonlinedoc->download_view, &DownloadView::fileDownloaded, this, &MainWindow::handleFileDownload);

    connect(recentFilesManager, &RecentFilesManager::fileOpened, tabManager, &TabManager::openFile);
    connect(tabManager, &TabManager::fileOpened,recentFilesManager, &RecentFilesManager::addFile);


    connect(file_system, &FileSystem::fileOpened, tabManager, &TabManager::openFile);
    connect(schedule_wid, &ScheduleWid::fileClicked, tabManager, &TabManager::openFile);
    connect(file_backup_view, &FileBackupView::s_fileopen, tabManager, &TabManager::openFile);

    connect(file_system, &FileSystem::tagopened, this, [=]{
        ui->stackedWidget->setCurrentWidget(schedule_wid);
    });
    connect(file_system, &FileSystem::filebackuplistOpened, this, [=]{
        ui->stackedWidget->setCurrentWidget(file_backup_view);
    });
}

void MainWindow::initMemubarLayout() {
    QSplitter *horizontalSplitter = new QSplitter(Qt::Horizontal);
    horizontalSplitter->addWidget(widgetfunc);
    horizontalSplitter->addWidget(tabWidget);
    horizontalSplitter->addWidget(ui->combinedWidget);
    horizontalSplitter->setStretchFactor(0, 0);
    horizontalSplitter->setStretchFactor(1, 1);
    horizontalSplitter->setStretchFactor(2, 3);
    setCentralWidget(horizontalSplitter);
    QList<int> sizes = {60, 550, 390};
    horizontalSplitter->setSizes(sizes);


    const QMap<int, QString> buttonNames = {
        {1, "文件标签"}, {2, "文件备份"}, {3, "备忘日程"}, {4, "在线文档"}, {5, "手写绘图"},
        {6, "邮件服务"}, {7, "剪切字板"}, {8, "用户登录"}, {9, "更多功能"}
    };
    connect(widgetfunc, &WidgetFunctional::buttonVisibilityChanged,
            this, [this, buttonNames](int buttonIndex, bool isVisible) {
                QAction *action = findChild<QAction*>(QString("Function%1").arg(buttonIndex));
                if (action && buttonNames.contains(buttonIndex)) {
                    const QString &name = buttonNames.value(buttonIndex);
                    action->setText((isVisible ? "关闭" : "打开") + name);
                }
            });

    for (int i = 1; i <= 9; ++i) {
        QAction *action = findChild<QAction *>(QString("Function%1").arg(i));
        if (action) {
            connect(action, &QAction::triggered, this, [this, i]() {
                widgetfunc->toggleButtonVisibility(i);
            });
        }
    }

}


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow),
    recentFilesManager(new RecentFilesManager(this))
{
    ui->setupUi(this);
    initCoreWidgets();
    initConnect();
    initMemubarLayout();
    tabManager->openFile(":/conf/help.txt");
    recentFilesManager->populateRecentFilesMenu(ui->recentFile);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showUserInfoDialog() {
    DInfo *dinfo = widgetfunc->getDInfo();
    if (dinfo != nullptr) {
        QPixmap avatar = dinfo->getStoredAvatar();
        if (!avatar.isNull()) {
            loginButton->setIcon(QIcon(avatar.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            loginButton->setIconSize(QSize(15, 15));
        }
        dinfo->exec();
    } else {
        QMessageBox::StandardButton reply =
            QMessageBox::question(this, "未登录", "您尚未登录，是否现在登录？",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
        );
        if (reply == QMessageBox::Yes)
            widgetfunc->on_pushButton_8_clicked();
    }
}

void MainWindow::showSetting()
{
    Setting *setting = new Setting();
    setting->show();
}



void MainWindow::on_actionopen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
        tr("All Files (*);;CSV Files (*.csv);;Text Files (*.txt)"));
    if (fileName.isEmpty())
        return;
    tabManager->openFile(fileName);
}

void MainWindow::on_actionsave_triggered()
{
    tabManager->saveCurrentTab(this);
}

void MainWindow::on_actionclose_triggered()
{
    int currentIndex = tabWidget->currentIndex();
    if (currentIndex >= 0) {
        tabManager->closeTab(currentIndex);
    }
}

void MainWindow::handleFileDownload(const QString &fileName, const QByteArray &fileContent)
{
    tabManager->openFileFromMemory(fileName, fileContent);
}

void MainWindow::handleFilePathSent()
{
    tabManager->openSharedCSVTab(wonlinedoc->shared_view);
}

void MainWindow::onTabContextMenuRequested(const QPoint &pos) {
    int tabIndex = tabWidget->tabBar()->tabAt(pos);
    if (tabIndex == -1) return; // 点击空白处无效

    QMenu menu;
    QAction *closeCurrent = menu.addAction("关闭当前");
    QAction *closeOthers = menu.addAction("关闭其他");
    QAction *closeAll = menu.addAction("关闭全部");

    QAction *selectedAction = menu.exec(tabWidget->tabBar()->mapToGlobal(pos));
    if (!selectedAction) return;

    if (selectedAction == closeCurrent) {
        tabManager->closeTab(tabIndex);  // 用点击的 tabIndex
    } else if (selectedAction == closeOthers) {
        for (int i = tabWidget->count() - 1; i >= 0; --i) {
            if (i != tabIndex)
                tabManager->closeTab(i);
        }
    } else if (selectedAction == closeAll) {
        for (int i = tabWidget->count() - 1; i >= 0; --i) {
            tabManager->closeTab(i);
        }
    }

}

