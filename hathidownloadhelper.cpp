#include "hathidownloadhelper.h"
#include "createpdfworker.h"

#include <QUrl>
#include <QMessageBox>
#include <QFileDialog>
#include <QNetworkProxy>
#include <QEventLoop>
#include <QApplication>
#include <QtGui>
#include <QDebug>
#include <QSslSocket>

// Avoid redefinition of same connection between same elements
#define connect(src, sig, trgt, slt) connect(src, sig, trgt, slt, Qt::UniqueConnection)


/* TODOS ----------------------------------------------------------------------------------------*/
// TODO: this file is ways too large -> split it up, build sub classes
// TODO: resize event / grid resizing still does  not work properly

/*-----------------------------------------------------------------------------------------------*
 * PLEASE NOTE:
 *-----------------------------------------------------------------------------------------------*
 * OS depending commands are defined by the following structure:
#if defined(Q_OS_LINUX)
    // some code
#elif defined(Q_OS_WIN)
    // some code
#elif defined(Q_OS_MAC)
    // some code
#elif  defined(Q_OS_FREEBSD)
    // some code
#else
    // some code
#endif

 * E.g. this is used for in the following function:
 *      showFileInFilemanager() -   Method for opening the OS filemanager
 *-----------------------------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------------------------*
 * Main class constructor
 *-----------------------------------------------------------------------------------------------*/
HathiDownloadHelper::HathiDownloadHelper(QWidget *parent) :
    QMainWindow(parent)
{
    qDebug();
    qDebug() << QString("Welcome to %0 %1").arg(qApp->applicationName()).arg(qApp->applicationVersion());

    //this->hide();

    // Initialize GUI
    initGuiComponents();

    // Initialize Network
    initNetworkComponents();

    // Apply user settings
    setXmlGuiSetup();

    // Initialize printer elements
    initPrinterComponents();

    // Initialize miscellaneous elements
    initMiscComponents();
}
/*-----------------------------------------------------------------------------------------------*
 * hathidownloadhelper destructor
 *-----------------------------------------------------------------------------------------------*/
HathiDownloadHelper::~HathiDownloadHelper()
{
    //
}

/*-----------------------------------------------------------------------------------------------*
 * Init network elements
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::initNetworkComponents()
{        
    mpNetwUpdateManager = new QNetworkAccessManager(this);
    connect(mpNetwUpdateManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(slot_handleSslErrors(QNetworkReply*,QList<QSslError>)));
    mpUpdateReply       = NULL;

    mpNetwBookInfoManager = new QNetworkAccessManager(this);
    mpBookInfoReply       = NULL;
    connect(mpNetwBookInfoManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_bookPageLoaded(QNetworkReply*)));
    connect(mpNetwBookInfoManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(slot_handleSslErrors(QNetworkReply*,QList<QSslError>)));

    mpBookPage = new QWebPage();

    // Proxy
    mpProxy            = new QNetworkProxy;
    mpUserProxy        = new QNetworkProxy;

    mpAutoProxy = new AutoProxy(this);
    connect(mpAutoProxy, SIGNAL(updateAutoProxyState(QString)), ui->labelBookInfoState, SLOT(setText(QString)));
    connect(mpAutoProxy, SIGNAL(updateAutoProxyProgressBar(bool)), this, SLOT(slot_infiniteProgressBarsBookPage(bool)));
    connect(mpAutoProxy, SIGNAL(finished(bool)), this, SLOT(slot_autoProxyRequestCompleted(bool)));

    mpProxyDialog      = new ProxyDialog(mpAutoProxy ,this);
    mpProxyDialog->setXmlProxySetup(mpUserProxy);
    if (!mpProxyDialog->getAutoProxy())
    {
        *mpProxy = *mpUserProxy;
    }
    // Deactivate Proxy by default
    mpProxy->setType(QNetworkProxy::NoProxy);
    QNetworkProxy::setApplicationProxy(*mpProxy);

    // Downloader
    mpFileDownloader   = new FileDownloader(mpAutoProxy, this);
    connect(mpFileDownloader, SIGNAL(updateDownloadProgressBar(int, int)), this, SLOT(slot_updateDownloadProgressBar(int, int)));
    connect(mpFileDownloader, SIGNAL(updateDownloadState(QString)), this, SLOT(slot_updateDownloadState(QString)));
    connect(mpFileDownloader, SIGNAL(updateWebProxyState(QString)), this, SLOT(slot_updateWebProxyState(QString)));
    connect(mpFileDownloader, SIGNAL(downloadFinished(bool)), this, SLOT(slot_fileDownloadFinished(bool)));
    connect(mpFileDownloader, SIGNAL(enableDownloadSettings(bool)), this, SLOT(slot_enableDownloadSettings(bool)));
    if (ui->batchJobTabWidget != 0)
        connect(mpFileDownloader, SIGNAL(cancelDownload()), ui->batchJobTabWidget, SLOT(slot_pushButtonCancel_clicked()));

}

/*-----------------------------------------------------------------------------------------------*
 * Init miscellaneous components
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::initMiscComponents()
{
    mpClipboard = QApplication::clipboard();
    connect (mpClipboard, SIGNAL(dataChanged()) , this, SLOT(slot_copyFromClipboard()));
    connect (mpClipboard, SIGNAL(changed(QClipboard::Mode)) , this, SLOT(slot_copyFromClipboard()));
    // Timer for download retries
    mpRetryTimer = new QTimer(this);
    mpRetryTimer->setSingleShot(true);
    // Timer for feedback messages
    mpRetryTimerFeedback = new QTimer(this);
    mpRetryTimerFeedback->setSingleShot(true);

#if defined(Q_OS_MAC)
    // Timer for clipboard check on Mac Os
    mpClipboardTimer = new QTimer(this);
    mpClipboardTimer->setInterval(500);
    mpClipboardTimer->stop();
    connect(mpClipboardTimer, SIGNAL(timeout()), this, SLOT(slot_copyFromClipboard()));
#endif
    // Document ocr text font
    mFontsize    = 8;
    msFontFamily = QString("Times New Roman");

    mpFile   = 0;
    miRetry  = 0;

    // Set flags
    mbWholeBookAvailable        = false;
    mbStopPdfMergingProcessFlag = true;
    mpThread                    = 0;
    mBatchJobActivated          = false;
    mLinkExportActivated        = false;
    mbTimerActive               = false;

    if (mpAutoUpdateAct->isChecked())
    {
        // Run auto update request
        slot_autoUpdateRequest();
    }

    // Init copy right codes and description according to hathitrust.org API
    msRightsCode << "unknown"
                 << "pd"
                 << "pd-pvt"
                 << "pd-us"
                 << "pd-google"
                 << "pd-us-google"
                 << "oa"
                 << "oa-google"
                 << "ic"
                 << "op"
                 << "orph"
                 << "und"
                 << "umall"
                 << "ic-world"
                 << "nobody"
                 << "pdus"
                 << "cc-by-3.0"
                 << "cc-by-nd-3.0"
                 << "cc-by-nc-nd-3.0"
                 << "cc-by-nc-3.0"
                 << "cc-by-nc-sa-3.0"
                 << "cc-by-sa-3.0"
                 << "orphcand"
                 << "cc-zero"
                 << "und-world"
                 << "icus"
                 << "ic-us"
                 << "ic-us-google"
                 << "cc-by-4.0"
                 << "cc-by-nd-4.0"
                 << "cc-by-nc-nd-4.0"
                 << "cc-by-nc-4.0"
                 << "cc-by-nc-sa-4.0"
                 << "cc-by-sa-4.0"
                 << "by-permission";

    msRightsCodeDscr << "unknown"
                     << "public domain"
                     << "public domain, privacy concerns"
                     << "public domain in the united states"
                     << "public domain, Google-digitized"
                     << "Public Domain in the United States, Google-digitized"
                     << "open access"
                     << "Open Access, Google-digitized"
                     << "in-copyright"
                     << "out-of-print (implies in-copyright)"
                     << "copyright-orphaned (implies in-copyright)"
                     << "undetermined copyright status"
                     << "available to UM affiliates and walk-in patrons (all campuses)"
                     << "in-copyright and permitted as world viewable by the copyright holder"
                     << "available to nobody; blocked for all users"
                     << "public domain only when viewed in the US"
                     << "Creative Commons Attribution license, 3.0 Unported"
                     << "Creative Commons Attribution-NoDerivatives license, 3.0 Unported"
                     << "Creative Commons Attribution-NonCommercial-NoDerivatives license, 3.0 Unported"
                     << "Creative Commons Attribution-NonCommercial license, 3.0 Unported"
                     << "Creative Commons Attribution-NonCommercial-ShareAlike license, 3.0 Unported"
                     << "Creative Commons Attribution-ShareAlike license, 3.0 Unported"
                     << "orphan candidate - in 90-day holding period (implies in-copyright)"
                     << "Creative Commons Zero license (implies public domain)"
                     << "undetermined copyright status and permitted as world viewable by the depositor"
                     << "in copyright in the US"
                     << "in copyright in the US"
                     << "in copyright in the US, Google-digitized"
                     << "Creative Commons Attribution 4.0 International license"
                     << "Creative Commons Attribution-NoDerivatives 4.0 International license"
                     << "Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International license"
                     << "Creative Commons Attribution-NonCommercial 4.0 International license"
                     << "Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International license"
                     << "Creative Commons Attribution-ShareAlike 4.0 International license"
                     << "Available by Permission";

    msRightsPublicDomainUs << "public domain in the united states"
                           << "Public Domain in the United States, Google-digitized"
                           << "public domain only when viewed in the US";


    // Check ssl support
    if(!QSslSocket::supportsSsl())
    {
        QMessageBox errorMessageBox(this);
        errorMessageBox.setIcon(QMessageBox::Warning);
        errorMessageBox.setWindowTitle("ERROR - MISSING SSL/TSL SUPPORT");
        errorMessageBox.setText("No SSL/TSL detected. SSL/TSL is required for https-requests.");
        errorMessageBox.addButton(QMessageBox::Ok);
        errorMessageBox.exec();
    }

    // Adjust application size at startup
    this->show();
    QSize widgetSize(this->size() + QSize(ui->scrollArea->horizontalScrollBar()->maximum(),ui->scrollArea->verticalScrollBar()->maximum()));
    QSize screenSize(QApplication::desktop()->screen()->size());
    this->resize(qMin(widgetSize.width(), (int)(screenSize.width()*0.9)), qMin(widgetSize.height(), (int)(screenSize.height()*0.8)));
    this->move(QApplication::desktop()->screen()->rect().center() - this->rect().center());
#if DISABLE_AUTOPROXY
    // HDH Version 1.1.1 - DISABLE WEBPROXY
    ui->checkBoxAutoProxy->setDisabled(true);
    ui->checkBoxAutoProxy->setChecked(false);
    ui->checkBoxAutoProxy->setCheckable(false);
#endif
#if DISABLE_WEBPROXY
    ui->checkBoxWebProxy->setDisabled(true);
    ui->checkBoxWebProxy->setChecked(false);
    ui->checkBoxWebProxy->setCheckable(false);
#endif

    if (mtUserSettings.sWhatsNewVersion != qApp->applicationVersion())
    {
        slot_showWhatsNew();
        slot_saveGuiSettings();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Init printer components
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::initPrinterComponents()
{
    // Workarround: Printer for user setup: required since QPageSetupDialog does not support QPrinter::PdfFormat
    int iMargin = 5; // mm
    mpSetupPrinter = new QPrinter;
    if (!setXmlPageSetup(mpSetupPrinter))
    {
        mpSetupPrinter->setOrientation(QPrinter::Portrait);
        mpSetupPrinter->setPageSize(QPrinter::A4);
        mpSetupPrinter->setPageMargins(iMargin,  iMargin, iMargin,  iMargin, QPrinter::Millimeter);
        mpSetupPrinter->setResolution(mpviDpi->at(ui->comboBoxZoom->currentIndex()));
    }
    mpPageSetup    = new QPageSetupDialog(mpSetupPrinter, this);

    // Setup pdf printer
    mpPrinter = new QPrinter;
    if (!setXmlPageSetup(mpPrinter))
    {
       mpPrinter->setOrientation(QPrinter::Portrait);
       mpPrinter->setPageSize(QPrinter::A4);
       mpPrinter->setPageMargins(iMargin,  iMargin, iMargin,  iMargin, QPrinter::Millimeter);
    }
    mpPrinter->setResolution(mpviDpi->at(ui->comboBoxPrinterRes->currentIndex()));
    mpPrinter->setOutputFormat(QPrinter::PdfFormat);
}


/*-----------------------------------------------------------------------------------------------*
 * Batchjob: Activate /  Deactivate proxy server
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_setProxyState(bool bState)
{
    // Check if proxy is already enabled
    if (ui->checkBoxUseProxy->isChecked())
    {
        // Disable proxy
        ui->checkBoxUseProxy->setChecked(false);
        QString sProxyState = ui->labelBookInfoState->text();
        on_checkBoxUseProxy_clicked(false);
        ui->labelBookInfoState->setText(sProxyState);
    }
#if !DISABLE_AUTOPROXY
    if (bState)
    {
        connect(mpAutoProxy, SIGNAL(updateAutoProxyState(QString)), ui->labelBookInfoState, SLOT(setText(QString)));

        // Re-Enable proxy
        ui->checkBoxUseProxy->setChecked(true);
        on_checkBoxUseProxy_clicked(true);
    }
#endif
}

/*-----------------------------------------------------------------------------------------------*
 * Network: AutoProxy State changed
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_autoProxyRequestCompleted(bool bState)
{
    slot_enablePushButtonGetBookInfo(true);
    if (!bState)
        slot_setProxyState(bState);

    if (mBatchJobActivated)
        emit proxyConnectionEstablished(bState);
}


/*-----------------------------------------------------------------------------------------------*
 * Enable / Disable AutoProxy checkbox
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_disableAutoProxy(bool bIsChecked)
{
    if (bIsChecked)
        ui->checkBoxAutoProxy->setChecked(false);
}

/*-----------------------------------------------------------------------------------------------*
 * Enable / Disable WebProxy checkbox
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_disableWebProxy(bool bIsChecked)
{
    if (bIsChecked)
        ui->checkBoxWebProxy->setChecked(false);
}

/*-----------------------------------------------------------------------------------------------*
 * Enable / Disable AutoProxy
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_enableAutoProxy(bool bIsChecked)
{
    qDebug() << "HDH: ENABLE AUTOPROXY " << bIsChecked;
    mpFileDownloader->enableAutoProxy(bIsChecked);
#if !ENABLE_PROXY_CHANGEOVER
    if (ui->pushButtonStartDownload->text().contains("cancel", Qt::CaseInsensitive))
        ui->checkBoxWebProxy->setDisabled(true);
#endif
}

/*-----------------------------------------------------------------------------------------------*
 * Enable / Disable WebProxy
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_enableWebProxy(bool bIsChecked)
{
    if (!mtUserSettings.bWebProxyNoteAccepted)
    {
         if(mpWebProxyNotification->exec())
         {
             // Set notification flag
             mtUserSettings.bWebProxyNoteAccepted = true;
             // save setting
             slot_saveGuiSettings();
         }
         else
         {
             disconnect(ui->checkBoxWebProxy, SIGNAL(toggled(bool)), this, SLOT(slot_enableWebProxy(bool)));
             ui->checkBoxWebProxy->setChecked(false);
             connect(ui->checkBoxWebProxy, SIGNAL(toggled(bool)), this, SLOT(slot_enableWebProxy(bool)));

             return;
         }
    }

    qDebug() << "HDH: ENABLE WEBPROXY " << bIsChecked;
    mpFileDownloader->enableWebProxy(bIsChecked);
    // TODO: added due to possible application crash on webproxy<->autoproxy toggle
#if !ENABLE_PROXY_CHANGEOVER
    if (ui->pushButtonStartDownload->text().contains("cancel", Qt::CaseInsensitive))
        ui->checkBoxAutoProxy->setDisabled(true);
#endif
}

/*-----------------------------------------------------------------------------------------------*
 * Apply pagesetup.xml configuration file
 *-----------------------------------------------------------------------------------------------*/
