#include "filedownloader.h"
#include "common.h"

#include <QDir>
#include <QMessageBox>
#include <QDateTime>
#include <QNetworkCookieJar>
#include <QSslError>

#include <QString>
#include <QDebug>

/* TODOS ----------------------------------------------------------------------------------------*/
//          - check handover between autoproxy and webproxy

#define HTTP_PIPELINIG_ALLOWED          true    //has to be true. Workarround to suppress warning "QNetworkAccessCache::addEntry: overriding active cache entry". Might be related to QTBUG-14850
#define DOWNLOAD_REQUEST_IS_FINISHED    (-1)
#define DOWNLOAD_REQUEST_RESUMED        (-2)
#define MINIMUM_FILESIZE                50 // BYTE
#define DOWNLOAD_MAX_NUM_RETRIES        50

#define WEBPROXY_HANGOVER_DELAY_CYCLES   15
#define WEBPROXY_HANGOVER_DELAY          1000

#define connect(src, sig, trgt, slt) connect(src, sig, trgt, slt, Qt::UniqueConnection)

/*-----------------------------------------------------------------------------------------------*
 * Constructor file downloader
 *-----------------------------------------------------------------------------------------------*/
FileDownloader::FileDownloader(AutoProxy *pAutoProxy, QWidget *parent):
    mpAutoProxy(pAutoProxy),
    mpParent(parent)
{
    initFileDownload();
}

/*-----------------------------------------------------------------------------------------------*
 * Init file downloader
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::initFileDownload()
{
    mpNetwManager = new QNetworkAccessManager(this);
    connect(mpNetwManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_fileDownloadFinished(QNetworkReply*)));
    connect(mpNetwManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(slot_handleSslErrors(QNetworkReply*,QList<QSslError>)));

    mpSignalMapperTimer   = new QSignalMapper(this);

    // Number of parallel downloads:  AT MAXIMUM 6 DUE TO QNetworkAccessManager LIMITATIONS!!!!
    miNumOfParallelDownloads = MAX_NUM_REQUEST;

    if (miNumOfParallelDownloads > MAX_NUM_REQUEST)
        miNumOfParallelDownloads = MAX_NUM_REQUEST;

    for (int i = 0; i < miNumOfParallelDownloads; i++)
    {
        tDownloadRequest *pPageRequest = new tDownloadRequest();
        pPageRequest->pTimer = new QTimer();
        pPageRequest->pTimer->setSingleShot(true);

        connect(pPageRequest->pTimer, SIGNAL(timeout()), mpSignalMapperTimer, SLOT(map()));
        mpSignalMapperTimer->setMapping(pPageRequest->pTimer, i);

        pPageRequest->psUsedWebProxy = new QString("");

        mCurrentDownloads.append(pPageRequest);
    }

    connect(mpSignalMapperTimer, SIGNAL(mapped(int)), this, SLOT(slot_downloadFilesInParallel(int)));

    // WebProxy
    mpWebProxy = new WebProxy(mpNetwManager);
    connect(mpWebProxy, SIGNAL(updateWebProxyState(QString)), this, SIGNAL(updateWebProxyState(QString)));
    connect(mpNetwManager, SIGNAL(finished(QNetworkReply*)), mpWebProxy,SLOT(slot_updateDownloadCounter(QNetworkReply*)));
    mpWebProxy->setNumParallelDownloads(miNumOfParallelDownloads);

    // AutoProxy
    connect(mpNetwManager, SIGNAL(finished(QNetworkReply*)), mpAutoProxy,SLOT(slot_updateDownloadCounter(QNetworkReply*)));
    connect(mpAutoProxy, SIGNAL(finished(bool)), this, SLOT(slot_clearCookieJar(bool)));

    mpElapsedTimeUpdateTimer = new QTimer(this);
    mpElapsedTimeUpdateTimer->setSingleShot(true);
    connect(mpElapsedTimeUpdateTimer , SIGNAL(timeout()), this, SLOT(slot_elapsedTime()));

    mpElapsedTimeProxyHandover = new QTimer(this);
    mpElapsedTimeProxyHandover->setSingleShot(true);
    connect(mpElapsedTimeProxyHandover , SIGNAL(timeout()), this, SLOT(slot_webProxyHandover()));

    mpElapsedTime = new QTime;

    msWaitUntil     = "none";
    msWaitTime      = "";
    msDocumentID    = "";
    msDstFolder     = "";
    msPages         = "";
    miWaitTime      = 0;
    miElapsedTime   = 0;
    miMaxPageNum    = 0;
    miCurPageNum    = 0;
    miCurPageSection= 0;
    miTotalPageNum  = 0;
    mbTimerActive   = false;
    mbResumeDownload        = false;
    mbWebProxyEnableState   = false;
    mbAutoProxyToWebProxyHandover = false;
    mbDownloadWholeBook = false;
    mbDownloadActive    = false;
    meDownloadSource    = eHATHITRUST;
}

/*-----------------------------------------------------------------------------------------------*
 * Call timer for download retry
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::slot_startTimer()
{
    mpElapsedTimeUpdateTimer->start((int) 1000);   // Update gui timer for elapsed time
    miElapsedTime += 1000;
}

/*-----------------------------------------------------------------------------------------------*
 * Call webProxy enable
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::slot_webProxyHandover()
{
    this->enableWebProxy(true);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Update gui label showing elapsed time since server timeout
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::slot_elapsedTime()
{
    int iElapsedTime = mpElapsedTime->elapsed();

    int iMinutes = (iElapsedTime)/(1000*60);
    int iSeconds = (iElapsedTime-(iMinutes*(1000*60)))/1000;
    emit updateDownloadState(QString("Server: maximum download limit exceeded...please wait.... [%1:%2 of appr. %3 min]").arg(iMinutes,2,10, QChar('0')).arg(iSeconds,2, 10, QChar('0')).arg(msWaitTime));
    mpElapsedTimeUpdateTimer->start((int) 1000);
}

/*-----------------------------------------------------------------------------------------------*
 * Handle Ssl errors on book info request
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::slot_handleSslErrors(QNetworkReply* pReply, const QList<QSslError> &errors)
{
    qDebug() << "DOWNLOADER: SSL ERROR DETECTED";
    foreach (QSslError e, errors)
    {
        qDebug() << "SSL ERROR: " << e;
    }
    pReply->ignoreSslErrors();
}

/*-----------------------------------------------------------------------------------------------*
 * Cancel download
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::cancelParallelDownload(void)
{
    qDebug() << "DOWNLOADER: CANCELING DOWNLOAD";

    // Cancel Webproxy request
    mpWebProxy->cancelRequest();
    mpAutoProxy->abortAutoProxyRequest();

    // cancel file download
    mbDownloadActive     = false;
    mbhttpRequestAborted = true;
    mbTimerActive        = false;
    muiTimerActiveTimestamp = 0;
    mpElapsedTimeUpdateTimer->stop();
    // Cancel download requests
    for(int iRequest=0; iRequest < mCurrentDownloads.count(); iRequest++)
    {
        if( (mCurrentDownloads.at(iRequest)->pReply != NULL) && mCurrentDownloads.at(iRequest)->pReply->isRunning() ) {
            disconnect(mCurrentDownloads.at(iRequest)->pReply, SIGNAL(readyRead()), this, SLOT(slot_newDataReceived()));
            disconnect(mCurrentDownloads.at(iRequest)->pReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(slot_updateDownloadProgess(qint64,qint64)));
            mCurrentDownloads.at(iRequest)->pReply->abort();
        }
        mCurrentDownloads.at(iRequest)->pTimer->stop();
        mCurrentDownloads.at(iRequest)->iRetry = 0;
        mCurrentDownloads.at(iRequest)->uiRequestTimestamp = 0;
    }

    miCurPageNum      = 0;
    miCurPageSection  = 0;
    msWaitUntil.clear();
    emit enableDownloadSettings(true);
    emit updateDownloadProgressBar(0, 1);
    emit updateDownloadState(QString("Download canceled."));
}

/*-----------------------------------------------------------------------------------------------*
 * Setup download target settings and start download
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::startDownload(QString sTargetUrl, QString sTargetPages, QString sDstFolder, int iMaxPageNum, QString sDocumentID, bool bDownloadPdfFiles, bool bEnableWebProxy, bool bEnableAutoProxy, bool bResumeDownload, int iZoomlevel, bool bGetOcr)
{
    mbDownloadActive    = true;
    msTargetUrl         = sTargetUrl;
    msPages             = sTargetPages;
    msDstFolder         = sDstFolder;
    miMaxPageNum        = iMaxPageNum;
    miCurPageNum        = 0;
    miCurPageSection    = 0;
    msDocumentID        = sDocumentID;
    mbDownloadPdfFiles  = bDownloadPdfFiles;
    mbResumeDownload    = bResumeDownload;
    miZoomlevel         = iZoomlevel;
    mbGetOcr            = bGetOcr;

    miTotalPageNum      = getTotalPageNum(msPages);
    meDownloadSource    = eHATHITRUST;
    mpTargetPagesList   = 0;
    mpTargetPagesNumList= 0;
    mbWebProxyEnableState =  bEnableWebProxy;
    this->enableWebProxy(mbWebProxyEnableState);
    mbAutoProxyEnableState = bEnableAutoProxy;
    this->enableAutoProxy(mbAutoProxyEnableState);
    if(msPages.contains("Download whole book"))
        mbDownloadWholeBook = true;
    else
        mbDownloadWholeBook = false;

    emit enableDownloadSettings(false);

    // Start parallel downloads
    for (int i=0; i < miNumOfParallelDownloads; i++ )
    {
         this->slot_downloadFilesInParallel(i);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Determine total number of pages to be downloaded
 *-----------------------------------------------------------------------------------------------*/
