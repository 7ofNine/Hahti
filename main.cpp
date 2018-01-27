
#if QT_VERSION >= 0x050000
    #include <QtWidgets>
#else
   #include <QtGui>
#endif

#include <QtGlobal>
#include <QMainWindow>
#include <QApplication>

#include "hathidownloadhelper.h"


int main(int argc, char *argv[])
{
#if QT_VERSION >= 0x050100
    qunsetenv("OPENSSL_CONF");
#endif
    QApplication a(argc, argv);
    a.setApplicationName("Hathi Download Helper");
    a.setApplicationVersion("1.1.5");
    a.setWindowIcon(QIcon("qrc:/images/elephant.ico"));
    a.setWindowIcon(QIcon("elephant.icns"));
    HathiDownloadHelper w;
    w.setWindowTitle(a.applicationName() + " " + a.applicationVersion());
    w.show();

    return a.exec();
}