int HathiDownloadHelper::setXmlPageSetup(QPrinter *aPrinter)
{
    QString sPageSetup = readFile(QString("pagesetup.xml"));

    if (sPageSetup.isEmpty())
        return 0;

    bool ok;
    QString sTagValue  = myXmlReader(&sPageSetup, "paperSize");
    int iTagValue = sTagValue.toInt(&ok,10);
    if (ok)
    {
       aPrinter->setPaperSize((QPrinter::PaperSize) iTagValue);
    }
    sTagValue  = myXmlReader(&sPageSetup, "orientation");
    iTagValue = sTagValue.toInt(&ok,10);
    if (ok)
    {
        aPrinter->setOrientation((QPrinter::Orientation) iTagValue);
    }

    qreal left, top, right, bottom;
    sTagValue  = myXmlReader(&sPageSetup, "marginLeft");
    left = sTagValue.toDouble(&ok);

    sTagValue  = myXmlReader(&sPageSetup, "marginTop");
    top = sTagValue.toDouble(&ok);

    sTagValue  = myXmlReader(&sPageSetup, "marginRight");
    right = sTagValue.toDouble(&ok);

    sTagValue  = myXmlReader(&sPageSetup, "marginBottom");
    bottom = sTagValue.toDouble(&ok);

    if ( (left > 0) && (top > 0) && (right > 0) && (bottom > 0) )
    {
        aPrinter->setPageMargins(left, top, right, bottom, QPrinter::Point);
    }

    return ok;
}

