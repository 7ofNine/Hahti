#-------------------------------------------------
#
# Project created by QtCreator 2013-04-14T15:18:44
#
#-------------------------------------------------

QT       += core gui
QT       += network
QT       += sql
lessThan(QT_MAJOR_VERSION, 5):    QT += webkit
greaterThan(QT_MAJOR_VERSION, 4): QT += webkitwidgets widgets printsupport

TARGET = HathiDownloadHelper
TEMPLATE = app

SOURCES +=  main.cpp\
            hathidownloadhelper.cpp \
            proxydialog.cpp \
            fileselectdialog.cpp \
            createpdfworker.cpp \
            common.cpp \
            webproxy.cpp \
            filedownloader.cpp \
            autoproxy.cpp \
            createbatchjob.cpp \
            hathidownloadhelperGui.cpp

HEADERS  += hathidownloadhelper.h \
            proxydialog.h \
            fileselectdialog.h \
            createpdfworker.h \
            common.h \
            webproxy.h \
            filedownloader.h \
            autoproxy.h \
            createbatchjob.h

# ALL FORMS HAVE BEEN REPLACE BY HARD CODED ELEMENTS
# FORMS    += \

RESOURCES += \
    images.qrc


# Set application icon
win32 {
     win32:RC_FILE += icon.rc
 }
win64 {
     win64:RC_FILE += icon.rc
 }
macx {
    ICON = elephant.icns
}

# Debug output only in debug mode
CONFIG(release, debug|release):{DEFINES += QT_NO_WARNING_OUTPUT QT_NO_DEBUG_OUTPUT}


