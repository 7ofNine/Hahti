#include "mainmenu.h"

menumainwindow::menumainwindow(QWidget *parent)
    : QMainWindow(parent)
{
    label1= new QLabel("Hello");
    setCentralWidget(label1);
    showAct = new QAction(("&Show"), this);
    connect(showAct, SIGNAL(triggered()),label1, SLOT(show()));
    hideAct = new QAction(("&Hide"), this);
    connect(hideAct, SIGNAL(triggered()),label1, SLOT(hide()));
    exitAct = new QAction(("&Exit"), this);
    connect(exitAct, SIGNAL(triggered()),parent, SLOT(quit()));
    bar = menuBar()->addMenu("&File");
    bar->addAction(showAct);
    bar->addAction(hideAct);
    bar->addSeparator();
    bar->addAction(exitAct);

}

menumainwindow::~menumainwindow()
{

}
