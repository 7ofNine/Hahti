#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QWidget>

// Global settings
#define DISABLE_AUTOPROXY   0   ///> Disable auto proxy feature
#define DISABLE_WEBPROXY    1   ///> Disable web proxy feature
#define USE_MENUS_IN_TABS   0   ///> TODO alternative GUI with tabs instead of dialogs:  unfinished

typedef struct
{
    QString sStyle;
    QString sFontFamily;
    int     iFontPointSize;
    bool    bFontItalic;
    bool    bFontBold;
    bool    bFontStrikeout;
    bool    bFontUnderline;
}tGuiSettings;

typedef struct
{
    bool    bAutoUpdate;

    bool    bUseProxy;

    bool    bDownloadPdfs;
    bool    bDownloadImages;
    bool    bDownloadOcr;
    bool    bDownloadWholeBook;
    int     iZoom;
    QString sDestinationFolder;
    bool    bCreateDownloadFolder;
    bool    bCreateDownloadFolderByTitle;
    int     iTitleChars;
    bool    bAutoPdfBook;
    bool    bEnableWebProxy;
    bool    bEnableAutoProxy;
    bool    bEnableAutoProxyActivation;
    bool    bResume;

    bool    bMergePdf;
    bool    bImage2Book;
    bool    bImage2Pdf;
    bool    bOcrOnly;
    bool    bKeepExistingFile;
    bool    bDeleteDownloadedPageData;

    bool    bWebProxyNoteAccepted;
    QString sWhatsNewVersion;
}tUserSettings;

int writeFile (QWidget* parent, const QString& sFilename, QString *sContent);
int writeFile(const QString& sFileName, QString *sContent);
QString readFile (const QString& sFilename);
QString myXmlReader(QString *sXmlData, QString sTag, int *piStartFrom = 0);
QString osName();

#endif // COMMON_H
