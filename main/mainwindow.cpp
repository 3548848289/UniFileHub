#include "include/mainwindow.h"
#include "ui/ui_mainwindow.h"
#include "../manager/include/dbService.h"
#include "../Setting/include/ThemeManager.h"
#include "../Setting/include/SettingManager.h"
#include "../../PersonalDrive/include/DriveManager.h"
#include <QTimer>

void MainWindow::initCoreWidgets() {
    loginButton = new QPushButton(this);
    loginButton->setFixedSize(26, 26);    
    loginButton->setStyleSheet("border: none; border-radius: 13px;");
    // loginButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::UserAvailable));
    loginButton->setIcon(IconManager::icon(IconManager::Icon::Login, QSize(24,24)));
    loginButton->setIconSize(loginButton->size());

    ui->menubar->setCornerWidget(loginButton, Qt::TopRightCorner);
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    tabWidget->tabBar()->setMovable(true);

    tabManager = new TabManager(tabWidget, this);

    connect(tabWidget, &QTabWidget::customContextMenuRequested, this, &MainWindow::onTabContextMenuRequested);
    file_system = new FileSystem(this);
    file_backup_view = new FileBackupView(this);
    wonlinedoc = new WOnlineDoc(this);
    schedule_wid= new ScheduleWid(this);
    widgetfunc = new WidgetFunctional(this);

    ui->stackedWidget->setObjectName("pWidget");
    ui->stackedWidget->setStyleSheet("QWidget#pWidget { border: 1px solid " + ThemeManager::Instance().borderColor().name() + "; }");
    ui->stackedWidget->addWidget(file_system);
    ui->stackedWidget->addWidget(file_backup_view);
    ui->stackedWidget->addWidget(wonlinedoc);
    ui->stackedWidget->addWidget(schedule_wid);
    ui->stackedWidget->setCurrentWidget(file_system);
}