int FileDownloader::getTotalPageNum(QString sPages)
{
    QStringList mPageSections = sPages.split(QString(","));
    int iNumPage = 0;
    bool ok1 = true, ok2 = true;

    if (sPages.contains("Download whole book"))
    {
        iNumPage = 1;
        miMaxPageNum = 1;
        emit updateDownloadProgressBar(-1, 0);
    }
    else
    {
        for (int iSection = 0; iSection <  mPageSections.length(); iSection++ )
        {
            // Page section is range
            if (mPageSections.at(iSection).contains(QString("-")))
            {
                QStringList curPageSections = mPageSections.at(iSection).split(QString("-"));

                iNumPage +=  curPageSections.at(1).toInt(&ok1,10) - curPageSections.at(0).toInt(&ok2,10) + 1;
            }
            else // page section is single page
            {
                iNumPage++;
            }
        }

        if (ok1 && ok2)
        {
           emit updateDownloadProgressBar(0, iNumPage);
        }
        else
        {
           emit updateDownloadProgressBar(0, 1);
           updateDownloadState("illegal page selection. Download canceled.");
        }
    }

    return iNumPage;
}

/*-----------------------------------------------------------------------------------------------*
 * Determine current page number to be downloaded
 *-----------------------------------------------------------------------------------------------*/
int FileDownloader::getCurPageNum(void)
{
    if (mbDownloadWholeBook)
    {
        miCurPageNum++;
    }
    else
    {
        if ( miCurPageNum > 0)
        {
            if ( miCurPageNum < miMaxPageNum)
            {
                 // Update progress bar
                emit updateDownloadProgressBar(miCurPageNum, miMaxPageNum);
            }
        }
        else
        {
            // Reset progress bar
            emit updateDownloadProgressBar(0, 1);
        }

         QStringList sPageSections = msPages.split(QString(","));

        if (miCurPageSection < sPageSections.length())
        {
            // Page section is range
            if (sPageSections.at(miCurPageSection).contains(QString("-")))
            {
                QStringList curPageSections = sPageSections.at(miCurPageSection).split(QString("-"));

                if (miCurPageNum < curPageSections.at(0).toInt())
                {
                    miCurPageNum =  curPageSections.at(0).toInt();
                }
                else
                {
                    miCurPageNum++;
                }

                if (miCurPageNum == curPageSections.at(1).toInt())
                {
                    miCurPageSection++;
                }
            }
            else // page section is single page
            {
                miCurPageNum = sPageSections.at(miCurPageSection).toInt();
                miCurPageSection++;
            }
        }
        else
        {
            miCurPageNum = miMaxPageNum+1;
        }
    }

    return miCurPageNum;
}

