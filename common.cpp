#include "common.h"

#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>

#if QT_VERSION >= 0x050000
     #include <QStandardPaths>
#else
     #include <QDesktopServices>
#endif


/*-----------------------------------------------------------------------------------------------*
 * Write file: ocr, xml
 *-----------------------------------------------------------------------------------------------*/
int writeFile(QWidget* parent, const QString& sFileName, QString *sContent)
{
    QFileInfo fi(sFileName);

#if defined(Q_OS_WIN)
    QString sPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);;
#else
#if QT_VERSION >= 0x050000
     QString sPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data/organization/application";
#else
     QString sPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
#endif // Q_OS_WIN

     if (fi.isAbsolute())
     {
         sPath = fi.path();
     }

     QDir filePath;
     if (filePath.mkpath(sPath))
     {
        QFile file(sPath + QDir::separator() + fi.fileName());
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream stream( &file );
            stream << *sContent << endl;
            file.close();
            file.deleteLater();
            return 1;
        }
        else
        {
            QMessageBox::information(parent, QObject::tr("Error"),QObject::tr("Unable to save the file %1: %2.").arg(fi.fileName()).arg(file.errorString()));
        }
        file.deleteLater();
    }
    return 0;
}


/*-----------------------------------------------------------------------------------------------*
 * Write file: ocr, xml
 *-----------------------------------------------------------------------------------------------*/
int writeFile(const QString& sFileName, QString *sContent)
{
    QFileInfo fi(sFileName);

#if defined(Q_OS_WIN)
    QString sPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#else
#if QT_VERSION >= 0x050000
     QString sPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data/organization/application";
#else
     QString sPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
#endif // Q_OS_WIN

     if (fi.isAbsolute())
     {
         sPath = fi.path();
     }

     QDir filePath;
     if (filePath.mkpath(sPath))
     {
        QFile file(sPath + QDir::separator() + fi.fileName());
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream stream( &file );
            stream << *sContent << endl;
            file.close();
            file.deleteLater();
            return 1;
        }
        file.deleteLater();
    }
    return 0;
}

/*-----------------------------------------------------------------------------------------------*
 * Read file: ocr, xml
 *-----------------------------------------------------------------------------------------------*/
QString readFile(const QString& sFileName)
{
    QFileInfo fi(sFileName);

#if defined(Q_OS_WIN)
    QString sPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#else
#if QT_VERSION >= 0x050000
    QString sPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data/organization/application";
#else
    QString sPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
#endif // Q_OS_WIN

    if (fi.isAbsolute())
    {
        sPath = fi.path();
    }

    QDir filePath;
    filePath.mkpath(sPath);
    QFile file(sPath + QDir::separator() + fi.fileName());

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        file.deleteLater();
        return stream.readAll();
    }
    file.deleteLater();

    return "";
}

/*-----------------------------------------------------------------------------------------------*
 * Return xml tag value
 *-----------------------------------------------------------------------------------------------*/
QString myXmlReader(QString *sXmlData, QString sTag, int *piStartFrom)
{    
    if (piStartFrom == 0)
    {
        int iStartFrom = 0;
        piStartFrom = &iStartFrom;
    }

    int startPos = sXmlData->indexOf( "<" + sTag+">", *piStartFrom);
    if (startPos < 0)
    {
        *piStartFrom = startPos;
        return QString("");
    }

    startPos += sTag.length()+2;
    int stopPos  = sXmlData->indexOf( "</"+ sTag + ">", *piStartFrom);
    if (stopPos < 0)
    {
        *piStartFrom = stopPos;
        return QString("");
    }

    *piStartFrom = startPos - sTag.length()-1;

    return sXmlData->mid(startPos, stopPos-startPos);
}

/*-----------------------------------------------------------------------------------------------*
 * Return OS name
 *-----------------------------------------------------------------------------------------------*/
QString osName()
{
#if defined(Q_OS_ANDROID)
    return QLatin1String("android");
#elif defined(Q_OS_BLACKBERRY)
    return QLatin1String("blackberry");
#elif defined(Q_OS_IOS)
    return QLatin1String("ios");
#elif defined(Q_OS_MAC)
    return QLatin1String("osx");
#elif defined(Q_OS_WINCE)
    return QLatin1String("wince");
#elif defined(Q_OS_WIN)
    return QLatin1String("windows");
#elif defined(Q_OS_LINUX)
    return QLatin1String("linux");
#elif defined(Q_OS_FREEBSD)
    return QLatin1String("freeBSD");
#elif defined(Q_OS_UNIX)
    return QLatin1String("unix");
#else
    return QLatin1String("unknown");
#endif
}