void MainWindow::initConnect() {
    connect(widgetfunc, &WidgetFunctional::loginStateChanged, this, [this]() {
        auto refreshAvatar = [this]() {
            DInfo *dinfo = widgetfunc->getDInfo();
            if (dinfo != nullptr) {
                QPixmap avatar = dinfo->getStoredAvatar();
                if (!avatar.isNull()) {
                    loginButton->setIcon(QIcon(avatar.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                    loginButton->setIconSize(QSize(15, 15));
                    return;
                }
            }

            loginButton->setIcon(IconManager::icon(IconManager::Icon::Login, QSize(24,24)));
            loginButton->setIconSize(loginButton->size());
        };

        refreshAvatar();
        QTimer::singleShot(300, this, refreshAvatar);
    });

    connect(widgetfunc, &WidgetFunctional::showFiletag, this, [this] {
        togglePanel(file_system);
    });
    connect(widgetfunc, &WidgetFunctional::showFilebackup, this, [this] {
        togglePanel(file_backup_view);
    });
    connect(widgetfunc, &WidgetFunctional::showwOnlinedoc, this, [this] {
        togglePanel(wonlinedoc);
    });
    connect(widgetfunc, &WidgetFunctional::showWSchedule, this, [this] {
        togglePanel(schedule_wid);
    });

    connect(widgetfunc, &WidgetFunctional::sendEmailForm, this, [this](SendEmail *form) {
        tabManager->addWidgetTab(form, "Email");
        form->show();
        ui->stackedWidget->setCurrentWidget(file_system);
    });
    connect(widgetfunc, &WidgetFunctional::showClipboard, this, [this](ClipboardView* clipboard) {
        tabManager->addWidgetTab(clipboard, "剪贴板");
        clipboard->show();
        ui->stackedWidget->setCurrentWidget(file_system);
    });

    connect(widgetfunc, &WidgetFunctional::showDrive, this, [this](DriveView* drive) {
        tabManager->addWidgetTab(drive, "个人网盘");
        drive->show();
        ui->stackedWidget->setCurrentWidget(file_system);
    });

    connect(loginButton, &QPushButton::clicked, this, &MainWindow::showUserInfoDialog);

    // connect(tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
    //     tabManager->closeTab(0, 0, index);  // 使用行列坐标 (0,0) 和 index
    // });


    connect(ui->actionhelp, &QAction::triggered, this, [this]() {
        tabManager->openFile(":/conf/help.txt");
    });

    connect(ui->actiondownload, &QAction::triggered, this, [this]() {
        ui->stackedWidget->setCurrentWidget(wonlinedoc);
        wonlinedoc->setCurrentTabIndex(1);
    });

    connect(ui->actiontxt_file, &QAction::triggered, this, [this]() {
        tabManager->createNewTab([]() { return new TextTab(""); }, "New Text Tab");
    });

    connect(ui->actioncsv_file, &QAction::triggered, this, [this]() {
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
    connect(file_system, &FileSystem::deleteFileRequested, tabManager, &TabManager::deleteFile);
    connect(file_system, &FileSystem::uploadToDriveRequested, this, [this](const QString &filePath) {
        const QFileInfo fileInfo(filePath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            QMessageBox::information(this, tr("提示"), tr("请选择一个本地文件上传到网盘。"));
            return;
        }

        DriveManager::Instance().uploadFile(filePath, 0);
        // QMessageBox::information(this, tr("提示"), tr("已开始上传到网盘根目录。"));
    });
    connect(file_system, &FileSystem::fileSelectedByKeyboard, this, [this](const QString &filePath) {
        // 创建预览标签页
        TabAbstract* previewTab = TabFactory::create(filePath);
        if (previewTab) {
            previewTab->loadFromFile(filePath);
            tabManager->setPreviewTab(previewTab);
        }
    });
    connect(schedule_wid, &ScheduleWid::fileClicked, tabManager, &TabManager::openFile);
    connect(file_backup_view, &FileBackupView::s_fileopen, tabManager, &TabManager::openFile);

    connect(file_system, &FileSystem::tagopened, this, [=]{
        ui->stackedWidget->setCurrentWidget(schedule_wid);
    });
    connect(file_system, &FileSystem::filebackuplistOpened, this, [=]{
        ui->stackedWidget->setCurrentWidget(file_backup_view);
    });


    connect(schedule_wid, &ScheduleWid::openInFileSystemRequested, this, [=](const QString &filePath){
        ui->stackedWidget->setCurrentWidget(file_system);
        QFileInfo fileInfo(filePath);
        QString directoryPath = fileInfo.path();
        file_system->changePath(directoryPath);
    });

    connect(file_backup_view, &FileBackupView::openInFileSystemRequested, this, [=](const QString &filePath){
        ui->stackedWidget->setCurrentWidget(file_system);
        QFileInfo fileInfo(filePath);
        QString directoryPath = fileInfo.path();
        file_system->changePath(directoryPath);
    });

    connect(tabManager, &TabManager::openInFileSystemRequested, this, [=](const QString &filePath){
        ui->stackedWidget->setCurrentWidget(file_system);
        QFileInfo fileInfo(filePath);
        QString directoryPath = fileInfo.path();
        file_system->changePath(directoryPath);
    });
}

void MainWindow::initMemubarLayout() {
    QSplitter *horizontalSplitter = new QSplitter(Qt::Horizontal);    
    horizontalSplitter->addWidget(widgetfunc);
    widgetfunc->setFixedWidth(60);

    horizontalSplitter->addWidget(ui->combinedWidget);
    ui->combinedWidget->setMaximumWidth(450);

    // 添加tabManager的容器部件，并隐藏原始tabWidget
    QWidget* containerWidget = tabManager->getContainerWidget();
    horizontalSplitter->addWidget(containerWidget);
    
    // 隐藏原始的tabWidget，避免重叠显示
    if (tabWidget) {
        tabWidget->setVisible(false);
    }
    
    horizontalSplitter->setStretchFactor(0, 0);
    horizontalSplitter->setStretchFactor(1, 1);
    horizontalSplitter->setStretchFactor(2, 20);
    setCentralWidget(horizontalSplitter);
    QList<int> sizes = {60, 340, 600};
    horizontalSplitter->setSizes(sizes);
    
    // 添加布局菜单选项
    QMenu* layoutMenu = new QMenu("布局", this);
    
    QAction* layout1x1 = new QAction("单视图 (1x1)", this);
    connect(layout1x1, &QAction::triggered, this, [this]() {
        tabManager->setLayoutType(LayoutType::LAYOUT_1X1);
    });
    
    QAction* layout1x2 = new QAction("水平分割 (1x2)", this);
    connect(layout1x2, &QAction::triggered, this, [this]() {
        tabManager->setLayoutType(LayoutType::LAYOUT_1X2);
    });
    
    QAction* layout2x1 = new QAction("垂直分割 (2x1)", this);
    connect(layout2x1, &QAction::triggered, this, [this]() {
        tabManager->setLayoutType(LayoutType::LAYOUT_2X1);
    });
    
    QAction* layout2x2 = new QAction("网格视图 (2x2)", this);
    connect(layout2x2, &QAction::triggered, this, [this]() {
        tabManager->setLayoutType(LayoutType::LAYOUT_2X2);
    });
    
    layoutMenu->addAction(layout1x1);
    layoutMenu->addAction(layout1x2);
    layoutMenu->addAction(layout2x1);
    layoutMenu->addAction(layout2x2);
    
    ui->menu_2->addMenu(layoutMenu); // 添加到"视图"菜单下


    const QMap<int, QString> buttonNames = {
        {1, "文件标签"}, {2, "文件备份"}, {3, "备忘日程"}, {4, "在线文档"},
        {6, "个人网盘"}, {7, "邮件服务"}, {8, "剪切字板"}, {9, "用户登录"}, {10, "更多功能"}
    };
    connect(widgetfunc, &WidgetFunctional::buttonVisibilityChanged,
            this, [this, buttonNames](int buttonIndex, bool isVisible) {
                QAction *action = findChild<QAction*>(QString("Function%1").arg(buttonIndex));
                if (action && buttonNames.contains(buttonIndex)) {
                    const QString &name = buttonNames.value(buttonIndex);
                    action->setText((isVisible ? "关闭" : "打开") + name);
                }
            });

    for (int i = 1; i <= 10; ++i) {
        if (i == 5) continue; // 跳过手写绘图按钮
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
    this->setWindowTitle("UniFileHub");
    initCoreWidgets();
    initConnect();
    initMemubarLayout();
    
    // 加载窗口大小和位置
    QSize size = SettingManager::Instance().getWindowSize();
    QPoint pos = SettingManager::Instance().getWindowPosition();
    this->resize(size);
    this->move(pos);
    
    // 恢复窗口最大化状态
    if (SettingManager::Instance().getWindowMaximized()) {
        this->showMaximized();
    }
    
    // 设置菜单图标
    ui->actionopen->setIcon(IconManager::icon(IconManager::Icon::MenuFileOpen, QSize(16, 16)));
    ui->actionsave->setIcon(IconManager::icon(IconManager::Icon::MenuFileSave, QSize(16, 16)));
    ui->actionclose->setIcon(IconManager::icon(IconManager::Icon::MenuFileClose, QSize(16, 16)));
    ui->actiondownload->setIcon(IconManager::icon(IconManager::Icon::MenuDownload, QSize(16, 16)));
    ui->actionshe->setIcon(IconManager::icon(IconManager::Icon::MenuSettings, QSize(16, 16)));
    ui->actionhelp->setIcon(IconManager::icon(IconManager::Icon::MenuHelp, QSize(16, 16)));
    ui->actionfind->setIcon(IconManager::icon(IconManager::Icon::MenuSearch, QSize(16, 16)));
    
    tabManager->openFile(":/conf/help.txt");
    recentFilesManager->populateRecentFilesMenu(ui->recentFile);
    widgetfunc->tryRestoreLogin();

    // 默认关闭在线文档
    QAction *action = findChild<QAction*>("Function4");
    if (action) {
        action->trigger();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 保存窗口大小、位置和状态
    SettingManager::Instance().setWindowSize(this->size());
    SettingManager::Instance().setWindowPosition(this->pos());
    SettingManager::Instance().setWindowMaximized(this->isMaximized());
    event->accept();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::togglePanel(QWidget* target) {
    if (ui->stackedWidget->currentWidget() == target && ui->stackedWidget->isVisible()) {
        ui->stackedWidget->hide();
        QList<int> sizes = {60, 0, 900}; // 第二块设成 0 宽度
        qobject_cast<QSplitter*>(centralWidget())->setSizes(sizes);
    } else {
        ui->stackedWidget->show();
        ui->stackedWidget->setCurrentWidget(target);
        QList<int> sizes = {60, 340, 600}; // 恢复
        qobject_cast<QSplitter*>(centralWidget())->setSizes(sizes);
    }
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
            widgetfunc->on_pushButton_9_clicked();
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
        tabManager->closeTab(0, 0, currentIndex);
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
    QAction *uploadToDrive = menu.addAction("上传到网盘");

    QAction *selectedAction = menu.exec(tabWidget->tabBar()->mapToGlobal(pos));
    if (!selectedAction) return;

    if (selectedAction == closeCurrent) {
        tabManager->closeTab(0, 0, tabIndex);  // 用点击的 tabIndex
    } else if (selectedAction == closeOthers) {
        for (int i = tabWidget->count() - 1; i >= 0; --i) {
            if (i != tabIndex)
                tabManager->closeTab(0, 0, i);
        }
    } else if (selectedAction == closeAll) {
        for (int i = tabWidget->count() - 1; i >= 0; --i) {
            tabManager->closeTab(0, 0, i);
        }
    } else if (selectedAction == uploadToDrive) {
        QWidget *widget = tabWidget->widget(tabIndex);
        auto *tab = qobject_cast<TabAbstract *>(widget);
        if (!tab) {
            QMessageBox::information(this, tr("提示"), tr("当前页签不是本地文件，无法上传到网盘。"));
            return;
        }

        const QString filePath = tab->getCurrentFilePath();
        if (filePath.isEmpty()) {
            QMessageBox::information(this, tr("提示"), tr("当前页签还没有保存成文件，请先保存后再上传到网盘。"));
            return;
        }

        const QFileInfo fileInfo(filePath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            QMessageBox::information(this, tr("提示"), tr("当前页签对应的本地文件不存在，无法上传到网盘。"));
            return;
        }

        DriveManager::Instance().uploadFile(filePath, 0);
        // QMessageBox::information(this, tr("提示"), tr("已开始上传到网盘根目录。"));
    }

}

void MainWindow::on_actionfind_triggered()
{
    if (tabManager) {
        tabManager->findInCurrentTab(this); // this 作为 parent，保证对话框弹在主窗口上
    }
}

void MainWindow::openFileFromCommandLine(const QString& filePath)
{
    if (tabManager && QFile::exists(filePath)) {
        tabManager->openFile(filePath);
    }
}

void MainWindow::on_actionTerminal_triggered()
{
    Q_INIT_RESOURCE(KodoTermThemes);
    
    auto *terminal = new KodoTerm(this);
    
#ifdef Q_OS_WIN
    QString terminalType = SettingManager::Instance().terminal_type();
    terminal->setProgram(terminalType == "powershell" ? "powershell.exe" : "cmd.exe");
#else
    terminal->setProgram("/bin/bash");
#endif
    
    // 从 SettingManager 获取配置
    QString fontFamily = SettingManager::Instance().terminal_font_family();
    int fontSize = SettingManager::Instance().terminal_font_size();
    QString themeName = SettingManager::Instance().terminal_theme();
    
    // 设置字体
    QFont font(fontFamily, fontSize);
    font.setStyleHint(QFont::Monospace);
    terminal->setFont(font);
    
    // 设置主题
    if (themeName == "Default") {
        terminal->setTheme(TerminalTheme::defaultTheme());
    } else {
        // 查找并加载选中的主题
        auto themes = TerminalTheme::builtInThemes();
        bool themeFound = false;
        for (const auto &info : themes) {
            if (info.name == themeName) {
                terminal->setTheme(TerminalTheme::loadTheme(info.path));
                themeFound = true;
                break;
            }
        }
        if (!themeFound) {
            terminal->setTheme(TerminalTheme::defaultTheme());
        }
    }
    
    tabManager->addWidgetTab(terminal, "终端");
    terminal->show();
    terminal->start();
}