/*-----------------------------------------------------------------------------------------------*
 * Parallel download: Start download of document pages (pdf, images)
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::slot_downloadFilesInParallel(int iRequest)
{
    QString sFileFolder;
    QString sFileFormat;
    QString sZoomlevel;

    bool bCreateNewRequest = true;

    /*-----------------------------------------------------------------------------------------------*/
    // CASE 0: TIMEOUT: Check if current request has not been finished properly
    if ( (mCurrentDownloads.at(iRequest)->pReply != 0) && (mCurrentDownloads.at(iRequest)->pReply->isRunning()))
    {
        qDebug() << "DOWNLOADER: TIMEOUT REQ(" << QString::number(iRequest) << ") URL: " <<  mCurrentDownloads.at(iRequest)->pReply->url();

        // Timeout for request occurred
        mCurrentDownloads.at(iRequest)->pTimer->stop();
        mCurrentDownloads.at(iRequest)->pReply->close();  // Close() will generate finshed() signal

        return;
    }
    else if (mbAutoProxyToWebProxyHandover)
    {
        // Wait until handover is done
        mCurrentDownloads.at(iRequest)->pTimer->start(5000);

        return;
    }
    /*-----------------------------------------------------------------------------------------------*/
    // CASE 1: Check for webproxy request state
    else if (mpWebProxy->mbWebProxyRequestActivated)
    {
        // WebProxy request is in procress - wait for new Webproxy
        mCurrentDownloads.at(iRequest)->pTimer->start(5000);
        bCreateNewRequest = false;

        return;
    }
    /*-----------------------------------------------------------------------------------------------*/
    // CASE 2: Check for autoproxy request state
    else if (mpAutoProxy->mbAutoProxyIsBusy)
    {
        // AutoProxy request is in procress - wait for new Autoproxy
        mCurrentDownloads.at(iRequest)->pTimer->start(5000);
        bCreateNewRequest = false;

        return;
    }
    /*-----------------------------------------------------------------------------------------------*/
    // CASE 3: Download has failed - Check for retry state
    else if ( (mCurrentDownloads.at(iRequest)->iRetry < 6)&& ( mCurrentDownloads.at(iRequest)->iRetry > 0 )) // TODO: check
    {
        // Re-Create download request
        bCreateNewRequest = false;
    }
    /*-----------------------------------------------------------------------------------------------*/
    // CASE 4: Create new request for next file
    else if ( bCreateNewRequest )
    {
        // Get next page number for current download
        mCurrentDownloads.at(iRequest)->iPage = getCurPageNum();
    }

    // Check number of downloaded pages: create a new request or re-create a failed request
    if ( (miCurPageNum <= miMaxPageNum)  || (!bCreateNewRequest && (iRequest < miTotalPageNum)))
    {
        //emit enableDownloadSettings(false);
        if (mpWebProxy->getWebProxyPrefix().isEmpty() && !mpAutoProxy->getAutoProxyState())
        {
            emit updateWebProxyState(QString(""));
        }

        // Create download URL
        QString sTargetUrl;
        if ( meDownloadSource == eHATHITRUST)
        {
            // Determine file format and zoom level
            if (mbDownloadPdfFiles)
            {
                sFileFolder = QString("download/");
                sFileFormat = QString("pdf");
                sZoomlevel  = QString("");
            }
            else
            {
                sFileFolder = QString("");
                sFileFormat = QString("image");
                sZoomlevel  = QString("size=");
                sZoomlevel.append(QString::number(miZoomlevel));
            }
            mCurrentDownloads.at(iRequest)->sFileFormat = sFileFormat;
            mCurrentDownloads.at(iRequest)->sZoomlevel  = sZoomlevel;

            // In case of "Download whole book" max page number is 1 and no "seq" is added to download link
            if (mbDownloadWholeBook)
                // Create download link request, Prefix is empty when no webproxy is used
                sTargetUrl = QString("https://babel.hathitrust.org/cgi/imgsrv/") + sFileFolder + mCurrentDownloads.at(iRequest)->sFileFormat + QString("?id=") + msDocumentID + QString(";") + mCurrentDownloads.at(iRequest)->sZoomlevel;
            else
                // Create download link request, Prefix is empty when no webproxy is used
                sTargetUrl = QString("https://babel.hathitrust.org/cgi/imgsrv/") + sFileFolder + mCurrentDownloads.at(iRequest)->sFileFormat + QString("?id=") + msDocumentID + QString(";seq=") + QString("%1;").arg(mCurrentDownloads.at(iRequest)->iPage,0,10)  + mCurrentDownloads.at(iRequest)->sZoomlevel;
        }

        QUrl fileUrl;
        if (mpWebProxy->getWebProxyEnableState())
            fileUrl = QUrl(mpWebProxy->getProxifiedUrl(sTargetUrl));
        else
            fileUrl = QUrl(sTargetUrl);

        QNetworkRequest request(fileUrl);
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");
        request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, HTTP_PIPELINIG_ALLOWED);

        if (mbDownloadWholeBook)
        {
            QByteArray rangeHeaderValue = "bytes=" + QByteArray::number(50779573) + "-";
            request.setRawHeader("Range",rangeHeaderValue);
        }
        mbhttpRequestAborted = false;

        // Create path and file
        QString sFileName;
            sFileName = makePathAndFilename(mCurrentDownloads.at(iRequest)->sFileFormat, mCurrentDownloads.at(iRequest)->iPage);
        if (sFileName.isNull())
        {
            // Directory creation failed
            return;
        }
        mCurrentDownloads.at(iRequest)->pFile = new QFile(sFileName);


        bool bDownloadFile = true;
        // Check if file already exist
        if (mbResumeDownload &&
           (  (   mCurrentDownloads.at(iRequest)->pFile->exists(sFileName + "pdf" ))
              || (mCurrentDownloads.at(iRequest)->pFile->exists(sFileName + "jpeg"))
              || (mCurrentDownloads.at(iRequest)->pFile->exists(sFileName + "png" ))  )
           && ( !mbGetOcr
              || mCurrentDownloads.at(iRequest)->pFile->exists(msDstFolder + "ocr/" + msDocumentID + QString("_page_%1.html").arg(mCurrentDownloads.at(iRequest)->iPage, QString::number(miMaxPageNum).length(), 10, QChar('0'))))
           )
        {
            qDebug() <<"DOWNLOADER: FILE ALREADY EXISTS";

            // Check if image file is readable
            if(    (mCurrentDownloads.at(iRequest)->pFile->exists(sFileName + "jpeg"))
                || (mCurrentDownloads.at(iRequest)->pFile->exists(sFileName + "png" )))
            {
                QImage imageFile;
                QString sImageFilename;

                if(mCurrentDownloads.at(iRequest)->pFile->exists(sFileName + "jpeg"))
                    sImageFilename = mCurrentDownloads.at(iRequest)->pFile->fileName() + "jpeg";
                else
                    sImageFilename = mCurrentDownloads.at(iRequest)->pFile->fileName() + "png";

                if(!imageFile.load(sImageFilename))
                    qDebug() << "DOWNLOADER: CORRPUTED IMAGE FILE DETECTED "<< sImageFilename;
                else
                    bDownloadFile = false;
            }
            // Check pdf file size
            else if(mCurrentDownloads.at(iRequest)->pFile->exists(sFileName + "pdf" ))
            {
                QFile pdfFile(sFileName + "pdf" );
                if (pdfFile.open(QIODevice::ReadOnly) &&((uint) pdfFile.size() >= MINIMUM_FILESIZE))
                    bDownloadFile = false;
                else
                    qDebug() << "DOWNLOADER: CORRPUTED PDF FILE DETECTED "<< mCurrentDownloads.at(iRequest)->pFile->exists(sFileName + "pdf" );
            }

            if (!bDownloadFile)
            {
                qDebug() <<"DOWNLOADER: FILE ALREADY EXISTS SKIPPING DOWNLOAD";

                mCurrentDownloads.at(iRequest)->iRetry = DOWNLOAD_REQUEST_RESUMED;

                // Page incremenation is done in getCurPageNum()
                emit updateDownloadState(QString("Resuming download: checking page %1 of %2...").arg(miCurPageNum).arg(miTotalPageNum));

                if (mCurrentDownloads.at(iRequest)->pReply != 0)
                {
                    mCurrentDownloads.at(iRequest)->pReply->deleteLater();
                    mCurrentDownloads.at(iRequest)->pReply = 0;
                }
                // Check next page
                mCurrentDownloads.at(iRequest)->pTimer->start(100);

                return;
            }
        }

        // Download file
        if (bDownloadFile)
        {
            // Page incremenation is done in getCurPageNum()
            qDebug() << QString::number(iRequest) << "DOWNLOADER: CREATE DOWNLOAD REQUEST FOR PAGE X= " << QString::number(mCurrentDownloads.at(iRequest)->iPage) << " of y = " <<  QString::number(miTotalPageNum);
            emit updateDownloadState(QString("Downloading page %1 of %2: receiving file data...").arg(mCurrentDownloads.at(iRequest)->iPage).arg(miTotalPageNum));

            if (!mCurrentDownloads.at(iRequest)->pFile->open(QIODevice::WriteOnly))
            {
                qDebug() << QString::number(iRequest) << "DOWNLOADER: UNABLE TO CREATE FILE";

                cancelParallelDownload();
                emit cancelDownload();

                QMessageBox::information(mpParent, tr("HTTP"),tr("Unable to save the file %1: %2.").arg(sFileName).arg(mCurrentDownloads.at(iRequest)->pFile->errorString()));

                if (mCurrentDownloads.at(iRequest)->pFile)
                {
                    mCurrentDownloads.at(iRequest)->pFile->close();
                    mCurrentDownloads.at(iRequest)->pFile->remove();
                    delete mCurrentDownloads.at(iRequest)->pFile;
                    mCurrentDownloads.at(iRequest)->pFile = 0;
                }

                return;
            }

            // Start download request
            if(mCurrentDownloads.at(iRequest)->pReply != NULL)
                mCurrentDownloads.at(iRequest)->pReply->deleteLater();
            mCurrentDownloads.at(iRequest)->pReply = mpNetwManager->get(request);
            if (mbDownloadWholeBook)
            {
                connect(mCurrentDownloads.at(iRequest)->pReply, SIGNAL(readyRead()), this, SLOT(slot_newDataReceived()));
                connect(mCurrentDownloads.at(iRequest)->pReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(slot_updateDownloadProgess(qint64,qint64)));
            }
            mCurrentDownloads.at(iRequest)->iRetry = 0;
            QDateTime timestamp;
            mCurrentDownloads.at(iRequest)->uiRequestTimestamp =timestamp.toTime_t();
            mCurrentDownloads.at(iRequest)->pTimer->start(15000);
        }
    }
    /*-----------------------------------------------------------------------------------------------*/
    // CASE 5: Download is finished when all requests are done
    else if ( (miCurPageNum > miMaxPageNum) && bCreateNewRequest)
    {
        qDebug() << "DOWNLOADER: DOWNLOAD REQUEST FINISHED";
        mCurrentDownloads.at(iRequest)->iRetry = DOWNLOAD_REQUEST_IS_FINISHED;
        mCurrentDownloads.at(iRequest)->pTimer->stop();

        bool bDownloadFinished = true;
        for (int i = 0; i < mCurrentDownloads.count(); i++)
        {
            bDownloadFinished = ( bDownloadFinished && (mCurrentDownloads.at(i)->pReply == NULL)
                                                    && (mCurrentDownloads.at(i)->iRetry == DOWNLOAD_REQUEST_IS_FINISHED) );
        }

        if (bDownloadFinished)
        {
            qDebug() << "DOWNLOADER: ALL DOWNLOADS FINISHED";

            if (mCurrentDownloads.at(iRequest)->pReply)
            {
                disconnect(mCurrentDownloads.at(iRequest)->pReply, SIGNAL(readyRead()), this, SLOT(slot_newDataReceived()));
                disconnect(mCurrentDownloads.at(iRequest)->pReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(slot_updateDownloadProgess(qint64,qint64)));
            }

            if (mbTimerActive)
            {
                mbTimerActive = false;
                muiTimerActiveTimestamp = 0;
                mpElapsedTimeUpdateTimer->stop();
            }

//            for (int i = 0; i < mCurrentDownloads.count(); i++)
//            {
//                mCurrentDownloads.at(i)->pTimer->stop();
//            }

            miCurPageNum    = 0;
            miCurPageSection = 0;

            // Reset prefix and update file with WebProxy list when Webproxy enabled
            if (mpWebProxy->getWebProxyEnableState())
                mpWebProxy->setWebProxyPrefix("");

            // Reset WebProxy
            mpWebProxy->reset();

            // Reset AutoProxy
            if (mpAutoProxy->getAutoProxyState())
            {
                mpAutoProxy->updateAutoProxyQueue(0);
                mpAutoProxy->abortAutoProxyRequest();
            }

            mbDownloadActive = false;

            emit updateDownloadProgressBar(miMaxPageNum, miTotalPageNum);
            emit updateDownloadState(QString("Done."));
            emit downloadFinished(true);
            emit enableDownloadSettings(true);
        }
        else
        {
            if (mpWebProxy->getWebProxyEnableState()  || mpAutoProxy->getAutoProxyState())
            {
                // Check download state of other requests
                for (int i = 0; i < mCurrentDownloads.count(); i++)
                {
                    /* When request is in waiting state (download failed, timer is running) */
                    if (    (mCurrentDownloads.at(i)->pReply != NULL)
                         && (mCurrentDownloads.at(i)->iRetry > 0)
                         && (mCurrentDownloads.at(i)->pTimer->isActive()) )
                    {
                        /* restart pending download request now */
                        mCurrentDownloads.at(i)->pTimer->start(50);
                    }
                }
            }

            // Wait for other download requests
        }
    }
}