/*-----------------------------------------------------------------------------------------------*
 * Load version file from dropbox for update check
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_loadVersionFile(void)
{
    // Stop update request if still active
    if( (mpUpdateReply != NULL) && (mpUpdateReply->isOpen()) ) {
        mpUpdateReply->close();
    }

    QUrl sUrl(QString("https://www.dropbox.com/s/r5xatuhbx04x37p/version.hdh?dl=1"));

    QNetworkRequest request(sUrl);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");
    // Change button
    slot_enablePushButtonGetBookInfo(false);

    // Start file download
    mbhttpRequestAborted    = false;
    if (mpUpdateReply != NULL)
        mpUpdateReply->deleteLater();
    mpUpdateReply           = mpNetwUpdateManager->get(request);
    mpRetryTimer->start(15000);
    connect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonCancelUpdate_clicked()));
    connect(mpUpdateReply, SIGNAL(finished()), this, SLOT(slot_checkUpdate()));

    ui->labelBookInfoState->setText(QString("Checking for update..."));
    slot_infiniteProgressBarsBookPage(true);

}

/*-----------------------------------------------------------------------------------------------*
 * Auto update check - send request
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_autoUpdateRequest(void)
{
    QUrl sUrl(QString("https://www.dropbox.com/s/r5xatuhbx04x37p/version.hdh?dl=1"));

    QNetworkRequest request(sUrl);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");

    // Start file download
    mbhttpRequestAborted    = false;
    if (mpUpdateReply != NULL)
        mpUpdateReply->deleteLater();
    mpUpdateReply           = mpNetwUpdateManager->get(request);
    connect(mpUpdateReply, SIGNAL(finished()), this, SLOT(slot_autoUpdateResponse()));

    mpRetryTimer->start(15000);
    connect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_autoUpdateResponse()));

    mpUpdateAct->setDisabled(true);
}

/*-----------------------------------------------------------------------------------------------*
 * Check for update - html loaded
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_autoUpdateResponse()
{
    mpUpdateAct->setEnabled(true);

    if (!mpRetryTimer->isActive())
        if (mpUpdateReply)
            mpUpdateReply->close();

    mpRetryTimer->stop();
    disconnect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_autoUpdateResponse()));
    disconnect(mpUpdateReply, SIGNAL(finished()), this, SLOT(slot_autoUpdateResponse()));

    QVariant redirectionTarget = mpUpdateReply->attribute(QNetworkRequest::RedirectionTargetAttribute);


    // On error
    if (mpUpdateReply->error())
    {
        QMessageBox::information(this, tr("Error"),tr("Check for update failed. [URL load failed! No response.]"));

        // Do nothing
        mpUpdateReply->deleteLater();
        mpUpdateReply  = 0;
    }
    else if(!redirectionTarget.isNull())
    {
        qDebug() << "HDH: AUTO UPDATE FOLLOWING REDIRECTION";

        // Follow redirect
        QNetworkRequest request(redirectionTarget.toUrl());
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");

        if (mpUpdateReply != NULL)
            mpUpdateReply->deleteLater();
        mpUpdateReply           = mpNetwUpdateManager->get(request);
        connect(mpUpdateReply, SIGNAL(finished()), this, SLOT(slot_autoUpdateResponse()));

        mpRetryTimer->start(15000);
        connect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_autoUpdateResponse()));

        mpUpdateAct->setDisabled(true);

    }
    else
    {
        QString sHtml = mpUpdateReply->readAll();

        if (!sHtml.isEmpty())
        {
            int pos = 0;
            // Version tags for identifying online version
            QString sHtmlVersion = myXmlReader(&sHtml,"version", &pos);

            if (!sHtmlVersion.isEmpty())
            {
                QString sHtmlVersionCompare = sHtmlVersion;
                QString sAppVersionCompare = qApp->applicationVersion();
                // Compare version tags
                if (sHtmlVersionCompare.remove(".").toInt() == qApp->applicationVersion().remove(".").toInt())
                {
                    // Do nothing
                }
                else if (sHtmlVersionCompare.remove(".").toInt() < sAppVersionCompare.remove(".").toInt())
                {
                    QMessageBox::information(this, tr("Error"),tr("Check for update failed. [version mismatch]" \
                                                                  "\n your version (v%1) > online version (v%2)").arg(qApp->applicationVersion()).arg(sHtmlVersion));
                }
                else
                {
                    QString sLinkToHomepage1Label   = myXmlReader(&sHtml,"homepage1label");
                    QString sLinkToHomepage1Url     = myXmlReader(&sHtml,"homepage1url");
                    QString sLinkToHomepage2Label   = myXmlReader(&sHtml,"homepage2label");
                    QString sLinkToHomepage2Url     = myXmlReader(&sHtml,"homepage2url");

                    // Extract source code download link
                    QString sLinkToSrcCd = myXmlReader(&sHtml,"linktosourcecode");

                    // Extract windows installer download link
#if defined(Q_OS_WIN)
                    QString sLinkToInstaller = myXmlReader(&sHtml,"linktowininstaller");
#elif defined(Q_OS_MAC)
                    QString sLinkToInstaller = myXmlReader(&sHtml,"linktomacosxinstaller");
#endif

                    // Create Message box
                    QString sMessage = QString("A new version is available: " \
                                               "<br><br>Homepage: <a href='%1'>Hahti Download Helper %2</a> on %3<br><br>" \
                                               "Homepage: <a href='%4' >Hahti Download Helper %2</a> on %5<br><br>" \
                                               "Download: <a href='%6'>source code</a><br><br>" \
                                               ).arg(sLinkToHomepage1Url).arg(sHtmlVersion).arg(sLinkToHomepage1Label)
                            .arg(sLinkToHomepage2Url).arg(sLinkToHomepage2Label)
                            .arg(sLinkToSrcCd);
                    // Append installer link

#if defined(Q_OS_WIN)
                    sMessage.append(QString("Download: <a href='%1'>Hathi Download Helper Installer v%2 for Windows</a>")
                                    .arg(sLinkToInstaller).arg(sHtmlVersion));
#elif defined(Q_OS_MAC)
                    sMessage.append(QString("Download: <a href='%1'>Hathi Download Helper Installer v%2 for Mac OS X</a>")
                                    .arg(sLinkToInstaller).arg(sHtmlVersion));
#endif
                    QMessageBox::information(this, tr("Information"),sMessage);
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Cancel update check
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_pushButtonCancelUpdate_clicked(void)
{
    ui->labelBookInfoState->setText(tr("Request canceled."));
    slot_infiniteProgressBarsBookPage(false);
    mbhttpRequestAborted = true;

    disconnect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonCancelUpdate_clicked()));
    mpRetryTimer->stop();

    if(mpUpdateReply != NULL)
        disconnect(mpUpdateReply, SIGNAL(finished()), this, SLOT(slot_checkUpdate()));

    if( (mpUpdateReply != NULL) && (mpUpdateReply->isOpen()) )
        mpUpdateReply->close();

    // Change button
    slot_enablePushButtonGetBookInfo(true);
}

/*-----------------------------------------------------------------------------------------------*
 * Check for update - html loaded, parse through
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_checkUpdate()
{
    qDebug() << "HDH: VERSION FILE LOADED";

    mpRetryTimer->stop();
    disconnect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonCancelUpdate_clicked()));
    disconnect(mpUpdateReply, SIGNAL(finished()), this, SLOT(slot_checkUpdate()));

    ui->labelBookInfoState->setText(QString(""));
    ui->progressBarBookPage->setMaximum(100);
    ui->progressBarBookPage->setValue(0);

    // Change button
    slot_enablePushButtonGetBookInfo(true);

    QVariant redirectionTarget = mpUpdateReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (mbhttpRequestAborted)
    {
        // Reset flag
        mbhttpRequestAborted = false;
        // Do nothing
    }
    else
    {
        // On error or redirection
        if (mpUpdateReply->error())
        {
            QMessageBox::information(this, tr("Error"),tr("Check for update failed. [URL load failed! No response.]"));

            mpUpdateReply->deleteLater();
            mpUpdateReply  = 0;
        }
        else if(!redirectionTarget.isNull())
        {
            qDebug() << "HDH: FOLLOWING REDIRECTION";

            // Follow redirect
            QNetworkRequest request(redirectionTarget.toUrl());
            request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");

            if (mpUpdateReply)
                mpUpdateReply->deleteLater();
            mpUpdateReply           = mpNetwUpdateManager->get(request);
            mpRetryTimer->start(15000);
            connect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonCancelUpdate_clicked()));
            connect(ui->pushButtonGetInfo, SIGNAL(clicked()), this, SLOT(slot_pushButtonCancelUpdate_clicked()));
            connect(mpUpdateReply, SIGNAL(finished()), this, SLOT(slot_checkUpdate()));

            // Change button
            slot_enablePushButtonGetBookInfo(false);

            ui->labelBookInfoState->setText(QString("Checking for update..."));
            slot_infiniteProgressBarsBookPage(true);

        }
        else
        {
            QString sHtml = mpUpdateReply->readAll();

            if (sHtml.isEmpty())
            {
                QMessageBox::information(this, tr("Error"),tr("Check for update failed. [URL load failed! Received empty document.]"));
            }
            else
            {
                int pos = 0;
                // Version tags for identifying online version
                QString sHtmlVersion = myXmlReader(&sHtml,"version", &pos);

                if (sHtmlVersion.isEmpty())
                {
                    QMessageBox::information(this, tr("Error"),tr("Check for update failed. [Can't find version tag]"));
                }
                else
                {
                    QString sHtmlVersionCompare = sHtmlVersion;
                    QString sAppVersionCompare = qApp->applicationVersion();
                    // Compare version tags
                    if (sHtmlVersionCompare.remove(".").toInt() == qApp->applicationVersion().remove(".").toInt())
                    {
                        QMessageBox::information(this, tr("Information"),tr("Congratulations! You are running the latest version."));
                    }
                    else if (sHtmlVersionCompare.remove(".").toInt() < sAppVersionCompare.remove(".").toInt())
                    {
                        QMessageBox::information(this, tr("Error"),tr("Check for update failed. [version mismatch]" \
                            "\n your version (v%1) > online version (v%2)").arg(qApp->applicationVersion()).arg(sHtmlVersion));
                    }
                    else
                    {
                        QString sLinkToHomepage1Label   = myXmlReader(&sHtml,"homepage1label");
                        QString sLinkToHomepage1Url     = myXmlReader(&sHtml,"homepage1url");
                        QString sLinkToHomepage2Label   = myXmlReader(&sHtml,"homepage2label");
                        QString sLinkToHomepage2Url     = myXmlReader(&sHtml,"homepage2url");

                        // Extract source code download link
                         QString sLinkToSrcCd = myXmlReader(&sHtml,"linktosourcecode");

                        // Extract windows installer download link

#if defined(Q_OS_WIN)
                         QString sLinkToInstaller = myXmlReader(&sHtml,"linktowininstaller");
#elif defined(Q_OS_MAC)
                         QString sLinkToInstaller = myXmlReader(&sHtml,"linktomacosxinstaller");
#endif
                         // Create Message box
                        QString sMessage = QString("A new version is available: " \
                                                   "<br><br>Homepage: <a href='%1'>Hahti Download Helper %2</a> on %3<br><br>" \
                                                   "Homepage: <a href='%4' >Hahti Download Helper %2</a> on %5<br><br>" \
                                                   "Download: <a href='%6'>source code</a><br><br>" \
                                                   ).arg(sLinkToHomepage1Url).arg(sHtmlVersion).arg(sLinkToHomepage1Label)
                                                    .arg(sLinkToHomepage2Url).arg(sLinkToHomepage2Label)
                                                    .arg(sLinkToSrcCd);
                        // Append installer link
#if defined(Q_OS_WIN)
                        sMessage.append(QString("Download: <a href='%1'>Hathi Download Helper Installer v%2 for Windows</a>")
                                         .arg(sLinkToInstaller).arg(sHtmlVersion));
#elif defined(Q_OS_MAC)
                        sMessage.append(QString("Download: <a href='%1'>Hathi Download Helper Installer v%2 for Mac OS X</a>")
                                         .arg(sLinkToInstaller).arg(sHtmlVersion));
#endif
                        QMessageBox::information(this, tr("Information"),sMessage);
                    }
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Load hathitrust html page with book information
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_pushButtonGetInfo_clicked()
{
    // Stop update request if still active
    if( (mpUpdateReply != NULL) && (mpUpdateReply->isOpen()) ){
           mpUpdateReply->close();
    }

    // Check for permament link URL
    on_lineEditBookURL_editingFinished();
    disconnect(mpAutoProxy, SIGNAL(finished(bool)), this, SLOT(slot_restartBookInfoRequestOnProxyFailure(bool)));

    QString sUrl = ui->lineEditBookURL->text();

    // Reset labels
    ui->labelBookTitleText->setText(QString("-"));
    ui->labelNumPagesText->setText(QString("-"));
    ui->labelBookIDText->setText(QString("-"));
    ui->labelAuthorText->setText(QString("-"));
    ui->labelPublishedText->setText(QString("-"));
    ui->labelCopyrightText->setText(QString("-"));
    if (!mBatchJobActivated)
        ui->pushButtonStartDownload->setEnabled(false);

        // Check if ID only was entered
    if ( !sUrl.isEmpty() && !sUrl.contains(QString("hathitrust.org")) && !sUrl.contains(QString("hdl.handle.net")) )
    {
        sUrl.prepend(QString("https://babel.hathitrust.org/cgi/pt?id="));
        sUrl.append(QString(";"));
        ui->lineEditBookURL->setText(sUrl);
    }

    if ( (sUrl.contains(QString("hathitrust.org")) && sUrl.contains(QString("id=")) ) || sUrl.contains(QString("hdl.handle.net")) )
    {
        if (!sUrl.contains(QString("http")) )
        {
            sUrl.prepend(QString("https://"));
        }
        if (!sUrl.contains(QString("http://")) )
        {
            sUrl.replace(QString("http://"), QString("https://"));
        }

        if (sUrl.contains(QChar('%')))
             sUrl = QUrl::fromPercentEncoding ( sUrl.toLocal8Bit() );

        QUrl bookUrl(sUrl);
        QNetworkRequest requestBook(bookUrl);
        requestBook.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");
        if (mpBookInfoReply != NULL)
            mpBookInfoReply->deleteLater();

        mpBookInfoReply = mpNetwBookInfoManager->get(requestBook);
        connect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonGetInfoCancel_clicked()));
        mpRetryTimer->start(15000);

        ui->progressBarBookPage->setValue(25);
        ui->progressBarBookPage->setMaximum(100);
        ui->labelBookInfoState->setText(QString("Loading URL ..."));

        // Change button
        slot_enablePushButtonGetBookInfo(false);

    }
    else
    {
        ui->labelBookInfoState->setText(QString("Invalid URL ..."));
        ui->lineEditBookURL->setText(QString(""));
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Cancel load html page action
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_pushButtonGetInfoCancel_clicked()
{
    qDebug() << "HDH: GET BOOK INFO REQUEST CANCELED";

    // Check if user wants to stop batchjob
    if (mBatchJobActivated)
    {
        // Get sender object and name
        QObject *senderObj = sender();
        QString senderObjName = senderObj->objectName();

        // Send cancel command to batch job
        if (!senderObjName.isEmpty())
        {
            emit cancelBatchJob();

            qDebug() << "HDH: BATCHJOB CANCELED BY USER";

            return;
        }
    }

    bool bRestartProxy = false;

    if (mpRetryTimer->isActive() || mpAutoProxy->getProxyBusyFlagState())
    {
        mpRetryTimer->stop();
        ui->labelBookInfoState->setText(QString("Http request canceled...."));
        qDebug() << "HDH: Http request canceled....";
    }
    else
    {
        ui->labelBookInfoState->setText(QString("Http request failed...."));
        qDebug() << "HDH: Http request failed....";
        if (ui->checkBoxUseProxy->isChecked())
            bRestartProxy = true;
    }

    // Reset progress par
    ui->progressBarBookPage->setValue(0);
    disconnect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonGetInfoCancel_clicked()));

    if( (mpBookInfoReply != NULL) && (mpBookInfoReply->isOpen()) )
    {
        // Stop download request
        mpBookInfoReply->abort();
    }

    if( (mpUpdateReply != NULL) && (mpUpdateReply->isOpen()) )
    {
        // Stop download request
        mpUpdateReply->abort();
    }

    if (mLinkExportActivated)
    {
        mLinkExportActivated = false;
        disconnect(this, SIGNAL(bookInfoLoaded(bool)), this, SLOT(slot_createLinksFile(bool)));
    }

    // Cancel auto proxy request when proxy is busy and disable proxy
    if (mpAutoProxy->getProxyBusyFlagState())
    {
        qDebug() << "HDH: PROXY DISABLED";

        mpAutoProxy->abortAutoProxyRequest();
        ui->checkBoxUseProxy->setChecked(false);
        mpAutoProxy->enableLocalHost(true);
        mpProxy->setType(QNetworkProxy::NoProxy);
        QNetworkProxy::setApplicationProxy(*mpProxy);
        ui->labelBookInfoState->setText(QString("Proxy disabled!"));
    }

    // Change button
    slot_enablePushButtonGetBookInfo(true);

    // Check for update request
    if (mpUpdateReply != NULL)
        slot_pushButtonCancelUpdate_clicked();

    disconnect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonCancelUpdate_clicked()));
    disconnect(mpNetwBookInfoManager, SIGNAL(finished(QNetworkReply*)), mpAutoProxy, SLOT(slot_extractProxyList(QNetworkReply*)));
    disconnect(mpNetwBookInfoManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_extractBookCopyright(QNetworkReply*)));
    disconnect(mpNetwBookInfoManager, SIGNAL(finished(QNetworkReply*)), mpAutoProxy, SLOT(slot_verifyProxyServerConnection(QNetworkReply*)));
    connect(mpNetwBookInfoManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_bookPageLoaded(QNetworkReply*)));

    // Restart proxy automatically
    if (bRestartProxy && !mBatchJobActivated)
        slot_setProxyState(bRestartProxy);

    if (mBatchJobActivated)
        emit bookInfoLoaded(false);
}

/*-----------------------------------------------------------------------------------------------*
 * Handle Ssl errors on book info request
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_handleSslErrors(QNetworkReply* pReply, const QList<QSslError> &errors)
{
    qDebug() << "HDH: SSL ERROR DETECTED";
    foreach (QSslError e, errors)
    {
        qDebug() << "SSL ERROR: " << e;
    }
    pReply->ignoreSslErrors();
}

/*-----------------------------------------------------------------------------------------------*
 * Extract document ID, number of pages and title from html page
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_bookPageLoaded(QNetworkReply* pReply)
{
    qDebug() << "HDH: BOOK PAGE LOADED  " << mpBookInfoReply->url();

    mpRetryTimer->stop();
    disconnect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonGetInfoCancel_clicked()));

    // Change button
    slot_enablePushButtonGetBookInfo(true);

    // Check received download reply
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (        pReply->error()                                                             // error
            || !redirectionTarget.isNull()                                                  // redirection
            ||  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty())    // empty page
    {

        qDebug() << " pReply->error() " <<  pReply->error() ;
        qDebug() << " redirectionTarget    " <<  redirectionTarget.toString();
        qDebug() << " pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty()) " <<  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty() ;

        ui->progressBarBookPage->setValue(0);
        if (pReply->error() == QNetworkReply::OperationCanceledError)
            ui->labelBookInfoState->setText(QString("Http request canceled...."));
        else
            ui->labelBookInfoState->setText(QString("Http request failed...."));

        // Signals for batch job handling
        if (mBatchJobActivated || mLinkExportActivated)
        {
            emit bookInfoLoaded(false);
        }

        return;
    }

    ui->progressBarBookPage->setValue(50);
    ui->labelBookInfoState->setText(QString("Extracting document title ..."));

    mpBookPage->mainFrame()->setHtml(pReply->readAll());

    QWebElement document = mpBookPage->mainFrame()->documentElement();

    if (    (pReply->url().toString().contains("hathitrust"))
         || (pReply->url().toString().contains("hdl.handle.net")) )
    {
        // Check restriction
        mbDocumentAvailable = checkBookRestriction(&document);

        // Extract book title and page number
        QString sDocumentID, sTitle, sAuthor, sPublisher, sDocumentCopyrightCode;
        int iMaxPageNum      = 0;
        bool bPublicDominInUsOnly = false;

        extractDocumentID(&document, &sDocumentID, &sDocumentCopyrightCode, &iMaxPageNum, eHATHITRUST);
        msDocumentID    = sDocumentID;

        if ( (!sDocumentID.isEmpty()) && (iMaxPageNum > 0) )
            mbEmptyDocument = false;
        else
            mbEmptyDocument = true;

        // Extract book title
        sTitle = extractBookTitle(mpBookPage->mainFrame(), &document);

        // Extract Auhtor
        sAuthor = extractBookAuthor(&document, eHATHITRUST);

        // Extract publisher
        sPublisher = extractBookPublisher(&document, eHATHITRUST);

        // Check for "whole book" download link available
        mbWholeBookAvailable = checkDownloadWholeBook(&document);

        // Show results
        if (sTitle.isEmpty())
            sTitle = "-";
        ui->labelBookTitleText->setText(sTitle);
        ui->labelNumPagesText->setText(QString::number(iMaxPageNum));
        if (sDocumentID.isEmpty())
            sDocumentID = "-";
        ui->labelBookIDText->setText(sDocumentID);
        if (sAuthor.isEmpty())
            sAuthor = "-";
        ui->labelAuthorText->setText(sAuthor);
        if (sPublisher.isEmpty())
            sPublisher = "-";
        ui->labelPublishedText->setText(sPublisher);

        ui->labelCopyrightText->setText(msRightsCodeDscr.at(msRightsCode.indexOf(sDocumentCopyrightCode)));

        // Alternative information resource - tested with copyright
        // Request json-data for book information
        // Request copyright licence information
        //getBookCopyright();

        // Signals for batch job handling
        if (mBatchJobActivated)
        {
            emit bookInfoCopyright(msRightsCodeDscr.at(msRightsCode.indexOf(sDocumentCopyrightCode)));
        }

//        if (    (ui->labelCopyrightText->text().contains("public domain only when viewed in the US", Qt::CaseInsensitive))
//            || (ui->labelCopyrightText->text().contains("Public Domain in the United States", Qt::CaseInsensitive)) )

        if (msRightsPublicDomainUs.contains(ui->labelCopyrightText->text(), Qt::CaseInsensitive))
        {
            mpAutoProxy->setUsProxyOnlyState(true);
            mpProxyDialog->setUsProxyOnlyState(true);
            bPublicDominInUsOnly = true;
        }

        // When document is not available
        if (!mbDocumentAvailable)
        {
            // Enable AutoProxy
            if ( !ui->checkBoxUseProxy->isChecked()
                 &&  mpAutoProxy->getAutoProxyState()
                 &&  mpAutoProxy->getUsProxyOnlyState()
                 && bPublicDominInUsOnly
                 && mtUserSettings.bEnableAutoProxyActivation )
            {
                ui->checkBoxUseProxy->setChecked(true);
            }

            // Connect with new proxy and refresh book info when proxy is in use and AutoProxy is enabled
            if( ui->checkBoxUseProxy->isChecked()
                && mpAutoProxy->getAutoProxyState()
                && !mBatchJobActivated
                && bPublicDominInUsOnly
               /*Always find a valid proxy: && mtUserSettings.bEnableAutoProxyActivation*/)
            {
                connect(mpAutoProxy, SIGNAL(finished(bool)), this, SLOT(slot_restartBookInfoRequestOnProxyFailure(bool)));
                on_checkBoxUseProxy_clicked(true);
            }

            // Signals for batch job handling
            if (mBatchJobActivated || mLinkExportActivated)
            {
                emit bookInfoLoaded(false);
            }

            ui->progressBarBookPage->setValue(0);
            if (mtUserSettings.bEnableAutoProxyActivation)
                if (bPublicDominInUsOnly)
                    ui->labelBookInfoState->setText(QString("Document not available...starting AutoProxy"));
                else
                    ui->labelBookInfoState->setText(QString("Document not available..."));
            else
                ui->labelBookInfoState->setText(QString("Document not available... <font color='blue'>please enable proxy server and retry</font>"));

            if (!mBatchJobActivated)
                ui->pushButtonStartDownload->setEnabled(false);
        }
        else
        {
            // When document ID and page numbers are avaiable = document is not in copyright
            if ( !mbEmptyDocument )
            {
                 ui->progressBarBookPage->setValue(100);

                if (!mBatchJobActivated)
                    ui->pushButtonStartDownload->setEnabled(true);

                miMaxPageNum    = iMaxPageNum;
                miRetry         = 0;

                if (mbWholeBookAvailable && mtUserSettings.bDownloadWholeBook)
                    ui->lineEditPages->setText(QString("Download whole book"));
                else
                    ui->lineEditPages->setText(QString("1-%1").arg(iMaxPageNum));

                ui->progressBarDownload->setValue(0);
                ui->progressBarDownload->setMaximum(miMaxPageNum);

                bool bReplaceSpareByTitle = false;
                if (ui->lineEditDstFolder->text().contains("[bookTitle]", Qt::CaseInsensitive))
                    bReplaceSpareByTitle =true;

                bool bReplaceSpareById = false;
                if (ui->lineEditDstFolder->text().contains("[bookID]", Qt::CaseInsensitive))
                    bReplaceSpareById =true;

                ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().remove("[bookTitle]/", Qt::CaseInsensitive));
                ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().remove("[bookID]/", Qt::CaseInsensitive));
                on_lineEditDstFolder_editingFinished();

                // Set auto download folder
                if (mtUserSettings.bCreateDownloadFolder || bReplaceSpareByTitle || bReplaceSpareById)
                {
                    if ((bReplaceSpareByTitle || mtUserSettings.bCreateDownloadFolderByTitle) && sTitle.length() > 0)
                    {
                        removeReservedCharFromString(&sTitle);
                        if (!ui->lineEditDstFolder->text().contains(sTitle.mid(0,mtUserSettings.iTitleChars), Qt::CaseInsensitive))
                        {
                            QString sPathTitle = ui->lineEditDstFolder->text() + sTitle.mid(0,mtUserSettings.iTitleChars);
                            ui->lineEditDstFolder->setText(sPathTitle.mid(0,MAX_LENGTH_PATH));
                        }
                    }
                    else
                    {
                        removeReservedCharFromString(&sDocumentID);
                        if (!ui->lineEditDstFolder->text().contains(sDocumentID,Qt::CaseInsensitive))
                        {
                            QString sPathID = ui->lineEditDstFolder->text() + sDocumentID;
                            ui->lineEditDstFolder->setText(sPathID.mid(0,MAX_LENGTH_PATH));
                        }
                    }

                }
                else
                {
                     ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().mid(0,MAX_LENGTH_PATH));
                }
                on_lineEditDstFolder_editingFinished();
            }
            // Document is in copyright
            else
            {
                ui->progressBarBookPage->setValue(0);
                ui->labelBookInfoState->setText(QString("Received empty document..."));
                if (!mBatchJobActivated)
                    ui->pushButtonStartDownload->setEnabled(false);
            }

            ui->progressBarBookPage->setValue(100);
            ui->labelBookInfoState->setText(QString("Done."));

            // Signals for batch job handling
            if (mBatchJobActivated || mLinkExportActivated)
            {
                emit bookInfoLoaded(!mbEmptyDocument);
            }
        }
    }
}



