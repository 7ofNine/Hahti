#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

#include "webproxy.h"
#include "autoproxy.h"

#include <QFile>
#include <QList>
#include <QTimer>
#include <QNetworkReply>
#include <QSignalMapper>
#include <QNetworkAccessManager>

#define MAX_NUM_REQUEST     (int) 6     // Maximum paralel download requests.DO NOT ALTER!!!

/*-----------------------------------------------------------------------------------------------*
 * Type defs
 *-----------------------------------------------------------------------------------------------*/
typedef struct {
    QNetworkReply      *pReply;
    QFile              *pFile;
    int                iRetry;
    QTimer             *pTimer;
    QString            *psUsedWebProxy;
    int                iPage;
    QString            sFileFormat;
    QString            sZoomlevel;
    uint               uiRequestTimestamp;
}tDownloadRequest;

enum eDownloadSource{
    eHATHITRUST
};


/*-----------------------------------------------------------------------------------------------*
 * File Downloader Class
 *-----------------------------------------------------------------------------------------------*/
class FileDownloader: public QObject
{
    Q_OBJECT

public:
    FileDownloader(AutoProxy *pAutoProxy = 0 ,QWidget *parent = 0);

    void initFileDownload(void);
    void startDownload(   QString sTargetUrl,
                          QString sTargetPages,
                          QString sDstFolder,
                          int iMaxPageNum,
                          QString sDocumentID,
                          bool bDownloadPdfFiles,
                          bool bEnableWebProxy,
                          bool bEnableAutoProxy,
                          bool bResumeDownload,
                          int iZoomlevel,
                          bool bGetOcr);

    int getNumOfParallelDownloads(void);
    void cancelParallelDownload(void);
    bool getEnableWebProxy(void);
    void enableWebProxy(bool);
    bool getEnableAutoProxy(void);
    void enableAutoProxy(bool);
    void setResumeDownloadEnabled(bool);
    bool getBusyState(void);

    QNetworkAccessManager   *mpNetwManager;
    QList<tDownloadRequest *> mCurrentDownloads;
    WebProxy                *mpWebProxy;
    AutoProxy               *mpAutoProxy;

signals:
   void updateDownloadState(QString);
   void cancelDownload(void);
   void updateDownloadProgressBar(int, int);
   void enableDownloadSettings(bool);
   void updateWebProxyState(QString);
   void updateAutoProxyState(QString);
   void downloadFinished(bool);

private slots:
    void slot_downloadFilesInParallel(int iRequest);
    void slot_newDataReceived(void);
    void slot_updateDownloadProgess(qint64 iBytesReceived, qint64 iBytesTotal);
    void slot_fileDownloadFinished(QNetworkReply* pReply);
    void slot_startTimer();
    void slot_webProxyHandover();
    void slot_elapsedTime(void);
    void slot_clearCookieJar(bool);
    void slot_handleSslErrors(QNetworkReply* pReply, const QList<QSslError> &errors);

private:
    void fileDownloadFinishedHathitrust(QNetworkReply* pReply);
    void downloadOcrText(void);
    void downloadOcrText(tDownloadRequest* apDownloadRequest);
    void ocrPageLoaded(int iRequest);
    int getCurPageNum(void);
    int getTotalPageNum(QString sPages);
    void removeReservedCharFromString(QString *psInput, QString *psOutput = 0);
    QString makePathAndFilename(QString sFileFormat, int iPageNum);

    QWidget                *mpParent;
    QSignalMapper          *mpSignalMapperTimer;
    QTimer                 *mpElapsedTimeUpdateTimer;
    QTimer                 *mpElapsedTimeProxyHandover;
    QTime                  *mpElapsedTime;
    int                    miNumOfParallelDownloads;
    int                    miMaxPageNum;
    int                    miCurPageNum;
    QString                msDocumentID;
    QString                msDstFolder;
    QString                msTargetUrl;
    QString                msPages;
    QStringList*           mpTargetPagesList;
    QStringList*           mpTargetPagesNumList;
    QString                msWaitTime;
    QString                msWaitUntil;
    int                    miWaitTime;
    int                    miElapsedTime;
    int                    miCurPageSection;
    int                    miTotalPageNum;
    bool                   mbDownloadPdfFiles;
    int                    miZoomlevel;
    bool                   mbGetOcr;
    bool                   mbhttpRequestAborted;
    bool                   mbTimerActive;
    bool                   mbResumeDownload;
    bool                   mbDownloadActive;
    uint                   muiTimerActiveTimestamp;
    bool                   mbWebProxyEnableState;
    bool                   mbAutoProxyEnableState;
    bool                   mbAutoProxyToWebProxyHandover;
    bool                   mbDownloadWholeBook;
    eDownloadSource        meDownloadSource;
};

#endif // FILEDOWNLOADER_H