/*-----------------------------------------------------------------------------------------------*
 * Make path and create filename
 *-----------------------------------------------------------------------------------------------*/
QString FileDownloader::makePathAndFilename(QString sFileFormat, int iPageNum)
{
    QDir filePath;
    if (!filePath.mkpath(msDstFolder))
    {
        cancelParallelDownload();
        emit cancelDownload();

        emit updateDownloadState(QString("Unable to create directory...."));

        return 0;
    }

    // create subdir
    QString sSubDir(msDstFolder);
    if (mbDownloadWholeBook)
    {
        // Direct download to directory
    }
    else if (sFileFormat.contains("pdf"))
    {
           sSubDir.append(QString("pdfs/"));
    }
    else
    {
          sSubDir.append(QString("images/"));
    }

    if (!filePath.mkpath(sSubDir))
    {
        cancelParallelDownload();
        emit cancelDownload();

        emit updateDownloadState(QString("Unable to create sub-directory: ") + filePath.path());

        return 0;
    }

    // Create file name
    int iMaxNumDigits = QString::number(miMaxPageNum).length();
    QString sFileName(msDocumentID);
    removeReservedCharFromString(&sFileName);
    if (mbDownloadWholeBook)
        sFileName.append(QString("."));
    else
        sFileName.append(QString("_page_%1.").arg(iPageNum, iMaxNumDigits, 10, QChar('0')));
    sFileName.prepend(sSubDir);

    return sFileName;
}