/*-----------------------------------------------------------------------------------------------*
 * Rest bookinfo page load on proxy failure
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_restartBookInfoRequestOnProxyFailure(bool bState)
{
    if (bState)
    {
        disconnect(mpAutoProxy, SIGNAL(finished(bool)), this, SLOT(slot_restartBookInfoRequestOnProxyFailure(bool)));
        on_pushButtonGetInfo_clicked();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Extract document title from html source code
 *-----------------------------------------------------------------------------------------------*/
QString HathiDownloadHelper::extractBookTitle(QWebFrame *pFrame, QWebElement *pDocument)
{
    // Extract first title part given in html title
    QString sTitle       = QString::fromUtf8(pFrame->title().toLocal8Bit());

    // Remove hathitrust.org label infromation
    sTitle               = sTitle.left(sTitle.indexOf(QString("..")));
    sTitle               = sTitle.left(sTitle.indexOf(QString(". .")));
    sTitle               = sTitle.left(sTitle.indexOf(QString("- Full View")));

    // Extract full document title given in one of the "span" tags
    pDocument->findFirst(sTitle);
    QWebElementCollection documentSpans = pDocument->findAll(QString("span"));
    QString sTemp;

    // Check all "span" tags to find full title
    for ( int i = 0; i < documentSpans.count(); i++ )
    {
        sTemp = documentSpans.at(i).attribute(QString("content"));

        if ( sTemp.contains(sTitle) )
        {
            sTitle = sTemp;
            break;
        }
    }

    removeReservedCharFromString(&sTitle);

    return sTitle;
}

/*-----------------------------------------------------------------------------------------------*
 * Check restriction of requested book
 *-----------------------------------------------------------------------------------------------*/
bool HathiDownloadHelper::checkBookRestriction(QWebElement *pDocument)
{
    // Extract "div" tags
    QWebElementCollection documentSpans = pDocument->findAll(QString("div"));
    QString sTemp;
    bool bAvailable =  true;

    // Check all "div" tags to find restriction tag
    for ( int i = 0; i < documentSpans.count(); i++ )
    {
        sTemp = documentSpans.at(i).toPlainText();

        if ( sTemp.contains(QString("not available online")) )
        {
            bAvailable = false;
            break;
        }
    }

    return bAvailable;
}

