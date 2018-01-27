#ifndef MAINMENU_H
#define MAINMENU_H


#include <QMainWindow>
#include "hathidownloadhelper.h"
#include <QMenuBar>
#include <QMenu>
#include <QLabel>
#include <QAction>

class menumainwindow : public QMainWindow
{
    Q_OBJECT

public:
    menumainwindow(QWidget *parent = 0);
    ~menumainwindow();

private:
    QLabel* label1;
    QMenu *bar;

    QAction* showAct;
    QAction* hideAct;
    QAction* exitAct;

};

#endif // MAINMENU_H