/*-----------------------------------------------------------------------------------------------*
 * Parallel download: Store new data package
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::slot_newDataReceived(void)
{
    int iReply;

    QNetworkReply* pReply = (QNetworkReply*) this->sender();

    if (pReply)
    {
        for (iReply=0; iReply < mCurrentDownloads.count(); iReply++)
        {
            if (mCurrentDownloads.at(iReply)->pReply == pReply)
            {
                mCurrentDownloads.at(iReply)->pTimer->start(15000);
                mCurrentDownloads.at(iReply)->pFile->write(pReply->readAll());

                break;
            }
        }
    }
}


/*-----------------------------------------------------------------------------------------------*
 * Parallel download: Update download progess for "whole book" downloads
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::slot_updateDownloadProgess(qint64 iBytesReceived, qint64 iBytesTotal)
{
    static int i = 0;
    if (i++ > 10)
    {
        emit updateDownloadState(QString("Downloading whole book: %1 KBytes received...").arg(iBytesReceived/1024));
        i = 0;
        qDebug() << "DOWNLOADER: " << iBytesReceived << " of " << iBytesTotal;
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Parallel Download finished: Store downloaded data on hdd when download has finished
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::slot_fileDownloadFinished(QNetworkReply* pReply)
{
    switch(meDownloadSource)
    {
    case eHATHITRUST:
        fileDownloadFinishedHathitrust(pReply);
        break;
    default:
        emit cancelDownload();
        break;
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Parallel Download finished: Store downloaded data on hdd when download has finished
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::fileDownloadFinishedHathitrust(QNetworkReply* pReply)
{

    static int iCorruptedFileRetries[MAX_NUM_REQUEST] = {0};

    // Identify which request has finished
    int iReply;
    for (iReply=0; iReply < mCurrentDownloads.count(); iReply++)
    {
        if (mCurrentDownloads.at(iReply)->pReply == pReply)
        {
            mCurrentDownloads.at(iReply)->pTimer->stop();
            break;
        }
    }

    // CASE 0: Download canceled by user
    if (mbhttpRequestAborted)
    {
        emit downloadFinished(false);

        if (iReply < mCurrentDownloads.count())
        {
            if ( mCurrentDownloads.at(iReply)->pFile)
            {
                mCurrentDownloads.at(iReply)->pFile->close();
                mCurrentDownloads.at(iReply)->pFile->remove();
                delete mCurrentDownloads.at(iReply)->pFile;
                mCurrentDownloads.at(iReply)->pFile = 0;
                mCurrentDownloads.at(iReply)->pTimer->stop();
            }
        }

        mpElapsedTimeUpdateTimer->stop();

        return;
    }

    // CASE 1: error - request could not be identified
    if( iReply == mCurrentDownloads.count())
    {
        qWarning("Error: unresolved download request failure on download finished!");

        if ( mCurrentDownloads.count() == 1)
        {
            cancelParallelDownload();
            emit cancelDownload();
        }

        // cancel this request
        return;
    }

    // Check received download reply
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    // CASE 2: On error or redirection
    if (        pReply->error()                                                                                                     // error
            || !redirectionTarget.isNull()                                                                                          // redirection
            ||  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty()                                             // empty page
            || !(pReply->header(QNetworkRequest::ContentTypeHeader).toString().contains(mCurrentDownloads.at(iReply)->sFileFormat)) // invalid file format
       )
    {
         qDebug() << QString::number(iReply)  << "DOWNLOADER: DOWNLOAD FAILED " << pReply->url().toString().toLatin1();
         qDebug() << QString::number(iReply)  << "Reply->error() : " << pReply->error();
         qDebug() << QString::number(iReply)  << "redirectionTarget.isNull(): " << redirectionTarget;
         qDebug() << QString::number(iReply)  << "pReply->header : " << pReply->header(QNetworkRequest::ContentTypeHeader).toString();

        // Delete file handle
        if ( mCurrentDownloads.at(iReply)->pFile)
        {
            mCurrentDownloads.at(iReply)->pFile->close();
            mCurrentDownloads.at(iReply)->pFile->remove();
            delete mCurrentDownloads.at(iReply)->pFile;
            mCurrentDownloads.at(iReply)->pFile = 0;
        }

        QDateTime nowDate = QDateTime::currentDateTime();
        QDateTime untilDate;

        //  Start timer for elapsed time -----------------------------------------------------------------
        if ( !mbTimerActive )
        {
            miElapsedTime = 0;
            mbTimerActive = true;
            muiTimerActiveTimestamp = nowDate.toTime_t();

            // Reset time for elapsed time value on GUI
            mpElapsedTime->restart();

            // Extract waiting time from reply data
            if ( pReply->hasRawHeader("X-Choke-Now") && pReply->hasRawHeader("X-Choke-Until") )
            {
                 QString sNowDate = QString::fromLatin1(pReply->rawHeader("X-Choke-Now"));
                 QString sUntilDate   = QString::fromLatin1(pReply->rawHeader("X-Choke-Until"));

                 if (!msWaitUntil.contains(sUntilDate))
                 {
                     msWaitUntil = sUntilDate;

                     nowDate    = QDateTime::fromString(sNowDate, Qt::ISODate);
                     untilDate  = QDateTime::fromString(sUntilDate, Qt::ISODate);
                     msWaitTime = QTime().addSecs(nowDate.secsTo(untilDate)).toString("mm:ss");
                     miWaitTime = nowDate.secsTo(untilDate) * 1000;
                 }
            }
            else
            {
                // Default values
                msWaitTime = QString("05:00");
                miWaitTime = 30000;
            }
        }

        mCurrentDownloads.at(iReply)->iRetry++;

        // Start timer for GUI timer
        slot_startTimer();

        // Start retry timer: check download availability after that time, e.g. 5:00 for pdfs
        mCurrentDownloads.at(iReply)->pTimer->start(miWaitTime);

        // Use WebProxies or AutoProxy when enabled -----------------------------------------------------------
        if (mpWebProxy->getWebProxyEnableState() || mpAutoProxy->getAutoProxyState())
        {
            // Check if a WebProxy request is in process
            if (mpWebProxy->mbWebProxyRequestActivated || mpAutoProxy->mbAutoProxyIsBusy)
            {
                // Start 5 sec timer in case of pending proxy request: check download availability after that time
                mCurrentDownloads.at(iReply)->pTimer->start(5000);
            }
            else
            {
                // Wait for all active download request to be finished
                bool bAllDownloadRequestWaiting = true;
                for ( int j=0; j < mCurrentDownloads.count(); j++ )
                {
                    if ( (mCurrentDownloads.at(j)->pReply != NULL) && (mCurrentDownloads.at(j)->iRetry > DOWNLOAD_REQUEST_IS_FINISHED) )
                    {
                        bAllDownloadRequestWaiting = ( bAllDownloadRequestWaiting && ((mCurrentDownloads.at(j)->iRetry > 0)) );
                    }
                }

                if (bAllDownloadRequestWaiting)
                {
                    QDateTime CurrentDate = QDateTime::currentDateTime();
                    if (mpWebProxy->getWebProxyEnableState())
                    {
                        qDebug() << "DOWNLOADER: WEBPROXY REQ START";

                        mpWebProxy->updateWebProxyQueue(CurrentDate.toTime_t() + (miWaitTime/1000));


                        // Start WebProxyRequest
                        mpWebProxy->getNewWebProxy(msTargetUrl + "view=plaintext;seq=1;");

                      //  mpWebProxy->getNewWebProxy(QString("https://babel.hathitrust.org/cgi/imgsrv/download/pdf?id=%1;seq=1;").arg(msDocumentID));
                    }
                    else if (mpAutoProxy->getAutoProxyState())
                    {
                        qDebug() << "DOWNLOADER: PROXY REQ START";
                        qDebug() << "wait for " << QString::number((miWaitTime/1000));
                        qDebug() << "AutoProxy is Busy: " << mpAutoProxy->getProxyBusyFlagState();
                        mpAutoProxy->updateAutoProxyQueue(CurrentDate.toTime_t() + (miWaitTime/1000));

                        // Apply new proxy connection
                        mpAutoProxy->applyProxyServer();
                    }

                    // Start parallel downloads
                    for (int i=0; i < miNumOfParallelDownloads; i++ )
                    {
                        mCurrentDownloads.at(i)->pTimer->start(50);
                    }
                }
                else
                {
                    // But wait at maximum 30 secs
                    mCurrentDownloads.at(iReply)->pTimer->start(30000);
                }
            }
        }
    }
    // CASE 4: Download completed
    else
    {
        // Download completed
        qDebug() << QString::number(iReply)  << "DOWNLOADER: DOWNLOAD COMPLETED " << pReply->url().toString().toLatin1();

        // Store data in file
        mCurrentDownloads.at(iReply)->pFile->write(pReply->readAll());
        mCurrentDownloads.at(iReply)->pFile->flush();
        mCurrentDownloads.at(iReply)->pFile->close();
        QString sFilename = mCurrentDownloads.at(iReply)->pFile->fileName();

        // Determine file extension from server response/Content-Type:
        // Server response for ocr:     text/html;charset=utf-8
        // Server response for image:   image/jpeg
        // Server response for image:   image/png
        // Server response for pdf:     application/pdf
        QString sFileExt(mCurrentDownloads.at(iReply)->pReply->header(QNetworkRequest::ContentTypeHeader).toString());
        sFileExt = sFileExt.right(sFileExt.length() - sFileExt.indexOf(QString("/")) - (int) 1);
        if ( sFileExt.contains(QChar(';')))
        {
            sFileExt = sFileExt.left(sFileExt.indexOf(QString(";")));
        }

        // Remove old file if exist
        sFilename.append(sFileExt);
        if (mCurrentDownloads.at(iReply)->pFile->exists(sFilename))
        {
            mCurrentDownloads.at(iReply)->pFile->remove(sFilename);
        }
        mCurrentDownloads.at(iReply)->pFile->rename(sFilename);

        // Check if image file is readable
        if(mCurrentDownloads.at(iReply)->sFileFormat.contains("image"))
        {
            QImage imageFile;
            if(!imageFile.load(mCurrentDownloads.at(iReply)->pFile->fileName()) && (iCorruptedFileRetries[iReply] < DOWNLOAD_MAX_NUM_RETRIES) )
            {
                // Increase retry counter for corrupted file, but ignore error after xx retries
                iCorruptedFileRetries[iReply]++;    // Required to avoid infinite loop in case of corrupted files online

                // Restart file download
                mCurrentDownloads.at(iReply)->iRetry++;
                mCurrentDownloads.at(iReply)->pTimer->start(1000);

                qDebug() << "DOWNLOADER: CORRPUTED FILED DETECTED "<< mCurrentDownloads.at(iReply)->pFile->fileName();

                return;
            }
            else
            {
                iCorruptedFileRetries[iReply] = 0;
            }
        }
        // Check pdf file size
        else if(mCurrentDownloads.at(iReply)->sFileFormat.contains("pdf"))
        {
            bool bError = true;

            QFile pdfFile(mCurrentDownloads.at(iReply)->pFile->fileName());
            if(pdfFile.open(QIODevice::ReadOnly))
            {
                QTextStream stream(&pdfFile);
                QString sPdfContent = stream.readAll();
                if (sPdfContent.contains("%%EOF"))
                    bError = false;
            }

            if ((bError || (pdfFile.size() < MINIMUM_FILESIZE)) && (iCorruptedFileRetries[iReply] < DOWNLOAD_MAX_NUM_RETRIES) )
            {
                qDebug() << QString("DOWNLOADER: DOWNLOAD FAILED:  CORRUPTED PDF FILE [EOF: %1, MINSIZE: %2").arg(!bError).arg((pdfFile.size() < MINIMUM_FILESIZE));

                // Increase retry counter for corrupted file, but ignore error after xx retries
                iCorruptedFileRetries[iReply]++;    // Required to avoid infinite loop in case of corrupted files online

                // Restart file download
                mCurrentDownloads.at(iReply)->iRetry++;
                mCurrentDownloads.at(iReply)->pTimer->start(1000);

                qDebug() << "DOWNLOADER: CORRPUTED FILED DETECTED "<< mCurrentDownloads.at(iReply)->pFile->fileName();

                return;
            }
            else
            {                
                 iCorruptedFileRetries[iReply] = 0;
            }
        }

        // Reset timer when another request is already active and not older than 15s
        if (mbTimerActive && ((muiTimerActiveTimestamp + 15) < mCurrentDownloads.at(iReply)->uiRequestTimestamp))
        {
            mbTimerActive = false;
            muiTimerActiveTimestamp = 0;
            mpElapsedTimeUpdateTimer->stop();
        }

        // Download ocr file, if checkbox is selected and file does not exist
        if ( mbGetOcr && !(sFileExt == QString("html")))
        {
            qDebug() << QString::number(iReply) << "DOWNLOADER: DOWNLOAD OCR FILE";
            delete mCurrentDownloads.at(iReply)->pFile;
            mCurrentDownloads.at(iReply)->pFile   = 0;
            mCurrentDownloads.at(iReply)->sFileFormat = QString("html");
            mCurrentDownloads.at(iReply)->pTimer->start(15000);

            downloadOcrText(mCurrentDownloads.at(iReply));
        }
        else if (sFileExt == QString("html"))
        {
            qDebug() << QString::number(iReply) << "DOWNLOADER: EXTRACT OCR FROM HTML";
            // Extract ocr text from downloaded data
            ocrPageLoaded(iReply);

            if (!mbhttpRequestAborted)
            {
                // Continue with next page
                delete mCurrentDownloads.at(iReply)->pFile;
                mCurrentDownloads.at(iReply)->pFile   = 0;
                mCurrentDownloads.at(iReply)->pReply->deleteLater();
                mCurrentDownloads.at(iReply)->pReply  = 0;

                // Download next page
                slot_downloadFilesInParallel(iReply);
            }
        }
        else
        {
            // Continue with next page
            delete mCurrentDownloads.at(iReply)->pFile;
            mCurrentDownloads.at(iReply)->pFile   = 0;
            mCurrentDownloads.at(iReply)->pReply->deleteLater();
            mCurrentDownloads.at(iReply)->pReply  = 0;

            slot_downloadFilesInParallel(iReply);
        }
    }
}


/*-----------------------------------------------------------------------------------------------*
 * Parallel download: Download Ocr text page from hathitrust.org
 *-----------------------------------------------------------------------------------------------*/
