/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2018  Miodrag Milanovic <miodrag@symbioticeda.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <QAction>
#include <QCoreApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QIcon>
#include <QSplitter>
#include "designwidget.h"
#include "fpgaviewwidget.h"
#include "jsonparse.h"
#include "log.h"
#include "mainwindow.h"
#include "pythontab.h"

static void initBasenameResource() { Q_INIT_RESOURCE(base); }

NEXTPNR_NAMESPACE_BEGIN

BaseMainWindow::BaseMainWindow(std::unique_ptr<Context> context, QWidget *parent)
        : QMainWindow(parent), ctx(std::move(context))
{
    initBasenameResource();
    qRegisterMetaType<std::string>();

    log_files.clear();
    log_streams.clear();

    setObjectName("BaseMainWindow");
    resize(1024, 768);

    createMenusAndBars();

    QWidget *centralWidget = new QWidget(this);

    QGridLayout *gridLayout = new QGridLayout(centralWidget);
    gridLayout->setSpacing(6);
    gridLayout->setContentsMargins(11, 11, 11, 11);

    QSplitter *splitter_h = new QSplitter(Qt::Horizontal, centralWidget);
    QSplitter *splitter_v = new QSplitter(Qt::Vertical, splitter_h);
    splitter_h->addWidget(splitter_v);

    gridLayout->addWidget(splitter_h, 0, 0, 1, 1);

    setCentralWidget(centralWidget);

    designview = new DesignWidget();
    designview->setMinimumWidth(300);
    splitter_h->addWidget(designview);

    tabWidget = new QTabWidget();

    console = new PythonTab();
    tabWidget->addTab(console, "Console");
    connect(this, SIGNAL(contextChanged(Context *)), console, SLOT(newContext(Context *)));

    centralTabWidget = new QTabWidget();
    FPGAViewWidget *fpgaView = new FPGAViewWidget();
    centralTabWidget->addTab(fpgaView, "Graphics");

    connect(this, SIGNAL(contextChanged(Context *)), fpgaView, SLOT(newContext(Context *)));
    connect(designview, SIGNAL(selected(std::vector<DecalXY>)), fpgaView,
            SLOT(onSelectedArchItem(std::vector<DecalXY>)));

    connect(designview, SIGNAL(highlight(std::vector<DecalXY>, int)), fpgaView,
            SLOT(onHighlightGroupChanged(std::vector<DecalXY>, int)));

    connect(this, SIGNAL(contextChanged(Context *)), designview, SLOT(newContext(Context *)));
    connect(this, SIGNAL(updateTreeView()), designview, SLOT(updateTree()));

    connect(designview, SIGNAL(info(std::string)), this, SLOT(writeInfo(std::string)));

    splitter_v->addWidget(centralTabWidget);
    splitter_v->addWidget(tabWidget);
    displaySplash();
}

BaseMainWindow::~BaseMainWindow() {}

void BaseMainWindow::displaySplash()
{
    splash = new QSplashScreen();
    splash->setPixmap(QPixmap(":/icons/resources/splash.png"));
    splash->show();
    connect(designview, SIGNAL(finishContextLoad()), splash, SLOT(close()));
    connect(designview, SIGNAL(contextLoadStatus(std::string)), this, SLOT(displaySplashMessage(std::string)));
    QCoreApplication::instance()->processEvents();
}

void BaseMainWindow::displaySplashMessage(std::string msg)
{
    splash->showMessage(msg.c_str(), Qt::AlignCenter | Qt::AlignBottom, Qt::white);
    QCoreApplication::instance()->processEvents();
}

void BaseMainWindow::writeInfo(std::string text) { console->info(text); }

void BaseMainWindow::createMenusAndBars()
{
    actionNew = new QAction("New", this);
    actionNew->setIcon(QIcon(":/icons/resources/new.png"));
    actionNew->setShortcuts(QKeySequence::New);
    actionNew->setStatusTip("New project file");
    connect(actionNew, SIGNAL(triggered()), this, SLOT(new_proj()));

    actionOpen = new QAction("Open", this);
    actionOpen->setIcon(QIcon(":/icons/resources/open.png"));
    actionOpen->setShortcuts(QKeySequence::Open);
    actionOpen->setStatusTip("Open an existing project file");
    connect(actionOpen, SIGNAL(triggered()), this, SLOT(open_proj()));

    QAction *actionSave = new QAction("Save", this);
    actionSave->setIcon(QIcon(":/icons/resources/save.png"));
    actionSave->setShortcuts(QKeySequence::Save);
    actionSave->setStatusTip("Save existing project to disk");
    actionSave->setEnabled(false);
    connect(actionSave, SIGNAL(triggered()), this, SLOT(save_proj()));

    QAction *actionExit = new QAction("Exit", this);
    actionExit->setIcon(QIcon(":/icons/resources/exit.png"));
    actionExit->setShortcuts(QKeySequence::Quit);
    actionExit->setStatusTip("Exit the application");
    connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

    QAction *actionAbout = new QAction("About", this);

    menuBar = new QMenuBar();
    menuBar->setGeometry(QRect(0, 0, 1024, 27));
    QMenu *menu_File = new QMenu("&File", menuBar);
    QMenu *menu_Help = new QMenu("&Help", menuBar);
    menuBar->addAction(menu_File->menuAction());
    menuBar->addAction(menu_Help->menuAction());
    setMenuBar(menuBar);

    mainToolBar = new QToolBar();
    addToolBar(Qt::TopToolBarArea, mainToolBar);

    statusBar = new QStatusBar();
    progressBar = new QProgressBar(statusBar);
    progressBar->setAlignment(Qt::AlignRight);
    progressBar->setMaximumSize(180, 19);
    statusBar->addPermanentWidget(progressBar);
    progressBar->setValue(0);
    progressBar->setEnabled(false);
    setStatusBar(statusBar);

    menu_File->addAction(actionNew);
    menu_File->addAction(actionOpen);
    menu_File->addAction(actionSave);
    menu_File->addSeparator();
    menu_File->addAction(actionExit);
    menu_Help->addAction(actionAbout);

    mainToolBar->addAction(actionNew);
    mainToolBar->addAction(actionOpen);
    mainToolBar->addAction(actionSave);
}

NEXTPNR_NAMESPACE_END