/*-----------------------------------------------------------------------------------------------*
 * Request copyright/restriction from hathitrust.org
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::getBookCopyright(void)
{
    qDebug() << "HDH: REQUESTING COPYRIGHT";

    // Change button
    slot_enablePushButtonGetBookInfo(false);

    disconnect(mpNetwBookInfoManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_bookPageLoaded(QNetworkReply*)));
    connect(mpNetwBookInfoManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_extractBookCopyright(QNetworkReply*)));

    QString sUrl = QString("https://catalog.hathitrust.org/api/volumes/brief/htid/%1.json").arg(msDocumentID);
    QUrl bookUrl(sUrl);
    QNetworkRequest requestBook(bookUrl);
    requestBook.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");

    if (mpBookInfoReply != NULL)
        mpBookInfoReply->deleteLater();
    mpBookInfoReply = mpNetwBookInfoManager->get(requestBook);
    mpRetryTimer->start(15000);
    connect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonGetInfoCancel_clicked()));

    ui->progressBarBookPage->setValue(75);
    ui->labelBookInfoState->setText(QString("Requesting copyright attributes ..."));
}

/*-----------------------------------------------------------------------------------------------*
 * Extract copyright/restriction from hathitrust.org response
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_extractBookCopyright(QNetworkReply* pReply)
{
    qDebug() << "HDH: EXTRACTING COPYRIGHT";

    bool bPublicDominInUsOnly = false;

    mpRetryTimer->stop();
    disconnect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_pushButtonGetInfoCancel_clicked()));
    disconnect(mpNetwBookInfoManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_extractBookCopyright(QNetworkReply*)));
    connect(mpNetwBookInfoManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_bookPageLoaded(QNetworkReply*)));

    // Check received download reply
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (        pReply->error()                                                             // error
            || !redirectionTarget.isNull()                                                  // redirection
            ||  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty())    // empty page
    {
        qDebug() << "HDH: EXTRACTING COPYRIGHT FAILED";  
        qDebug() << " pReply->error() " <<  pReply->error() ;
        qDebug() << " redirectionTarget    " <<  redirectionTarget.toString();
        qDebug() << " pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty()) " <<  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty() ;

        ui->progressBarBookPage->setValue(0);
        if (pReply->error() == QNetworkReply::OperationCanceledError)
            ui->labelBookInfoState->setText(QString("Http request canceled...."));
        else
            ui->labelBookInfoState->setText(QString("Http request failed...."));

        mpRetryTimer->start(15000);
        slot_pushButtonGetInfoCancel_clicked();

        return;
    }

    // Change button
    slot_enablePushButtonGetBookInfo(true);

    QString sBookInfos = pReply->readAll();

    // Check copyright code table
    QString sTagValue = extractValueFromJSON(&sBookInfos, "\"rightsCode\":\"");
    if (!sTagValue.isEmpty() && msRightsCode.indexOf(sTagValue) > -1)
    {
         ui->labelCopyrightText->setText(msRightsCodeDscr.at(msRightsCode.indexOf(sTagValue)));
         // Signals for batch job handling
         if (mBatchJobActivated)
         {
             emit bookInfoCopyright(msRightsCodeDscr.at(msRightsCode.indexOf(sTagValue)));
         }
    }
    else
        ui->labelCopyrightText->setText(QString("unknown"));

    qDebug() << "HDH: COPYRIGHT STATE: " << ui->labelCopyrightText->text();

//    if (    (ui->labelCopyrightText->text().contains("public domain only when viewed in the US", Qt::CaseInsensitive))
//         || (ui->labelCopyrightText->text().contains("Public Domain in the United States", Qt::CaseInsensitive)) )
    if (msRightsPublicDomainUs.contains(ui->labelCopyrightText->text(), Qt::CaseInsensitive))
    {
        mpAutoProxy->setUsProxyOnlyState(true);
        mpProxyDialog->setUsProxyOnlyState(true);
        bPublicDominInUsOnly = true;
    }

    if(mbDocumentAvailable)
    {
        ui->progressBarBookPage->setValue(100);
        ui->labelBookInfoState->setText(QString("Done."));

        // Signals for batch job handling
        if (mBatchJobActivated || mLinkExportActivated)
        {
            emit bookInfoLoaded(!mbEmptyDocument);
        }
    }
    else
    {
        if (!msDocumentID.isEmpty())
            ui->labelBookIDText->setText(msDocumentID);

        sTagValue =  extractValueFromJSON(&sBookInfos, "\"titles\":[\"");
        if (!sTagValue.isEmpty())
            ui->labelBookTitleText->setText(sTagValue);

        sTagValue =  extractValueFromJSON(&sBookInfos, "\"publishDates\":[\"");
        if (!sTagValue.isEmpty())
            ui->labelPublishedText->setText(sTagValue);

        // Enable AutoProxy
        if ( !ui->checkBoxUseProxy->isChecked()
             &&  mpAutoProxy->getAutoProxyState()
             &&  mpAutoProxy->getUsProxyOnlyState()
             && bPublicDominInUsOnly
             && mtUserSettings.bEnableAutoProxyActivation )
        {
            ui->checkBoxUseProxy->setChecked(true);
        }

        // Connect with new proxy and refresh book info when proxy is in use and AutoProxy is enabled
        if( ui->checkBoxUseProxy->isChecked()
            && mpAutoProxy->getAutoProxyState()
            && !mBatchJobActivated
            && bPublicDominInUsOnly
            && mtUserSettings.bEnableAutoProxyActivation)
        {
            connect(mpAutoProxy, SIGNAL(finished(bool)), this, SLOT(slot_restartBookInfoRequestOnProxyFailure(bool)));
            on_checkBoxUseProxy_clicked(true);
        }

        // Signals for batch job handling
        if (mBatchJobActivated || mLinkExportActivated)
        {
            emit bookInfoLoaded(false);
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Extract tag from JSON return structure of hathitrust.org
 *-----------------------------------------------------------------------------------------------*/
QString HathiDownloadHelper::extractValueFromJSON(QString* psText, QString sTag)
{
    int iStartIndex = -1;
    int iStopIndex = -1;
    iStartIndex = psText->indexOf(sTag);

    if (iStartIndex > -1)
    {
        iStartIndex += sTag.length();
        iStopIndex = psText->indexOf("\"", iStartIndex);
        if (iStopIndex > -1)
            return psText->mid(iStartIndex,iStopIndex - iStartIndex);
    }

    return ("");
}

/*-----------------------------------------------------------------------------------------------*
 * Extract document author from html source code
 *-----------------------------------------------------------------------------------------------*/
QString HathiDownloadHelper::extractBookAuthor(QWebElement *pDocument, int eSource)
{
    QString sTemp, sAuthor;

    if (eSource == eHATHITRUST)
    {
        // Extract "span" tags
        QWebElementCollection documentSpans = pDocument->findAll(QString("span"));

        // Check all "span" tags to find creator tag
        for ( int i = 0; i < documentSpans.count(); i++ )
        {
            sTemp = documentSpans.at(i).attribute(QString("property"));

            if ( sTemp.contains(QString("creator")) )
            {
                sAuthor = QString::fromUtf8(documentSpans.at(i).attribute(QString("content")).toLocal8Bit());
                break;
            }
        }
    }

    return sAuthor;
}

/*-----------------------------------------------------------------------------------------------*
 * Extract document pubisher from html source code
 *-----------------------------------------------------------------------------------------------*/
QString HathiDownloadHelper::extractBookPublisher(QWebElement *pDocument, int eSource)
{
            QString sTemp, sPublisher;

    if (eSource == eHATHITRUST)
    {
        // Extract "span" tags
        QWebElementCollection documentSpans = pDocument->findAll(QString("span"));

        // Check all "span" tags to find creator tag
        for ( int i = 0; i < documentSpans.count(); i++ )
        {
            sTemp = documentSpans.at(i).attribute(QString("property"));

            if ( sTemp.contains(QString("publisher")) )
            {
                sPublisher = documentSpans.at(i).attribute(QString("content"));;
                break;
            }
        }
    }

    return sPublisher;
}

/*-----------------------------------------------------------------------------------------------*
 * Remove reserved characters from string
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::removeReservedCharFromString(QString *psInput, QString *psOutput)
{
    if (!psOutput)
    {
       psOutput = psInput;
    }

    // Remove reserved characters from string
    QString sReservedChar("[<>:\"\\/?*|^']");
    psOutput->remove( QRegExp(sReservedChar, Qt::CaseInsensitive ) );
}

/*-----------------------------------------------------------------------------------------------*
 * Extract document ID and number of pages from html source code
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::extractDocumentID(QWebElement *pdocument, QString *psDocumentID, QString *psDocumentCopyright, int *piMaxPageNum, int eSource)
{
    // Extract document ID and max page nu,ber from document links
    ui->progressBarBookPage->setValue(75);
    ui->labelBookInfoState->setText(QString("Extracting document ID ..."));

    // Extract all links identified by their "a" tag
    QWebElementCollection documentLinks = pdocument->findAll(QString("a"));
    QString sTemp, sTag;

    int iStartIndex, iStopIndex =0, iCurrPage;

    *psDocumentCopyright = "unknown";

    // Check all links
    for ( int i = 0; i < documentLinks.count(); i++ )
    {
        sTemp = documentLinks.at(i).attribute(QString("href"));
        sTemp.append(";");

        if (psDocumentID->isEmpty())
        {
            // Identify document ID number
            if ( sTemp.contains(QString("?id=")) )
            {
                iStartIndex  = sTemp.indexOf(QString("?id=")) + (int) 4;
                if (eSource == eHATHITRUST)
                    iStopIndex   = sTemp.indexOf(QString(";"), iStartIndex);
                *psDocumentID = sTemp.mid(iStartIndex,iStopIndex-iStartIndex);
            }
        }

        // Extract page number and copyright
        ui->progressBarBookPage->setValue(90);
        ui->labelBookInfoState->setText(QString("Extracting page number ..."));

        if (eSource == eHATHITRUST)
        {   // Extract page number
            if ( sTemp.contains(QString("seq=")) )
            {
                iStartIndex = sTemp.indexOf(QString("seq=")) + (int) 4;
                iStopIndex  = sTemp.indexOf(";", iStartIndex);
                iCurrPage   = sTemp.mid(iStartIndex,iStopIndex-iStartIndex).toInt();

                // Link to last page has largest page number
                if ( iCurrPage > *piMaxPageNum )
                {
                    *piMaxPageNum = iCurrPage;
                }
            }
            // Extract copyright
            if ( sTemp.contains(QString("access_use#")) )
            {
                iStartIndex = sTemp.indexOf(QString("_use#")) + (int) 5;
                iStopIndex  = sTemp.indexOf(";", iStartIndex);
                sTag        = sTemp.mid(iStartIndex,iStopIndex-iStartIndex);

                if (!sTag.isEmpty() && msRightsCode.indexOf(sTag) > -1)
                {
                    *psDocumentCopyright = sTag;
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Check if "download whole book" is available
 *-----------------------------------------------------------------------------------------------*/
bool HathiDownloadHelper::checkDownloadWholeBook(QWebElement *pdocument)
{
    // Extract link "download whole book" identified by their "a" tag and attribute
    QWebElementCollection documentLinks = pdocument->findAll(QString("a[rel=allow]"));
    bool bWholeBookAvailable = true;

    if(documentLinks.count() == 0)
        bWholeBookAvailable = false;

    return bWholeBookAvailable;
}