int FileDownloader::getNumOfParallelDownloads(void)
{
    return miNumOfParallelDownloads;
}


/*-----------------------------------------------------------------------------------------------*
 * Parallel download: Download Ocr text page from hathitrust.org
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::downloadOcrText(tDownloadRequest* apDownloadRequest)
{
    /*-----------------------------------------------------------------------------------------------*
    // Direct access to ocr files is limited by server and causing a timeout due to maximum traffic exceeded: 15 downloads/ 5 min
    // QUrl url( QString("https://babel.hathitrust.org/cgi/imgsrv/") + sFileFormat + QString("?id=") + msDocumentID + QString(";seq=") + QString("%1").arg(miCurPageNum,0,10));

    // Instead download plain text view and extract ocr text later on
     *-----------------------------------------------------------------------------------------------*/

    // Create download link request
    QUrl url(QString("https://babel.hathitrust.org/cgi/pt?id=") + msDocumentID + QString(";view=plaintext;seq=%1;").arg(apDownloadRequest->iPage));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, HTTP_PIPELINIG_ALLOWED);
    mbhttpRequestAborted = false;

    if (!mbTimerActive && (miCurPageNum <= miTotalPageNum))
    {
        emit updateDownloadState(QString("Downloading page %1 of %2: receiving ocr data..." ).arg(apDownloadRequest->iPage).arg(miTotalPageNum));
    }

    // Create path
    QDir filePath;
    if (!filePath.mkpath(msDstFolder))
    {
        emit updateDownloadState(QString("Unable to create directory...."));
        cancelParallelDownload();
        emit cancelDownload();

        return;
    }

    // Create subdir
    QString sSubDir(msDstFolder);
    sSubDir.append(QString("ocr/"));

    if (!filePath.mkpath(sSubDir))
    {
        emit updateDownloadState(QString("Unable to create sub-directory: ") + filePath.path());
        cancelParallelDownload();
        emit cancelDownload();

        return;
    }

    // Create file name
    QString sFileName(msDocumentID);
    removeReservedCharFromString(&sFileName);

    sFileName.append(QString("_page_%1.").arg(apDownloadRequest->iPage, QString::number(miMaxPageNum).length(), 10, QChar('0')));
    sFileName.prepend(sSubDir);

    apDownloadRequest->pFile = new QFile(sFileName);
    if (!apDownloadRequest->pFile->open(QIODevice::WriteOnly))
    {
        qDebug() << "DOWNLOADER: Unable to save the file ";
        cancelParallelDownload();
        emit cancelDownload();

        QMessageBox::information(mpParent, tr("HTTP"),tr("Unable to save the file %1: %2.").arg(sFileName).arg(apDownloadRequest->pFile->errorString()));

        if (apDownloadRequest->pFile)
        {
            apDownloadRequest->pFile->close();
            apDownloadRequest->pFile->remove();
            delete apDownloadRequest->pFile;
            apDownloadRequest->pFile = 0;
        }

        return;
    }

    // Start file download
    if (apDownloadRequest->pReply != NULL)
        apDownloadRequest->pReply->deleteLater();
    apDownloadRequest->pReply = mpNetwManager->get(request);
}

/*-----------------------------------------------------------------------------------------------*
 * Parallel download: Extract plain ocr text from html page
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::ocrPageLoaded(int iRequest)
{
    if (mbTimerActive && (miElapsedTime > miWaitTime) )
    {
        mbTimerActive = false;
        muiTimerActiveTimestamp = 0;
        mpElapsedTimeUpdateTimer->stop();
    }

    QString sFilename = mCurrentDownloads.at(iRequest)->pFile->fileName();

    qDebug() << "DOWNLOADER: EXTRACT OCR FORM FILE: " << sFilename;

    // Extract ocr content
    QString sHtml       = readFile(sFilename);

    // Determine index of ocr start tag
    int iStartOfOcrTxt  = sHtml.indexOf("<div class=\"page-item page-text");
    // Determine index of ocr end tag
    int iEndOfOcrTxt    = sHtml.indexOf("</div>", iStartOfOcrTxt);

    // Extract ocr text
    QString sHtmlOcr;
    if ( (iStartOfOcrTxt > 0) && (iEndOfOcrTxt > 0) )
    {
        sHtmlOcr    = sHtml.mid(iStartOfOcrTxt, iEndOfOcrTxt - iStartOfOcrTxt + QString("</div>").length());
    }
    else
    {
        qDebug() << "DOWNLOADER: NO OCR ON PAGE";
        // Empty file
        sHtmlOcr = "<div id=\"content\">";
        sHtmlOcr.append("<div class=\"alert alert-block alert-info alert-headline\">");
        sHtmlOcr.append("<p>NO TEXT ON PAGE</p>");
        sHtmlOcr.append("</div>");
        sHtmlOcr.append("<p>This page does not contain any text recoverable by the OCR engine.</p>");
        sHtmlOcr.append("</div>");
    }

    if (!writeFile(mpParent, sFilename, &sHtmlOcr ))
    {
        emit updateDownloadState(QString("Unable to create ocr text file ..."));
        cancelParallelDownload();
        emit cancelDownload();

        return;
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Remove reserved characters from string
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::removeReservedCharFromString(QString *psInput, QString *psOutput)
{
    if (!psOutput)
    {
       psOutput = psInput;
    }

    // Remove reserved characters from string
    QString sReservedChar("<>:\"\\/?*|");
    for (int i=0; i < sReservedChar.count(); i++)
    {
        psOutput->remove(sReservedChar.at(i));
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Return enable webproxy
 *-----------------------------------------------------------------------------------------------*/