/*-----------------------------------------------------------------------------------------------*
 * Remove illegal characters from page selection string
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::removeIllegalChar(QString *psPages)
{
    // Check for "Download whole book" tag
    if (psPages->contains("Download whole book") && mbWholeBookAvailable && mtUserSettings.bDownloadWholeBook)
    {
        // Do nothing
    }
    else
    {
        // Remove blank characters at first
        psPages->remove(QChar(' '));

        if (!psPages->isEmpty())
        {
            // Check for allowed characters: numbers 0-9, separators ',' and '-'
            for(int i = 0; i < psPages->length(); i++)
            {
                if ( (!psPages->at(i).isDigit()) && (psPages->at(i) != QChar(',')) && (psPages->at(i) != QChar('-')) )
                    psPages->replace(i,1,QChar(' '));
            }

            // Remove blank characters at first
            psPages->remove(QChar(' '));

            // Remove illegal symbol combinations
            while(psPages->contains(QString(",,")) || psPages->contains(QString("-,")) || psPages->contains(QString("-,")) || psPages->contains(QString("--") ))
            {
                while (psPages->contains(QString(",,")))
                    psPages->replace(QString(",,"),QString(","));
                while (psPages->contains(QString(",-")))
                    psPages->replace(QString(",-"),QString(","));
                while (psPages->contains(QString("-,")))
                    psPages->replace(QString("-,"),QString(","));
                while (psPages->contains(QString("--")))
                    psPages->replace(QString("--"), QString("-"));
            }

            if (!psPages->isEmpty())
            {
                // Remove leading separator symbols
                while( (psPages->length() > 0) &&  ( (psPages->at(0) == QChar(',')) || (psPages->at(0) == QChar('-')) ) )
                {
                    if (psPages->length() > 0)
                    {
                        psPages->remove(0,1);
                    }
                    else
                    {
                        break;
                    }
                }

                // Remove tailing separator symbols
                while( (psPages->length() > 0)  && ( (psPages->at(psPages->length()-1) == QChar(',')) || (psPages->at( psPages->length()-1) == QChar('-')) ) )
                {
                    if (psPages->length() > 0)
                    {
                        psPages->remove(psPages->length()-1,1);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        if(psPages->isEmpty() && !msDocumentID.isEmpty())
            *psPages = QString("1-%1").arg(QString::number(miMaxPageNum));
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Start download of document pages (pdf, images)
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_pushButtonStartDownload_clicked()
{
    mbhttpRequestAborted = false;

    QString sUrl = ui->lineEditBookURL->text();
    slot_removeLinkFromCollectorList(sUrl);

    mpFileDownloader->startDownload(    ui->lineEditBookURL->text(),
                                        ui->lineEditPages->text(),
                                        ui->lineEditDstFolder->text(),
                                        miMaxPageNum,
                                        msDocumentID,
                                        ui->radioButtonPdf->isChecked(),
                                        ui->checkBoxWebProxy->isChecked(),
                                        ui->checkBoxAutoProxy->isChecked(),
                                        ui->checkBoxResume->isChecked(),
                                        mpviZoomlevel->at(ui->comboBoxZoom->currentIndex()),
                                        ui->checkBoxGetOcr->isChecked());
}

/*-----------------------------------------------------------------------------------------------*
 * update gui elements when download finished
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_fileDownloadFinished(bool bFlag)
{
    // Add download to history
    if(bFlag)
        addBookToHistory();

    // Signals for batch job handling
    if (mBatchJobActivated)
    {
        emit downloadFinished(bFlag);
    }

    // Create pdf if requestd
    if ( ui->checkBoxAutoPdf->isChecked() && !mbhttpRequestAborted && bFlag )
    {
        on_pushButtonCreatePdf_clicked();
    }
    else if (mBatchJobActivated)
    {
        // Signals for batch job handling
        emit pdfMergeFinished(true);
    }
    else
    {
        // All is done
        QApplication::alert(this);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Cancel download
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_pushButtonCancelDownload_clicked(void)
{
    // Check if user wants to stop batchjob
    if (mBatchJobActivated)
    {
        // Get sender object and name
        QObject *senderObj = sender();
        QString senderObjName = senderObj->objectName();

        // Send cancel command to batch job
        if (!senderObjName.isEmpty())
        {
            emit cancelBatchJob();

            qDebug() << "HDH: BATCHJOB CANCELED BY USER";

            return;
        }
    }

    mpFileDownloader->cancelParallelDownload();

    ui->labelDownloadState->setText(tr("Download canceled."));
    ui->progressBarDownload->setValue(0);
    mbhttpRequestAborted = true;
    mbTimerActive        = false;

    disconnect(mpFileDownloader, SIGNAL(updateAutoProxyState(QString)), this, SLOT(slot_updateWebProxyState(QString)));    

    // Signals for batch job handling
    if (mBatchJobActivated)
    {
        emit downloadFinished(false);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Select PDF conversion method: ( a lot of identical code...but better overview :-P )
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_pushButtonCreatePdf_clicked()
{
    if (ui->lineEditSrcFolder->text().isEmpty())
    {
        ui->labelCreatePdfState->setText(QString("Invalid source folder."));

        // Signals for batch job handling
        if (mBatchJobActivated)
        {
            emit pdfMergeFinished(false);
        }
        else
        {
            QApplication::alert(this);
        }

        return;
    }

    // Set path
    QDir filePath(ui->lineEditSrcFolder->text());
    if (!filePath.exists())
    {
        ui->labelCreatePdfState->setText(QString("Unable to find directory: ") + filePath.path());

        // Signals for batch job handling
        if (mBatchJobActivated)
        {
            emit pdfMergeFinished(false);
        }
        else
        {
            QApplication::alert(this);
        }
        return;
    }

    // Start animated gif
    ui->mpMovieMerge->start();

    // Change button
    ui->pushButtonCreatePdf->setText("Cancel merging");
    disconnect(ui->pushButtonCreatePdf,SIGNAL(clicked()),this, SLOT(on_pushButtonCreatePdf_clicked()));
    connect(ui->pushButtonCreatePdf, SIGNAL(clicked()), this, SLOT(slot_stopPdfMergingProcess()));

    if(mBatchJobActivated)
        ui->pushButtonCreatePdf->setDisabled(false);

    // Reset flag
    mbStopPdfMergingProcessFlag = false;

    if ( ui->radioButtonMergePdftk->isChecked() )
    {
        if (ui->lineEditPages->text().contains("Download whole book") && mbWholeBookAvailable && mtUserSettings.bDownloadWholeBook)
        {
            // Use book title as filename
            QString sOutputFilename = ui->labelBookTitleText->text() ;
            if (sOutputFilename.length() < 2)
                sOutputFilename = "book_full_view";
            // limit title length
            if (sOutputFilename.length() > 50)
                sOutputFilename.truncate(sOutputFilename.indexOf(QChar(' '),50));
            sOutputFilename.append(".pdf.part");

            QString sTmpFilename = ui->lineEditSrcFolder->text() + msDocumentID + QString(".pdf");
            sOutputFilename.prepend(ui->lineEditSrcFolder->text());

            // Rename output file
            QFile::rename(sTmpFilename, sOutputFilename);
            slot_pdfWorkerCompleted(true, sOutputFilename);
        }
        else
        {
            // Merge pdfs with 'pdftk'
            mergePdfWithPdftk();
        }
    }
    else
    {
        if ( ui->checkBoxConvertOcrOnly->isChecked() )
        {
            // Convert OCR text only to pdf book or single pages
            convertAndMergeOcrToPdfFile();
        }
        else
        {
            // Convert images to pdf book or single pages
            convertAndMergeImagesToPdfFile();
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * PDF: Convert images & create PDF book or single pdfs
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::convertAndMergeImagesToPdfFile()
{
    bool bManualSelection = false;
    bool bCreatePdfBook = ui->radioButtonMergeImages->isChecked();

    QList<QByteArray> fileFormats  = QImageReader::supportedImageFormats ();

    // Set directory
    QDir imgFileDir(ui->lineEditSrcFolder->text() + QString("images/"));

    // Get image file list for image files
    QStringList imgFileList;
    bManualSelection = getFileList( &imgFileDir, &imgFileList,&fileFormats, true );
    fileFormats.clear();

    if ( bManualSelection )
    {
        if (imgFileList.isEmpty())
        {
            slot_pdfWorkerCompleted(true, "");

            return;
        }
    }

    // Create ocr filelist
    fileFormats.append("html");
    QDir ocrFileDir(ui->lineEditSrcFolder->text() + QString("ocr/"));
    QStringList ocrFileList;
    // Get ocr file list
    getFileList( &ocrFileDir, &ocrFileList,&fileFormats, false );

    if (imgFileList.isEmpty())
    {
        if (!bManualSelection ) {
            ui->labelCreatePdfState->setText(QString("Can't find any image files!"));
        }
        ui->progressBarCreatePdf->setValue(0);
        slot_pdfWorkerCompleted(true, "");
    }
    else
    {
        //  Add path to input file list
        if ( !bManualSelection )
        {
            for (int i = 0; i < imgFileList.count(); i++)
            {
                imgFileList.replace(i, imgFileDir.path() + "/" + imgFileList.at(i));
            }
            for (int i = 0; i < ocrFileList.count(); i++)
            {
                ocrFileList.replace(i, ocrFileDir.path() + "/" + ocrFileList.at(i));
            }
        }

        // Create output path
        QString sSubDirPdfOut(ui->lineEditSrcFolder->text());
        if (!bCreatePdfBook)
        {
            sSubDirPdfOut.append(QString("pdfs/"));
        }
        QDir pdfFileDir(sSubDirPdfOut);
        if (!pdfFileDir.mkpath(sSubDirPdfOut))
        {
            ui->labelCreatePdfState->setText(QString("Unable to create sub-directory: ") + sSubDirPdfOut );
            slot_pdfWorkerCompleted(true, "");
            return;
        }

        ui->progressBarCreatePdf->setMaximum(imgFileList.count());

        mpThread = new QThread;
        CreatePdfWorker* pdfWorker = new CreatePdfWorker( ui->labelBookTitleText->text(),
                                                          ui->lineEditSrcFolder->text(),
                                                          pdfFileDir.path(),
                                                          imgFileList,
                                                          ocrFileList,
                                                          mpPrinter,
                                                          &mbStopPdfMergingProcessFlag,
                                                          bCreatePdfBook,
                                                          bManualSelection);
        pdfWorker->moveToThread(mpThread);
        connect(mpThread,  SIGNAL(started()), pdfWorker, SLOT(slot_convertAndMergeImagesToPdfFile()));
        connect(pdfWorker, SIGNAL(updateProgressBar(int)), this, SLOT(slot_updatePdfMergeProgressBar(int)));
        connect(pdfWorker, SIGNAL(pdfMergeState(QString)), this, SLOT(slot_updateMergeState(QString)));
        connect(pdfWorker, SIGNAL(finished(bool, QString)), this, SLOT(slot_pdfWorkerCompleted(bool, QString)));
        connect(pdfWorker, SIGNAL(createMessageBox(QString)), this, SLOT(slot_createMessageBox(QString)));
        connect(this,      SIGNAL(abortThread(bool)), pdfWorker, SLOT(slot_abortThread(bool)));
        connect(pdfWorker, SIGNAL(finished(bool, QString)), pdfWorker, SLOT(deleteLater()));
        connect(pdfWorker, SIGNAL(destroyed()), mpThread, SLOT(quit()));
        connect(mpThread,  SIGNAL(finished()), mpThread, SLOT(deleteLater()));

        enablePdfMergeSettings(false);

        mpThread->start();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Convert OCR plaintext only to pdf book or single pages
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::convertAndMergeOcrToPdfFile()
{
    bool bManualSelection = false;
    bool bCreatePdfBook = ui->radioButtonMergeImages->isChecked();

    QList<QByteArray> fileFormats;

    // Get ocr file list for supported ocr formats
    QDir ocrFileDir(ui->lineEditSrcFolder->text() + QString("ocr/"));
    fileFormats.append("html");
    QStringList ocrFileList;
    bManualSelection = getFileList( &ocrFileDir, &ocrFileList,&fileFormats, true );

    if ( bManualSelection )
    {
        if (ocrFileList.isEmpty())
        {
            slot_pdfWorkerCompleted(true, "");

            return;
        }
    }

    if (ocrFileList.isEmpty())
    {
        if(!bManualSelection )
        {
            ui->labelCreatePdfState->setText(QString("Missing folders: can't find any 'ocr' files / 'ocr' subfolder!"));
        }
        ui->progressBarCreatePdf->setValue(0);
        slot_pdfWorkerCompleted(true, "");
    }
    else
    {
        //  Add path to file list
        if ( !bManualSelection )
        {
            for (int i = 0; i < ocrFileList.count(); i++)
            {
                ocrFileList.replace(i, ocrFileDir.path() + "/" + ocrFileList.at(i));
            }
        }

        // Create output path
        QString sSubDirPdfOut(ui->lineEditSrcFolder->text());
        if (!bCreatePdfBook && !bManualSelection)
        {
            sSubDirPdfOut.append(QString("pdfs_text_only/"));
        }
        QDir pdfFileDir(sSubDirPdfOut);
        if (!pdfFileDir.mkpath(sSubDirPdfOut))
        {
            ui->labelCreatePdfState->setText(QString("Unable to create sub-directory: ") + sSubDirPdfOut );
            slot_pdfWorkerCompleted(true, "");

            return;
        }

        ui->progressBarCreatePdf->setMaximum(ocrFileList.count());

        mpThread= new QThread;
        CreatePdfWorker* pdfWorker = new CreatePdfWorker( ui->labelBookTitleText->text(),
                                                          ui->lineEditSrcFolder->text(),
                                                          pdfFileDir.path(),
                                                          ocrFileList, // Dummy for code reuse
                                                          ocrFileList,
                                                          mpPrinter,
                                                          &mbStopPdfMergingProcessFlag,
                                                          bCreatePdfBook,
                                                          bManualSelection);
        pdfWorker->moveToThread(mpThread);
        connect(mpThread,  SIGNAL(started()), pdfWorker, SLOT(slot_convertAndMergeOcrToPdfFile()));
        connect(pdfWorker, SIGNAL(updateProgressBar(int)), this, SLOT(slot_updatePdfMergeProgressBar(int)));
        connect(pdfWorker, SIGNAL(pdfMergeState(QString)), this, SLOT(slot_updateMergeState(QString)));
        connect(pdfWorker, SIGNAL(finished(bool, QString)), this, SLOT(slot_pdfWorkerCompleted(bool, QString)));
        connect(pdfWorker, SIGNAL(createMessageBox(QString)), this, SLOT(slot_createMessageBox(QString)));
        connect(this,      SIGNAL(abortThread(bool)), pdfWorker, SLOT(slot_abortThread(bool)));
        connect(pdfWorker, SIGNAL(destroyed()), mpThread, SLOT(quit()));
        connect(pdfWorker, SIGNAL(finished(bool, QString)), pdfWorker, SLOT(deleteLater()));
        connect(mpThread,  SIGNAL(finished()), mpThread, SLOT(deleteLater()));
        enablePdfMergeSettings(false);

        mpThread->start();
    }
}


/*-----------------------------------------------------------------------------------------------*
 * Update GUI after pdf Worker / Thread has finished
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_pdfWorkerCompleted(bool bCreatePdfBook, QString sFilename )
{
    if ( !mBatchJobActivated )
        QApplication::alert(this);

    if (!sFilename.isEmpty() )
    {        
        QFile sFile(sFilename);

        if ( bCreatePdfBook && !mbStopPdfMergingProcessFlag)
        {
            if (sFilename.contains(".part"))
                sFilename.truncate(sFilename.lastIndexOf(".part"));

            if (mtUserSettings.bKeepExistingFile)
            {
                // Keep existing file: new file name :  filename_i.pdf
                sFilename.truncate(sFilename.lastIndexOf(".pdf"));
                int i = 1;
                while (sFile.exists(sFilename + QString("_%1.pdf").arg(i)))
                    i++;
                sFilename.append(QString("_%1.pdf").arg(i));
            }

            if ( !mBatchJobActivated )
            {
#if defined(Q_OS_MAC)
                /* Workarround for issue: QTBUG-27664 */
                sFilename = QFileDialog::getSaveFileName( this,
                                                        "Save File",
                                                        sFilename,
                                                        "(*.pdf)", 0, QFileDialog::DontUseNativeDialog ).toUtf8();