bool FileDownloader::getEnableWebProxy(void)
{
    return mbWebProxyEnableState;
}

/*-----------------------------------------------------------------------------------------------*
 * Enable webproxy
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::enableWebProxy(bool bIsChecked)
{
    // TODO: CHECK HANDOVER BETWEEN AUTOPROXY AND WEBPROXY

    static int iCountdown = 0;

    if (mbDownloadActive && mbAutoProxyEnableState && bIsChecked && (iCountdown < WEBPROXY_HANGOVER_DELAY_CYCLES))
    {
        qDebug() << "DOWNLOADER: PROXY HANDOVER " << iCountdown;
        iCountdown++;

        emit updateWebProxyState(QString("WebProxy: proxy handover started...please wait[00:%i]").arg(iCountdown,2,10, QChar('0')));

        // Wait some time
        mbAutoProxyToWebProxyHandover = true;
        mpElapsedTimeProxyHandover->start(WEBPROXY_HANGOVER_DELAY);
    }
    else
    {
        mpElapsedTimeProxyHandover->stop();
        iCountdown = 0;
        mbWebProxyEnableState = bIsChecked;

        mpWebProxy->setWebProxyEnableState(bIsChecked);

        // Abort webproxy
        if (mpWebProxy->mbWebProxyRequestActivated && !bIsChecked)
        {
           // mpWebProxy->abortWebProxyLoadPage(); /// testi
            mpWebProxy->reset();
        }

        // Check if there are any pending downloads
        bool bAnyActiveDownloads = false;
        for ( int j=0; j < mCurrentDownloads.count(); j++ )
        {
            if (mCurrentDownloads.at(j)->pReply != NULL)
            {
                bAnyActiveDownloads = ( bAnyActiveDownloads || ((mCurrentDownloads.at(j)->iRetry > 0)) );
            }
        }
        if (bAnyActiveDownloads)
        {
            // Start all waiting downloads
            for ( int j=0; j < mCurrentDownloads.count(); j++ )
            {
                 mCurrentDownloads.at(j)->pTimer->start(1000);
            }
        }
//        if (bIsChecked)
//            emit updateWebProxyState(QString("WebProxy: enabled"));
//        else
            emit updateWebProxyState(QString(""));

        mpWebProxy->setWebProxyPrefix("");
        mbAutoProxyToWebProxyHandover = false;        
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Enable download resume
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::setResumeDownloadEnabled(bool bState)
{
    qDebug() << "DOWNLOADER: RESUME ENABLED " << bState;
    mbResumeDownload = bState;
}

/*-----------------------------------------------------------------------------------------------*
 * Get filedownload busy state
 *-----------------------------------------------------------------------------------------------*/
bool FileDownloader::getBusyState(void)
{
   return mbDownloadActive;
}

/*-----------------------------------------------------------------------------------------------*
 * Get enable autoproxy
 *-----------------------------------------------------------------------------------------------*/
bool FileDownloader::getEnableAutoProxy(void)
{
   return mbAutoProxyEnableState;
}

/*-----------------------------------------------------------------------------------------------*
 * Enable autoproxy
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::enableAutoProxy(bool bIsChecked)
{
    mbAutoProxyEnableState = bIsChecked;
    mpAutoProxy->setAutoProxyState(bIsChecked);

    // Abort autoproxy
    if (mpAutoProxy->mbAutoProxyIsBusy && !bIsChecked)
    {
        mpAutoProxy->abortAutoProxyRequest();
    }

    // Check if there are any pending downloads
    bool bAnyActiveDownloads = false;
    for ( int j=0; j < mCurrentDownloads.count(); j++ )
    {
        if (mCurrentDownloads.at(j)->pReply != NULL)
        {
            bAnyActiveDownloads = ( bAnyActiveDownloads || ((mCurrentDownloads.at(j)->iRetry > 0)) );
        }
    }
    if (bAnyActiveDownloads)
    {
        // Start all waiting downloads
        for ( int j=0; j < mCurrentDownloads.count(); j++ )
        {
            mCurrentDownloads.at(j)->pTimer->start(50);
        }
    }
    emit updateAutoProxyState(QString(""));
}

/*-----------------------------------------------------------------------------------------------*
 * Clear Cookie Jar
 *-----------------------------------------------------------------------------------------------*/
void FileDownloader::slot_clearCookieJar(bool bSuccess)
{
    if (bSuccess)
        mpNetwManager->setCookieJar(new QNetworkCookieJar());
}