#else
                // Ask user for filename and destination folder
                sFilename = QFileDialog::getSaveFileName( this,
                                                          "Save File",
                                                          sFilename,
                                                          "(*.pdf)" ).toUtf8();
#endif
            }

            if (sFilename.isEmpty())
            {
                sFile.remove();
            }
            else
            {
                // Check if file already exist
                if (sFile.exists(sFilename))
                {
                    sFile.remove(sFilename);
                }
                sFile.rename(sFilename);
                emit updateHistoryTableContentPath(sFilename);

                if (mtUserSettings.bDeleteDownloadedPageData)
                {
                    // Delete downloaded pages
                    removeDownloadedFileData();
                }
            }
        }
        else if ( bCreatePdfBook && mbStopPdfMergingProcessFlag )
        {
            sFile.remove();
        }

        // Show generated file
        if (!sFilename.isEmpty() && !mBatchJobActivated  && !mbStopPdfMergingProcessFlag )
        {
                showFileInFilemanager(sFilename);
        }

        if ( !mbStopPdfMergingProcessFlag)
        {
            ui->labelCreatePdfState->setText(QString("Done."));
        }
        else
        {
           ui->labelCreatePdfState->setText(QString("Aborted by user."));
        }
        // Signals for batch job handling
        if (mBatchJobActivated)
        {
            emit pdfMergeFinished(true);
        }
    }
    else
    {
        // Signals for batch job handling
        if (mBatchJobActivated)
        {
            emit pdfMergeFinished(false);
        }
    }

    miCurPdfRetry = 0;

    mbStopPdfMergingProcessFlag = true;
    mpThread = 0;
    enablePdfMergeSettings(true);

    // Change button
    ui->pushButtonCreatePdf->setText("Create pdf");
    disconnect(ui->pushButtonCreatePdf,SIGNAL(clicked()), this, SLOT(slot_stopPdfMergingProcess()));
    connect(ui->pushButtonCreatePdf, SIGNAL(clicked()), this, SLOT(on_pushButtonCreatePdf_clicked()));

    if (mBatchJobActivated)
        ui->pushButtonCreatePdf->setDisabled(true);

    ui->mpMovieMerge->stop();
    ui->mpMovieMerge->jumpToFrame(0);
}

/*-----------------------------------------------------------------------------------------------*
 * PDF: delete downloaded file data
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::removeDownloadedFileData()
{
    bool bCreatePdfBookFromImages = ui->radioButtonMergeImages->isChecked();
    bool bCreatePdfBookFromPdfFile = ui->radioButtonMergePdftk->isChecked();

    // Create format string
    QString sDocumentID = msDocumentID;
    removeReservedCharFromString(&sDocumentID);

    // Set directory
    QDir dataFileDir(ui->lineEditSrcFolder->text());
    QFileInfoList dataFileList;
    QList<QByteArray> fileFormats;

    ui->progressBarCreatePdf->setMaximum(0);
    ui->progressBarCreatePdf->setValue(1);
    ui->labelCreatePdfState->setText(QString("Removing temporary files..."));
    if ((!sDocumentID.isEmpty()) && (bCreatePdfBookFromImages || bCreatePdfBookFromPdfFile))
    {
        for (int j = 0; j < 2; j++)
        {
            fileFormats.clear();
            if (j > 0)
            {
                fileFormats.append("html");
                dataFileDir.setPath(ui->lineEditSrcFolder->text() + QString("ocr/"));
            }
            else if (bCreatePdfBookFromImages)
            {
                fileFormats.append(QImageReader::supportedImageFormats());
                dataFileDir.setPath(ui->lineEditSrcFolder->text() + QString("images/"));
            }
            else
            {
                fileFormats.append("pdf");
                dataFileDir.setPath(ui->lineEditSrcFolder->text() + QString("pdfs/"));
            }

            QStringList sFormatList;
            for (int i=0; i < fileFormats.count(); i++ )
            {
                sFormatList.append( "*" + sDocumentID + "*page*." + fileFormats.at(i)); 
            }

            // Check for subfolder existance
            if(!dataFileDir.exists())
            {
                // Move up
                dataFileDir.cdUp();
            }

            // Create file list
            dataFileList = dataFileDir.entryInfoList( sFormatList,
                                                  QDir::Files | QDir::NoSymLinks, 0);
            // Remove files
            if (!dataFileList.isEmpty())
            {
                foreach (QFileInfo fileInfo, dataFileList)
                {
                    if (fileInfo.isFile() && fileInfo.absoluteFilePath().contains(ui->lineEditSrcFolder->text()))
                        QFile::remove(fileInfo.absoluteFilePath());
                        qDebug() << "HDH: DELETING " << fileInfo.absoluteFilePath();
                }
            }
            // Delete folder when empty
            dataFileList = dataFileDir.entryInfoList(QDir::Files | QDir::NoSymLinks, 0);

            if (dataFileList.isEmpty())
            {
                //dataFileDir.remove();
                QDir().rmdir(dataFileDir.absolutePath());
            }
        }
    }
    ui->progressBarCreatePdf->setMaximum(1);
}

/*-----------------------------------------------------------------------------------------------*
 * PDF: Get file list for pdf creation
 *-----------------------------------------------------------------------------------------------*/
bool HathiDownloadHelper::getFileList(QDir *pFileDir, QStringList *pFileList, QList<QByteArray> *pFileFormats, bool bMandatory)
{
    bool bManualSelection = false;

    // Create format string
    QString sDocumentID = msDocumentID;
    removeReservedCharFromString(&sDocumentID);
    QStringList sFormatList;
    for (int i=0; i < pFileFormats->count(); i++ )
        sFormatList.append( "*" + sDocumentID + "*page*." + pFileFormats->at(i));

    // Check for subfolder existance
    if(!pFileDir->exists())
    {
        // Move up
        pFileDir->cdUp();
    }

    // Create file list
    *pFileList = pFileDir->entryList( sFormatList,
                                      QDir::Files | QDir::NoSymLinks, 0);

    // If file list is empty retry with empty document ID
    if (pFileList->isEmpty())
    {
        for (int i=0; i < pFileFormats->count(); i++ )
            sFormatList.append( "*page*." + pFileFormats->at(i));
    }

    // Create file list
    *pFileList = pFileDir->entryList( sFormatList,
                                      QDir::Files | QDir::NoSymLinks, 0);

    // If file list is empty
    if (pFileList->isEmpty() && bMandatory)
    {
        QString sMessage = "Can't find any files created by HathiDownloadHelper in current directory.\n";
        sMessage.append("Do you want to select the files manually?\n\n");

        // Confirm convert action for image files
        if( QMessageBox::Yes == QMessageBox::question( this, "Unknown files!",
                                      sMessage,
                                      QMessageBox::Yes , QMessageBox::No ) )
        {
            bManualSelection = true;
            // Update extension list
            sFormatList.clear();
            for (int i=0; i < pFileFormats->count(); i++ )
                sFormatList.append("*." + pFileFormats->at(i));

            // Open file selcetion dialog
            FileSelectDialog selectDialog(this, ui->lineEditSrcFolder->text());
            selectDialog.setFileTypes(sFormatList);

            if(selectDialog.exec())
            {
                *pFileList = selectDialog.getFileList();
            }
        }
    }
    else if (pFileList->isEmpty() && sFormatList.contains("*" + sDocumentID + "*page*.html"))
    {
        // check for 'ocr' folder in upper folder layer
        pFileDir->cdUp();
        pFileDir->cd("ocr");
        // Check for 'ocr' files (*page*.html) in current/or subfolder
        *pFileList = pFileDir->entryList( sFormatList,
                                              QDir::Files | QDir::NoSymLinks, 0);
    }

    return bManualSelection;
}

/*-----------------------------------------------------------------------------------------------*
 * PDF: Manually select pdf files to be merged
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_selectPdfFilesToBeMergerd(void)
{
    QStringList pdfFileList;
    // Open file selcetion dialog
    FileSelectDialog selectDialog(this, ui->lineEditSrcFolder->text());
    selectDialog.setFileTypes(QStringList("*.pdf"));

    if(selectDialog.exec())
    {
        pdfFileList = selectDialog.getFileList();
    }
    // Merging will be done in thread
    if (!pdfFileList.isEmpty())
    {
        ui->progressBarCreatePdf->setMaximum(pdfFileList.count());
        ui->labelCreatePdfState->setText(QString("Merging pdfs using 'pdftk'' application..."));
        ui->progressBarCreatePdf->setValue(pdfFileList.count()/2);

        // Add filename
        QString sFilename = "book_full_view";

        // Set error threshold: pdftk will merge 50 pages per step + 10 for more tolerance
        miPdfRetryThreshold = 0;

        // Start animated gif
        ui->mpMovieMerge->start();

        // Change button
        ui->pushButtonCreatePdf->setText("Cancel merging");
        disconnect(ui->pushButtonCreatePdf,SIGNAL(clicked()),this, SLOT(on_pushButtonCreatePdf_clicked()));
        connect(ui->pushButtonCreatePdf, SIGNAL(clicked()), this, SLOT(slot_stopPdfMergingProcess()));

        // Reset flag
        mbStopPdfMergingProcessFlag = false;

        // Create thread for merging process
        mpThread= new QThread;
        CreatePdftkWorker* pdftkWorker = new CreatePdftkWorker(pdfFileList, sFilename, &mbStopPdfMergingProcessFlag);
        pdftkWorker->moveToThread(mpThread);
        connect(mpThread, SIGNAL(started()), pdftkWorker, SLOT(slot_mergePdfWithPdftk()));
        connect(pdftkWorker, SIGNAL(updateProgressBar(int)), this, SLOT(slot_updatePdfMergeProgressBar(int)));
        connect(pdftkWorker, SIGNAL(finished(bool, QString)), this, SLOT(slot_pdfWorkerCompleted(bool, QString)));
        connect(pdftkWorker, SIGNAL(pdfMergeState(QString)), this, SLOT(slot_updateMergeState(QString)));
        connect(pdftkWorker, SIGNAL(destroyed()), mpThread, SLOT(quit()));
        connect(pdftkWorker, SIGNAL(finished(bool, QString)), pdftkWorker, SLOT(deleteLater()));
        connect(mpThread, SIGNAL(finished()), mpThread, SLOT(deleteLater()));
        connect(pdftkWorker, SIGNAL(pdftkErrorMessage(QString)), this, SLOT(slot_showPdftkErrorMessage(QString)));

        enablePdfMergeSettings(false);

        mpThread->start();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * PDF: Stop 'create pdf' process
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_stopPdfMergingProcess()
{
    // Check if user wants to stop batchjob
    if (mBatchJobActivated)
    {
        // Get sender object and name
        QObject *senderObj = sender();
        QString senderObjName = senderObj->objectName();

        // Send cancel command to batch job
        if (!senderObjName.isEmpty())
        {
            emit cancelBatchJob();

            qDebug() << "HDH: BATCHJOB CANCELED BY USER";

            return;
        }
    }

    mbStopPdfMergingProcessFlag = true;
}

/*-----------------------------------------------------------------------------------------------*
 * PDF: Merge pdf files using pdftk (www.pdflabs.com)
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::mergePdfWithPdftk()
{
    // Use book title as filename
    QString sFilename = ui->labelBookTitleText->text() ;
    if (sFilename.length() < 2)
        sFilename = "book_full_view";
    // limit title length
    if (sFilename.length() > 50)
        sFilename.truncate(sFilename.indexOf(QChar(' '),50));
    sFilename.append(".pdf.part");

    // Create pdf filelist from files in 'pdfs'-subfolder
    QDir pdfFileDir(ui->lineEditSrcFolder->text() + QString("pdfs/"));
    QStringList pdfFileList;
    bool bManualSelection = false;

    QString sDocumentID = msDocumentID;
    removeReservedCharFromString(&sDocumentID);

    // Check if 'pdfs' subfolder exists
    if (pdfFileDir.exists())
    {
        // Find pdfs with *page* filename pattern
        pdfFileList = pdfFileDir.entryList( QStringList("*" + sDocumentID + "*page*"),
                                            QDir::Files | QDir::NoSymLinks, 0);

    }
    else
    {
        // Check if current folder contains *page*.pdf files to be merged
        pdfFileDir.setPath(ui->lineEditSrcFolder->text());
        pdfFileList = pdfFileDir.entryList( QStringList("*" + sDocumentID + "*page*.pdf"),
                                             QDir::Files | QDir::NoSymLinks, 0);
    }

    //  If filelist is empty, check for files without book id
    if (pdfFileList.isEmpty())
    {
        sFilename = "book_full_view";

        // Check if current folder contains *page*.pdf files to be merged
        pdfFileDir.setPath(ui->lineEditSrcFolder->text() + QString("pdfs/"));
        pdfFileList = pdfFileDir.entryList( QStringList("*page*.pdf"),
                                             QDir::Files | QDir::NoSymLinks, 0);
    }
    //  If filelist is empty, check for files in upper folder
    if (pdfFileList.isEmpty())
    {
        sFilename = "book_full_view";

        // Check if current folder contains *page*.pdf files to be merged
        pdfFileDir.setPath(ui->lineEditSrcFolder->text());
        pdfFileList = pdfFileDir.entryList( QStringList("*page*.pdf"),
                                             QDir::Files | QDir::NoSymLinks, 0);
    }

    //  If filelist is empty, ask User for manual file selection
    if (pdfFileList.isEmpty())
    {
        QString sMessage = "Can't find any pdf-files created by HathiDownloadHelper in current directory.\n";
        sMessage.append("Do you want to select the files manually?\n\n");

        // Confirm merge action for pdf files
        if( QMessageBox::Yes == QMessageBox::question(   this, "Unknown files!",
                                                        sMessage,
                                                        QMessageBox::Yes , QMessageBox::No ) )
        {
            bManualSelection = true;

            FileSelectDialog selectDialog;
            selectDialog.setFileTypes(QStringList("*.pdf"));
            if(selectDialog.exec())
                pdfFileList = selectDialog.getFileList();

            if (pdfFileList.isEmpty())
            {
                slot_pdfWorkerCompleted(true, "");
                return;
            }
        }
        else
        {
            ui->labelCreatePdfState->setText(QString(""));
            slot_pdfWorkerCompleted(true, "");
            return;
        }
    }

    // Check if pdf files are corrupted
#define MINIMUM_FILESIZE 50
    QStringList sCorruptedFileList;
    bool bError = true;
    foreach (QString sFile, pdfFileList)
    {            
            bError = true;
            QFile pdfFile(bManualSelection ? sFile : pdfFileDir.path() + "/" + sFile);

            if(pdfFile.open(QIODevice::ReadOnly))
            {
                QTextStream stream(&pdfFile);
                QString sPdfContent = stream.readAll();
                if (sPdfContent.contains("%%EOF"))
                    bError = false;
            }


            if (bError &&((uint) pdfFile.size() < MINIMUM_FILESIZE))
            {
                qDebug() << "HDH : CORRUPTED PDF DETECTED! [EOF: " << !bError << "], MINIMUM_FILESIZE: " << ((uint) pdfFile.size() < MINIMUM_FILESIZE) << ", " + sFile + "]";
                sCorruptedFileList.append(sFile);
            }
    }

    if (sCorruptedFileList.count() > 0)
    {
        // User action required
        QString sCorruptedFileMessage = "Corrupted file(s) detected:\n";
        foreach (QString sFile, sCorruptedFileList)
            sCorruptedFileMessage.append(sFile + "\n");
        sCorruptedFileMessage.append("\nDo you want to continue and skip corrputed files?\n\n");

        if( QMessageBox::Yes == QMessageBox::question(   this, "Corrupted files!",
                                                        sCorruptedFileMessage,
                                                        QMessageBox::Yes , QMessageBox::No ) )
        {
            foreach (QString sFile, sCorruptedFileList)
                pdfFileList.removeOne(sFile);
        }
        else
        {
            ui->labelCreatePdfState->setText(QString(""));
            slot_pdfWorkerCompleted(true, "");
            return;
        }
    }

    // Merging will be done in thread
    if (!pdfFileList.isEmpty() && pdfFileList.count() > 1)
    {
        ui->progressBarCreatePdf->setMaximum(pdfFileList.count());
        ui->labelCreatePdfState->setText(QString("Merging pdfs using 'pdftk'' application..."));
        ui->progressBarCreatePdf->setValue(pdfFileList.count()/2);

        // Add path to filename
        sFilename.prepend(ui->lineEditSrcFolder->text());

        //  Add path to file list
        if ( !bManualSelection )
        {
            for (int i = 0; i < pdfFileList.count(); i++)
            {
                pdfFileList.replace(i, pdfFileDir.path() + "/" + pdfFileList.at(i));
            }
        }

        if ( pdfFileList.contains(sFilename) )
        {
            ui->labelCreatePdfState->setText(QString("Error: Source and target filenames have to be different!: %1").arg(sFilename));
            slot_pdfWorkerCompleted(true, "");
            return;
        }

        // Set error threshold: pdftk will merge 50 pages per step + 10 for more tolerance
        miPdfRetryThreshold = (int)(pdfFileList.count()/50) + 10;

        // Create thread for merging process
        mpThread= new QThread;
        CreatePdftkWorker* pdftkWorker = new CreatePdftkWorker(pdfFileList, sFilename, &mbStopPdfMergingProcessFlag);
        pdftkWorker->moveToThread(mpThread);
        connect(mpThread, SIGNAL(started()), pdftkWorker, SLOT(slot_mergePdfWithPdftk()));
        connect(pdftkWorker, SIGNAL(updateProgressBar(int)), this, SLOT(slot_updatePdfMergeProgressBar(int)));
        connect(pdftkWorker, SIGNAL(finished(bool, QString)), this, SLOT(slot_pdfWorkerCompleted(bool, QString)));
        connect(pdftkWorker, SIGNAL(pdfMergeState(QString)), this, SLOT(slot_updateMergeState(QString)));
        connect(pdftkWorker, SIGNAL(destroyed()), mpThread, SLOT(quit()));
        connect(pdftkWorker, SIGNAL(finished(bool, QString)), pdftkWorker, SLOT(deleteLater()));
        connect(mpThread, SIGNAL(finished()), mpThread, SLOT(deleteLater()));
        connect(pdftkWorker, SIGNAL(pdftkErrorMessage(QString)), this, SLOT(slot_showPdftkErrorMessage(QString)));

        enablePdfMergeSettings(false);

        mpThread->start();
    }
    else
    {
        ui->labelCreatePdfState->setText(QString(""));
        slot_pdfWorkerCompleted(true, "");
    }
}

/*-----------------------------------------------------------------------------------------------*
 * PDF: Delete corrupted files,set resume download option and restart download
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_showPdftkErrorMessage(QString sErrorMessage)
{
    qDebug() << "HDH: CORRUPTED PDF FILE DETECTED BY PDFTK";

    QString sDocumentID = msDocumentID;
    removeReservedCharFromString(&sDocumentID);

    miCurPdfRetry++;
    if ((miCurPdfRetry < miPdfRetryThreshold) && sErrorMessage.contains(sDocumentID))
    {
        // Determine and delete all corrputed files
        QStringList pdfCorruptedFileList = sErrorMessage.split("\n", QString::SkipEmptyParts);
        foreach (QString sFile, pdfCorruptedFileList)
        {
            sFile = QDir::fromNativeSeparators(sFile);
            if (sFile.contains(ui->lineEditSrcFolder->text()))
            {

                sFile = sFile.mid(sFile.indexOf(ui->lineEditSrcFolder->text()));
                sFile.remove(QChar(13)); // remove CR: Feedback from pdftk contains CR;

                QFile::remove(sFile);
            }
        }
        // Activate resume on download option
        if (!ui->checkBoxResume->isChecked())
            ui->checkBoxResume->setChecked(true);

        mbStopPdfMergingProcessFlag = true;
        mpThread = 0;
        enablePdfMergeSettings(true);

        // Change button
        ui->pushButtonCreatePdf->setText("Create pdf");
        disconnect(ui->pushButtonCreatePdf,SIGNAL(clicked()), this, SLOT(slot_stopPdfMergingProcess()));
        connect(ui->pushButtonCreatePdf, SIGNAL(clicked()), this, SLOT(on_pushButtonCreatePdf_clicked()));

        ui->mpMovieMerge->stop();
        ui->mpMovieMerge->jumpToFrame(0);

        // Signals for batch job handling
        if ((!msDocumentID.isEmpty()) && ui->lineEditBookURL->text().contains(msDocumentID) && (sErrorMessage.contains(sDocumentID)))
        {
            on_pushButtonStartDownload_clicked();
        }
        else
        {
            QString sShortErrorMsg;
            if (sErrorMessage.contains(".pdf") && sErrorMessage.indexOf(".pdf") < 2)
                sShortErrorMsg = sErrorMessage.mid(0,sErrorMessage.indexOf(".pdf")+4);
            else if (sErrorMessage.contains("\r\n"))
                sShortErrorMsg = sErrorMessage.mid(0,sErrorMessage.indexOf("\r\n"));
            else
                sShortErrorMsg = "A PDFTK error has occurred.";

            QMessageBox errorMessageBox(this);
            errorMessageBox.setIcon(QMessageBox::Warning);
            errorMessageBox.setWindowTitle("PDFTK: An error has occured!");
            errorMessageBox.setText(sShortErrorMsg);
            errorMessageBox.addButton(QMessageBox::Ok);
            errorMessageBox.setDetailedText(sErrorMessage);
            errorMessageBox.exec();
        }
    }
    else
    {
        QString sShortErrorMsg;
        if (sErrorMessage.contains(".pdf") && sErrorMessage.indexOf(".pdf") < 2)
            sShortErrorMsg = sErrorMessage.mid(0,sErrorMessage.indexOf(".pdf")+4);
        else if (sErrorMessage.contains("\r\n"))
            sShortErrorMsg = sErrorMessage.mid(0,sErrorMessage.indexOf("\r\n"));
        else
            sShortErrorMsg = "A PDFTK error has occurred.";

        QMessageBox errorMessageBox(this);
        errorMessageBox.setIcon(QMessageBox::Warning);
        errorMessageBox.setWindowTitle("PDFTK: An error has occured!");
        errorMessageBox.setText(sShortErrorMsg);
        errorMessageBox.addButton(QMessageBox::Ok);
        errorMessageBox.setDetailedText(sErrorMessage);
        errorMessageBox.exec();

        slot_pdfWorkerCompleted(false, "");
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Close app
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_pushButtonExit_clicked()
{    
    // Stop update check request
    if(mpUpdateReply)
    {
        mpRetryTimer->start(15000);
        slot_pushButtonCancelUpdate_clicked();
    }

    // Stop book query
    if (ui->pushButtonGetInfo->text().contains("Cancel"))
      ui->pushButtonGetInfo->click();

    // Stop download
    if (ui->pushButtonStartDownload->text().contains("Cancel"))
        ui->pushButtonStartDownload->click();

    // Stop all merging processes
    // Check if merging thread is active
    if ( mpThread != 0)
    {
        QString sMessage = QString("Stopping thread. Please wait...");
        ui->labelCreatePdfState->setText(sMessage);
        slot_stopPdfMergingProcess();

        // Wait for thread to be finished and reset of flag
        int i = 0;
        while( mpThread != 0 && (i < 10) )
        {
            sMessage.append(".");
            ui->labelCreatePdfState->setText(sMessage);

            QEventLoop loop;
            QTimer::singleShot(250, &loop, SLOT(quit()));
            loop.exec();
            i++;
        }
    }

    // Close database
    mHistoryDatabase.commit();
    mHistoryDatabase.close();

    close();
}
