#include "webproxy.h"
#include "common.h"

#include <QNetworkRequest>
#include <QWebView>
#include <QTableWidget>
#include <QSslError>

/*-----------------------------------------------------------------------------------------------*
 * Class definition:    WebProxy                                                                 *
 * The WebProxy class is used to establish a session with a remote webproxy. After               *
 * establishing a session it provides a method to get proxified target URLs (hot linking).       *
 * To start a session                                                                            *
 *                                                                                               *
 *      void getNewWebProxy(QString sTargetUrl);                                                 *
 *                                                                                               *
 * has to be called. This method will load a random webproxy from the web, fill the input        *
 * formula and open a cookie session. On successfully opening the requested URL the cookie       *
 * session will be copied to the QNetworkAccessManager specified on initialization. The          *
 * proxified (and if necessary also encrypted) target URL can be obtained by calling             *
 *                                                                                               *
 *      QString getProxifiedUrl(QString sTargetUrl)                                              *
 *                                                                                               *
 * WebProxy was implemented as subfunction of Hathi Download Helpler.                            *
 *-----------------------------------------------------------------------------------------------*/

/* TODOS ----------------------------------------------------------------------------------------*/
//  + add support for crypted url addresses
//  + check and update php-script handling (url-encrytion) for hotlinking
//  + alternative for crypted url: add download module which is called by filedownloader and
//    downloads up to 6 files in parallel -> finish singal to filedownloader
//  + handover with autoproxy
//  + handling of chunked https headers without length attribute
//  + import webproxy URLS from webpages
//  + add webproxy dialog + webproxy check


/* Private definition ---------------------------------------------------------------------------*/
#define NUM_DOWNLOAD_REQUESTS 6
#define WAIT_TIME             30000

// Avoid redefinition of same connection between same elements
#define connect(src, sig, trgt, slt) connect(src, sig, trgt, slt, Qt::UniqueConnection)

/*-----------------------------------------------------------------------------------------------*
 * Constructor of WebProxy
 *-----------------------------------------------------------------------------------------------*/
WebProxy::WebProxy(QNetworkAccessManager* pDownloadManager):
    mpDownloadManager(pDownloadManager)
{
    // Initialize WebProxy
    initWebProxy();
}

/*-----------------------------------------------------------------------------------------------*
 * Destructor of WebProxy
 *-----------------------------------------------------------------------------------------------*/
WebProxy::~WebProxy(void)
{
    // Deinit
}

/*-----------------------------------------------------------------------------------------------*
 * Init WebProxy
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::initWebProxy()
{
    // Use independet network access manager for proxy requests
    mpProxyManager  = new HdhNetworkAccessManager();
    connect(mpProxyManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(slot_handleSslErrors(QNetworkReply*,QList<QSslError>)));
    mpRequest       = new QNetworkRequest();
    mpRequest->setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");
    mpReply         = NULL;

    mpWebProxyPage  = new WebProxyWebPage(this);

    /* Security settings for webpage */
    QWebSettings* mpWebProxyPageSettings = mpWebProxyPage->settings();
    mpWebProxyPageSettings->setAttribute( QWebSettings::JavascriptEnabled, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::JavascriptCanOpenWindows, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::JavaEnabled, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::AutoLoadImages, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::LocalContentCanAccessFileUrls, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::XSSAuditingEnabled, true);
    mpWebProxyPageSettings->setAttribute( QWebSettings::PluginsEnabled, false);

    mpWebProxyPage->setNetworkAccessManager(mpProxyManager);

    mpWebProxyPageTimer         = new QTimer();
    mpWebProxyPageTimer->setSingleShot(true);
    connect(mpWebProxyPageTimer, SIGNAL(timeout()), this, SLOT(slot_abortWebProxyLoadPage()));

    msWebproxyPrefix            = "";
    msTargetUrl                 = "";
    mbWebProxyRequestActivated  = false;
    mbWebProxyIsEnabled         = false;    // flag used to stop pending proxy-requests
    mbWebProxyCanceled          = false;
    miNumDownloads              = 0;
    msPrevWebProxyPrefix        = "";
    muiWebproxyTimestamp        = 0;
    miNumParallelDownloads      = NUM_DOWNLOAD_REQUESTS;

    mpWebProxyQueuingList       = new tWebProxyQueuingList();
    // add local host
    mpWebProxyQueuingList->sHostList.append("localhost");
    mpWebProxyQueuingList->uiHoldTimeList.append(0);
    mpWebProxyQueuingList->iQuality.append(miNumDownloads);

    // Load webproxy list from file
    QString sProxyCollection = readFile("prxylst.xml");
    if (sProxyCollection.isEmpty())
    {
        // Generate default proxy list
        generateDefaultProxyList();
        // Generate default proxy file
        generateProxyFile();
    }

    // Generate
    readProxyCollectionFile(&sProxyCollection);
}

/*-----------------------------------------------------------------------------------------------*
 * Full Reset of WebProxy
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::resetWebProxy()
{
    qDebug() << "WEBPROXY: RUN FULL RESET";

    // Use independet network access manager for proxy requests
    if(!mpProxyManager)
        mpProxyManager->deleteLater();
    mpProxyManager  = new HdhNetworkAccessManager();  // REQUIRED IN CASE OF HANDOVER FROM AUTOPROXY: CRASHES OTHERWISE ON GET() COMMAND
    connect(mpProxyManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(slot_handleSslErrors(QNetworkReply*,QList<QSslError>)));

    /* Security settings for webpage */
    QWebSettings* mpWebProxyPageSettings = mpWebProxyPage->settings();
    mpWebProxyPageSettings->setAttribute( QWebSettings::JavascriptEnabled, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::JavascriptCanOpenWindows, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::JavaEnabled, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::AutoLoadImages, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::LocalContentCanAccessFileUrls, false);
    mpWebProxyPageSettings->setAttribute( QWebSettings::XSSAuditingEnabled, true);
    mpWebProxyPageSettings->setAttribute( QWebSettings::PluginsEnabled, false);

    mpWebProxyPage->setNetworkAccessManager(mpProxyManager);
}

/*-----------------------------------------------------------------------------------------------*
 * Read web proxy collection file prxylst.xml
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::readProxyCollectionFile(QString* psProxyCollection)
{
    QString sXmlItem, sProxyUrl, sScore;
    int iIdxCollection = 0, iIdxItem = 0;

    // Random proxy sources
    while(!(sXmlItem = myXmlReader(psProxyCollection, "randomproxy", &iIdxCollection)).isEmpty())
    {
        iIdxItem  = 0;
        sProxyUrl = myXmlReader(&sXmlItem, "url", &iIdxItem);

        if (!sProxyUrl.isEmpty())
        {
            msRandomProxyList.append(sProxyUrl);
        }
    }

    // Webproxy sources
    iIdxCollection = 0;
    while(!(sXmlItem = myXmlReader(psProxyCollection, "webproxy", &iIdxCollection)).isEmpty())
    {
        iIdxItem  = 0;
        sProxyUrl = myXmlReader(&sXmlItem, "url", &iIdxItem);
        sScore    = myXmlReader(&sXmlItem, "score", &iIdxItem);

        if (!sProxyUrl.isEmpty() && !sScore.isEmpty())
        {
            msWebProxyList.append(sProxyUrl);
            miWebProxyScore.append(sScore.toInt());
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Generate defaultweb proxy collection
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::generateDefaultProxyList(void)
{
    // Store Settings in prylst.xml file
    msRandomProxyList.append("http://proxy.org/proxy.pl?url=homepage&proxy=random");
    msRandomProxyList.append("http://prx.centrump2p.com/english/random");

    // TODO: extracting webproxy from webproxy lists
    // TODO: add webproxy dialog + webproxy check
    // candidates list with web proxy collection
    // http://www.proxy4free.com/list/webproxy_rating1.html
    // http://free-proxy-list.net/web-proxy.html
    // http://proxylist.me/proxyweb

    msWebProxyList.append("http://www.4everproxy.com/");  // also includes list with webproxy
   // msWebProxyList.append("http://hidemyass.com/proxy/");  // missing content-length in head. maybe related to QTBUG-44824
    msWebProxyList.append("http://roogen.com/");
    msWebProxyList.append("http://fro4nix.org/");
    msWebProxyList.append("http://snapxs.com/");
    msWebProxyList.append("http://fblocker.info");
    msWebProxyList.append("https://whoer.net/webproxy");
    msWebProxyList.append("http://www.vpnbook.com/webproxy");

    // black list - not working anymore or blocked
    // msRandomProxyList.append("http://www.proxylisty.com/out/");
    // msWebProxyList.append("http://proxycab.com/");
    // msWebProxyList.append("http://proxymesh.com/web/index.php");
    // msWebProxyList.append("http://anonymouse.org/anonwww.html");
    // msWebProxyList.append("http://phpbido.org/");  // unknown encryption
    // msWebProxyList.append("http://needproxy.pw/");

    for(int i=0; i < msWebProxyList.count(); i++)
    {
        miWebProxyScore.append(i);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Generate web proxy collection file prxylst.xml
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::generateProxyFile(void)
{
    int row;
    QTableWidget* pProxyTable;
    QTableWidgetItem *pUrlItem, *pScoreItem ;
    QString* psProxyCollection = new QString();

    pProxyTable = new QTableWidget(0, 2);

    for (int i=0; i < msWebProxyList.length(); i++)
    {
        row = pProxyTable->rowCount();
        pProxyTable->insertRow(row);
        pUrlItem = new QTableWidgetItem();
        pUrlItem->setText(msWebProxyList.at(i));
        pProxyTable->setItem(row, 0, pUrlItem);
        pScoreItem  = new QTableWidgetItem();;
        pScoreItem->setText(QString("%1").arg((miWebProxyScore.at(i)),3, 10, QChar('0')));
        pProxyTable->setItem(row, 1, pScoreItem);
    }

    pProxyTable->sortItems(1,Qt::DescendingOrder);

    psProxyCollection->clear();
    psProxyCollection->append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    psProxyCollection->append("<proxycollection>\n");
    // Add random proxies
    for (int i=0; i < msRandomProxyList.length(); i++)
    {
        psProxyCollection->append("<randomproxy>");
        psProxyCollection->append("<url>");
        psProxyCollection->append(msRandomProxyList.at(i));
        psProxyCollection->append("</url>");
        psProxyCollection->append("</randomproxy>\n");
    }
    // Add web proxies
    for (int i=0; i < msWebProxyList.length(); i++)
    {
        psProxyCollection->append("<webproxy>");
        psProxyCollection->append("<url>");
        psProxyCollection->append(pProxyTable->item(i,0)->text());
        psProxyCollection->append("</url>");
        psProxyCollection->append("<score>");
        psProxyCollection->append(pProxyTable->item(i,1)->text());
        psProxyCollection->append("</score>");
        psProxyCollection->append("</webproxy>\n");
    }
    psProxyCollection->append("</proxycollection>\n");

    writeFile(QString("prxylst.xml"), psProxyCollection);
}

/*-----------------------------------------------------------------------------------------------*
 * Receive requested target page
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::updateWebProxyFile(void)
{
    int iScore;
    QString sProxy;

    for( int j = 1; j < mpWebProxyQueuingList->sHostList.count(); j++)
    {
        sProxy = mpWebProxyQueuingList->sHostList.at(j);
        iScore = mpWebProxyQueuingList->iQuality.at(j);
        int i  = msWebProxyList.indexOf(sProxy);

        if (i < 0)
        {
            // Add new WebProxy
            msWebProxyList.append(sProxy);
            miWebProxyScore.append(iScore);
        }
        else
        {
            // Update webproxy list
            iScore = 0.5 * ( iScore + miWebProxyScore.at(i) );
            miWebProxyScore.replace(i, iScore);
        }
    }

    /* Generate web proxy file */
    generateProxyFile();
}

/*-----------------------------------------------------------------------------------------------*
 * Get new webproxy - Load webproxy page with form
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::getNewWebProxy(QString sTargetUrl)
{
    qDebug() << "WEBPROXY: GET NEW WEBPROXY";

    QDateTime currentDate = QDateTime::currentDateTime();

    mpWebProxyPageTimer->stop();
    mpWebProxyPage->triggerAction(QWebPage::Stop);
    mbWebProxyRequestActivated = true;
    msTargetUrl                = sTargetUrl;
    mpWebProxyPage->mainFrame()->load(QUrl("about:blank"));

        qDebug() << "WEBPROXY: Target Url: " << msTargetUrl;

    if (mbWebProxyCanceled)
    {
        mbWebProxyRequestActivated = false;
        mbWebProxyCanceled         = false;
        // Do nothing
    }
    // Check localhost queuing time
    else if ( mpWebProxyQueuingList->uiHoldTimeList.at(0) < currentDate.toTime_t() )
    {
        // Use locahost
        msWebproxyPrefix = "";
        emit updateWebProxyState(QString(""));
        mbWebProxyRequestActivated = false;
        msCurProxyUrl.clear();
    }
    else if( (muiWebproxyTimestamp + 15)  < currentDate.toTime_t() )
    {
        QUrl proxySrcUrl;
        int iTrials = 10;
        do
        {
            iTrials--;
            // Random choice --------------------------*/
            switch(qrand()%3)
            {
            /* ----------------------------------------*/
            // Use random proxy sources (33% chance)
            case 0:
                proxySrcUrl.setUrl(msRandomProxyList.at(qrand()%msRandomProxyList.count()));

                break;
            /* ----------------------------------------*/
            // Use webproxy sources (66% chance)
            default:

                proxySrcUrl.setUrl(msWebProxyList.at(qrand()%msWebProxyList.count()));

                break;
            }
            // Store selected webproxy url
            msCurProxyUrl = proxySrcUrl.toString();
        }while(iTrials && msBlockedProxyList.contains(msCurProxyUrl));


        // TODO: remove after testing
//        msCurProxyUrl = "http://snapxs.com/";
//        proxySrcUrl = QUrl("http://snapxs.com/");


        emit updateWebProxyState(QString("WebProxy: Acquire new WebProxy..."));
        mpWebProxyPageTimer->start(WAIT_TIME);
        mpRequest->setUrl(proxySrcUrl);
        // FOR DEBUGGING
        qDebug() << "WEBPROXY: NEW PROXY: " << msCurProxyUrl;
        qDebug() << "valid: "  << proxySrcUrl.isValid();
        qDebug() << "empty: "  << proxySrcUrl.isEmpty();
        qDebug() << "string: " << proxySrcUrl.toString();

        if(mpReply != NULL)
            mpReply->deleteLater();
        connect(mpProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_followRedirectionToWebpageForm(QNetworkReply*)));
        mpReply =  mpProxyManager->get(*mpRequest);
        qDebug() << "WEBPROXY: SEND REQUEST";
    }
    else
    {
        // wait some time since webproxy has been updated just before
        mbWebProxyRequestActivated = false;
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Set number of parallel downloads - used to kick out bad scored proxies
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::setNumParallelDownloads(int iNumOfParallelDownloads)
{
    miNumParallelDownloads = iNumOfParallelDownloads;
}

/*-----------------------------------------------------------------------------------------------*
 * Cancel webproxy request
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::cancelRequest(void)
{
    qDebug() << "WEBPROXY: CANCELED";

    //if (mbWebProxyRequestActivated)
    mbWebProxyCanceled          = true;

    abortWebProxyLoadPage();
    setWebProxyPrefix("");

    mbWebProxyRequestActivated  =  false;

    emit updateWebProxyState(QString(""));
}

/*-----------------------------------------------------------------------------------------------*
 * reset webproxy request
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::reset(void)
{
    qDebug() << "WEBPROXY: RESET()";
    cancelRequest();
}

/*-----------------------------------------------------------------------------------------------*
 * Abort webproxy load page
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::abortWebProxyLoadPage(void)
{
    // call private method
    slot_abortWebProxyLoadPage();
}

/*-----------------------------------------------------------------------------------------------*
 * Get current WebProxy url
 *-----------------------------------------------------------------------------------------------*/
QString WebProxy::getWebProxyPrefix(void)
{
    return msWebproxyPrefix;
}

/*-----------------------------------------------------------------------------------------------*
 * Set current WebProxy url
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::setWebProxyPrefix(QString sUrl)
{
    qDebug() << "WEBPROXY: SET NEW WEBPROXY PREFIX " << sUrl;

    msWebproxyPrefix = sUrl;

    updateWebProxyList();
    updateWebProxyFile();
}

/*-----------------------------------------------------------------------------------------------*
 * Get current WebProxy enable state
 *-----------------------------------------------------------------------------------------------*/
bool WebProxy::getWebProxyEnableState(void)
{
    return mbWebProxyIsEnabled;
}

/*-----------------------------------------------------------------------------------------------*
 * Set current WebProxy enable state
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::setWebProxyEnableState(bool bEnable)
{
    qDebug() << "WEBPROXY: SET ENABLED " << bEnable;

    if (mbWebProxyIsEnabled && !bEnable)
        cancelRequest();
    else
        resetWebProxy();

    mbWebProxyIsEnabled = bEnable;
}

/*-----------------------------------------------------------------------------------------------*
 * Get used webproxy list
 *-----------------------------------------------------------------------------------------------*/
QStringList WebProxy::getLogOfUsedWebProxies(void)
{
    return mpWebProxyQueuingList->sHostList;
}

/*-----------------------------------------------------------------------------------------------*
 * Increment download counter
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::slot_updateDownloadCounter(QNetworkReply* pReply)
{
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    // On error or redirection
    if (pReply->error() || !redirectionTarget.isNull() || pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty())
    {
        // Do nothing
    }
    else
    {
        // Update download counter
        miNumDownloads++;
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Get proxified target URL
 *-----------------------------------------------------------------------------------------------*/
QString WebProxy::getProxifiedUrl(QString sTargetUrl)
{
    QDateTime currentDate = QDateTime::currentDateTime();

    // Check localhost queuing time
    if ( (mpWebProxyQueuingList->uiHoldTimeList.at(0) + 15) < currentDate.toTime_t() )
    {
        msWebproxyPrefix = "";
        emit updateWebProxyState(QString(""));
    }
    else
    {
        // Update download counter for current proxy
        if (msWebproxyPrefix.compare(msPrevWebProxyPrefix) != 0)
        {
            // Proxy has changed
            // Update proxy list
            updateWebProxyList();
            // Reset variables
            msPrevWebProxyPrefix = msWebproxyPrefix;
            miNumDownloads       = 0;
        }

        // Re-create request with old URL + webproxy
        if (msWebproxyPrefix.contains("hidemyass"))
        {
            // Base 64 encoding is used for target URL
            // encoded URL does not have 'http' prefix
            sTargetUrl = sTargetUrl.mid(QString("http").length());
            sTargetUrl = sTargetUrl.toUtf8().toBase64();
        }
        else if (msWebproxyPrefix.contains(QString("index.php?q=")))
        {
            // Base 64 encoding is used for target URL
            sTargetUrl = sTargetUrl.toUtf8().toBase64();
        }
        else if (msWebproxyPrefix.contains(QString("browse.php?u=")))
        {
            // Plaintext URL for target URL
        }
        else if (msWebproxyPrefix.contains(QString("secure.php?u=")))
        {
            // Plaintext URL for target URL
        }
        else if ((msWebproxyPrefix.contains(QString("b.php?u="))))
        {
            // Plaintext URL for target URL
        }
        else if ((msWebproxyPrefix.contains(QString("php.php?u="))))
        {
            // Plaintext URL for target URL
        }
        else
        {
            msWebproxyPrefix = "";
            emit updateWebProxyState(QString(""));
        }

        sTargetUrl.prepend(msWebproxyPrefix);
        qDebug() << "WEBPROXY: PROXIFIED URL: " << sTargetUrl;
    }

    return sTargetUrl;
}

/*-----------------------------------------------------------------------------------------------*
 * Abort webproxy load page
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::slot_abortWebProxyLoadPage(void)
{
    qDebug() << "WEBPROXY: ABORT PAGE LOAD";

    mpWebProxyPageTimer->stop();
    if( (mpReply != NULL) && (mpReply->isOpen()) )
    {

        qDebug() << "mpReply->url(): " <<  mpReply->url().toString();

        mpReply->abort();
        mpReply->deleteLater();
        mpReply = NULL;
    }


    disconnect(mpProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_followRedirectionToWebpageForm(QNetworkReply*)));
    disconnect(mpProxyManager, SIGNAL(finished(QNetworkReply*)) ,this, SLOT(slot_proxyTargetPageLoaded(QNetworkReply*)));

    disconnect(mpWebProxyPage->mainFrame(), SIGNAL(loadFinished(bool)) , this, SLOT(slot_proxyFormPageLoaded(bool)));

    mpWebProxyPage->triggerAction(QWebPage::Stop);
    emit updateWebProxyState(QString("WebProxy: Http webproxy request canceled...."));

    mbWebProxyRequestActivated = false;
}

/*-----------------------------------------------------------------------------------------------*
 * Update webproxy queing list + queuing times
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::updateWebProxyQueue(uint iUntilTime)
{
    QDateTime CurrentDate = QDateTime::currentDateTime();
    if ( iUntilTime > (CurrentDate.toTime_t() + 600) )
        iUntilTime = CurrentDate.toTime_t() + 600;

    if ( msWebproxyPrefix.isEmpty())
    {
        mpWebProxyQueuingList->uiHoldTimeList.replace(0, iUntilTime);
    }
    else
    {
        int i = mpWebProxyQueuingList->sHostList.indexOf(msCurProxyUrl); //msWebproxyPrefix);

        if (i < 0)
        {
            // Add new WebProxy
            mpWebProxyQueuingList->sHostList.append(msCurProxyUrl);//msWebproxyPrefix);
            mpWebProxyQueuingList->uiHoldTimeList.append(iUntilTime);
            mpWebProxyQueuingList->iQuality.append(0);
        }
        else
        {
            // Update hold time of current WebProxy
            mpWebProxyQueuingList->uiHoldTimeList.replace(i,iUntilTime);
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Update webproxy list + number of downloads
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::updateWebProxyList(void )
{
    int iAvgDownloads;

    if ( msWebproxyPrefix.isEmpty()) // msPrevWebProxyPrefix
    {
        iAvgDownloads = (mpWebProxyQueuingList->iQuality.at(0) + miNumDownloads) * 0.5;
        mpWebProxyQueuingList->iQuality.replace(0,  iAvgDownloads);
    }
    else
    {
        int i = mpWebProxyQueuingList->sHostList.indexOf(msCurProxyUrl);//msWebproxyPrefix);  //msPrevWebProxyPrefix

        if (i < 0)
        {
            // Add new WebProxy
            mpWebProxyQueuingList->sHostList.append(msCurProxyUrl);//msWebproxyPrefix);  //msPrevWebProxyPrefix
            mpWebProxyQueuingList->uiHoldTimeList.append(0);
            mpWebProxyQueuingList->iQuality.append(miNumDownloads);
        }
        else
        {
            // Update number of avg. downloads for current WebProxy
            if (mpWebProxyQueuingList->iQuality.at(i) > 0)
                iAvgDownloads = (mpWebProxyQueuingList->iQuality.at(i) + miNumDownloads) * 0.5;
            else
                iAvgDownloads = miNumDownloads;

            // Performance check
            if (miNumDownloads < miNumParallelDownloads)
                iAvgDownloads -= miNumParallelDownloads;

            // Remove bad proxies
            if (iAvgDownloads <= -100)
            {
                qDebug() << "WEBPROXY: REMOVING WEPROXY DUE TO WEAK PERFORMANCE - " << mpWebProxyQueuingList->sHostList.at(i).toLatin1();
                mpWebProxyQueuingList->sHostList.removeAt(i);
                mpWebProxyQueuingList->uiHoldTimeList.remove(i);
                mpWebProxyQueuingList->iQuality.remove(i);
            }
            else
            {
                mpWebProxyQueuingList->iQuality.replace(i,iAvgDownloads);
            }
        }

        // DEBUG OUTPUT - list all used webproxies
        qDebug() << "===========================================================================";
        qDebug() << "   number   _|_   webproxy URL   _|_   queuing time   _|_  downloads";
        qDebug() << "===========================================================================";
        for (int j = 0; j < mpWebProxyQueuingList->sHostList.length(); j++ )
        {
            qDebug() << QString::number(j)
                     <<  " __|__ " << mpWebProxyQueuingList->sHostList.at(j).toLatin1()
                     <<  " __|__ "  <<  QString::number( mpWebProxyQueuingList->uiHoldTimeList.at(j))
                     <<  " __|__ "  <<  QString::number( mpWebProxyQueuingList->iQuality.at(j));
            qDebug() << "---------------------------------------------------------------------------------------------------";
        }
        qDebug() << "===========================================================================";
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Follow redirections for request webpage
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::slot_followRedirectionToWebpageForm(QNetworkReply* pReply)
{
    mpWebProxyPageTimer->stop();
    disconnect(mpProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_followRedirectionToWebpageForm(QNetworkReply*)));

    if (mbWebProxyCanceled)
    {
        mbWebProxyCanceled         = false;

        return;
    }

    bool bRedirectionStateOk   = true;
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    // Check for redirection list
    if (msUrlRedirects.isEmpty())
    {
        // Add current url to url list
        msUrlRedirects.append(pReply->url().toString());
    }

    // Check if redirection target was already used
    if (msUrlRedirects.contains(redirectionTarget.toUrl().toString()))
    {
         qDebug() << "WEBPROXY: INFINITE REDIRECTION DETECTED. PROXY DROPPED";

        // Inifitive loop - stop requests
        bRedirectionStateOk = false;
    }
    else
    {
        // valid redirection
        msUrlRedirects.append(redirectionTarget.toUrl().toString());
        qDebug() << "WEBPROXY: ADDING REDIRECTION: " << redirectionTarget.toUrl().toString();
    }

    if (!pReply->error() && !redirectionTarget.toUrl().toString().isEmpty() && bRedirectionStateOk )
    {
        qDebug() << "WEBPROXY: FOLLOWING REDIRECTION: " << redirectionTarget.toUrl().toString();

        // loading redirection";
        mpWebProxyPageTimer->start(WAIT_TIME);
        connect(mpProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_followRedirectionToWebpageForm(QNetworkReply*)));
        mpRequest->setUrl(redirectionTarget.toUrl());
        if(mpReply != NULL)
            mpReply->deleteLater();
        mpReply = mpProxyManager->get(*mpRequest);
    }
    else if (pReply->error() || pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty())
    {
        qDebug() << "WEBPROXY: REQUEST FAILED - ERROR " << pReply->error();

        // Request failed - start new request
        msUrlRedirects.clear();
        emit updateWebProxyState(QString("WebProxy: Http webproxy request failed...."));
        getNewWebProxy(msTargetUrl);
    }
    else
    {
        msUrlRedirects.clear();

        /* WORKARROUND: Remove iFrames from webpage, which might causes webpage to crash */
        QString sHtml=  (QString)pReply->readAll();
        sHtml.replace("iFrame","a", Qt::CaseInsensitive);

        // Disable google scripts
      //  sHtml.replace("google","elgoog");

        // Set filtered html page
        connect(mpWebProxyPage->mainFrame(), SIGNAL(loadFinished(bool)) , this,  SLOT(slot_proxyFormPageLoaded(bool)));
        mpWebProxyPage->mainFrame()->setHtml(sHtml, pReply->url());
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Fill form and create web proxy target request
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::slot_proxyFormPageLoaded(bool bLoaded)
{
    qDebug() << "WEBPROXY: FILLING PROXY INPUT FORM";

    int i = 0;

    // Disconnect mpWebProxyPage: required due to submit action
    disconnect(mpWebProxyPage->mainFrame(), SIGNAL(loadFinished(bool)) , this,  SLOT(slot_proxyFormPageLoaded(bool)));
    mpWebProxyPageTimer->stop();

    if (mbWebProxyCanceled)
    {
        mbWebProxyCanceled         = false;

        return;
    }

    if (!bLoaded)
    {
        qDebug() << "WEBPROXY: FAILURE - PAGE NOT LOADED - RESTART " << mpWebProxyPage->mainFrame()->url().toString();

        // Request failed - start new request
        emit updateWebProxyState(QString("WebProxy: Http webproxy request failed...."));
        getNewWebProxy(msTargetUrl);

        return;
    }

    // Identify webproxy input form and sumbit button -------------------------------------------*/
    // WebProxy mask pattern #1:
    //    <form action=" ....php?..."  method="post" >
    //        <input .. type="text" ..></input>
    //        <input .. type="submit" ..></input>
    //    </form>
    //
    // WebProxy mask pattern #2:
    //    <form action=" ..../query..."  method="post" >
    //        <input .. type="text" ..></input>
    //        <input .. type="submit" ..></input>
    //    </form>
    // WebProxy mask hidemyass.com
    //    <form ...>
    //      <input .. type="text" ..></input> := This is the country input field
    //      <input .. type="text" ..></input> := This is the url input field
    //      <button .. type="submit" ..></button>
    //    </form>

    // Identify all form elements with method=post on page:
    QWebElement webDocument             = mpWebProxyPage->mainFrame()->documentElement();
    QWebElementCollection webFormList   = webDocument.findAll("form[method=post]");

    // When there is no form element with method=post attribute search for all forms
    if (webFormList.count() == 0)
    {
        qDebug() << "no form[method=post] found";
        webFormList = webDocument.findAll("form");
    }

    QWebElement textInputField;
    QWebElement submitButton;
    QString     sFormAction;

    qDebug() << "URL " << mpWebProxyPage->mainFrame()->url().toString();
    qDebug() << "URL is hma " << mpWebProxyPage->mainFrame()->url().toString().contains("hidemyass");

    // For each webelement from form list
    for(i = 0; i < webFormList.count(); i++ )
    {
        // Get all form element with elements
        sFormAction = webFormList.at(i).attribute("action");
        qDebug() << "sFormAction " <<  sFormAction;

        if (!sFormAction.isEmpty())
        {
            // Check if form-action contains ".php"  or "/query"
            if (sFormAction.contains(".php") || sFormAction.contains("/query") || sFormAction.contains("/") || sFormAction.contains("#"))
            {
                qDebug() << "WEBPROXY: FORM FOUND ON " << mpWebProxyPage->mainFrame()->url().toString();

                // Extract input field and submit buttons
                textInputField  =  webFormList.at(i).findFirst("input[type=text]");
                submitButton    =  webFormList.at(i).findFirst("input[type=submit]");
                if (submitButton.isNull())
                    submitButton    =  webFormList.at(i).findFirst("button[type=submit]");

                break;
            }
            else
            {
                // Find other input form
            }
        }
        // Special handling for hidemyass.com
        else if (mpWebProxyPage->mainFrame()->url().toString().contains("hidemyass"))
        {
//            qDebug() << "WEBPROXY: HIDEMYASS MATCH";
//            webFormList.at(i).setAttribute("method","get");


            // Extract input field and submit buttons
            textInputField = webFormList.at(i).findFirst("input[class=\"connect-url form-control\"]");
            qDebug() << "textInputField attributes";
            foreach(QString att, textInputField.attributeNames())
                qDebug() << att;
            submitButton    =  webFormList.at(i).findFirst("input[type=submit]");
            if (submitButton.isNull())
                submitButton    =  webFormList.at(i).findFirst("button[type=submit]");
            qDebug() << "submitButton attributes";
            foreach(QString att, submitButton.attributeNames())
                qDebug() << att;

            break;
        }
//        else
//        {
//            // Extract input field and submit buttons
//            textInputField  =  webFormList.at(i).findFirst("input[type=text]");
//            qDebug() << "textInputField " << textInputField.attributeNames();
//            submitButton    =  webFormList.at(i).findFirst("input[type=submit]");
//            if (submitButton.isNull())
//                submitButton    =  webFormList.at(i).findFirst("button[type=submit]");

//            qDebug() << "submitButton " << submitButton.attributeNames();
//            break;

//        }
    }

    if (!textInputField.isNull() && !submitButton.isNull())
    {
        // Enter target url into text field
        if (mpWebProxyPage->mainFrame()->url().toString().contains("hidemyass"))
        {
            textInputField.setAttribute("placeholder", msTargetUrl);
            qDebug() << "textInputField placeholder " << textInputField.attribute("placeholder");
        }
        else
        {
            textInputField.setAttribute("value", msTargetUrl);
            qDebug() << "textInputField " << textInputField.attribute("value");
        }
        // Submit link request
        emit updateWebProxyState(QString("WebProxy: Send download request for current files...."));

        // Connect mpWebProxyPage: required due to submit action
        connect(mpProxyManager, SIGNAL(finished(QNetworkReply*)) ,this,  SLOT(slot_proxyTargetPageLoaded(QNetworkReply*)));

        mpWebProxyPageTimer->start(WAIT_TIME);
        QWebSettings* mpWebProxyPageSettings = mpWebProxyPage->settings();
        mpWebProxyPageSettings->setAttribute(QWebSettings::JavascriptEnabled, true);

        // Press submit button
        submitButton.evaluateJavaScript("this.click()");
     //   mpWebProxyPageSettings->setAttribute(QWebSettings::JavascriptEnabled, false);

        qDebug() << "WEBPROXY: SUBMITTING REQUEST TO " << mpWebProxyPage->mainFrame()->url().toString();
    }
    else
    {
        qDebug() << "WEBPROXY: NO INPUT FORM FOUND - RESTART " << mpWebProxyPage->mainFrame()->url().toString();;

        // Can't identify form - start new request
        emit updateWebProxyState(QString("WebProxy: Http webproxy request failed...."));
        getNewWebProxy(msTargetUrl);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Receive requested target page
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::slot_proxyTargetPageLoaded(QNetworkReply* pReply)
{
     qDebug() << "WEBPROXY: PROXY PAGE LOADED " << mpWebProxyPage->mainFrame()->url().toString();;
    qDebug() << "all  " << mpWebProxyPage->mainFrame()->toHtml();
    disconnect(mpProxyManager, SIGNAL(finished(QNetworkReply*)) ,this, SLOT(slot_proxyTargetPageLoaded(QNetworkReply*)));
    mpWebProxyPageTimer->stop();

    if (mbWebProxyCanceled)
    {
        mbWebProxyCanceled         = false;

        return;
    }

    QUrl targetUrl;
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    // Check redirection link
    if (redirectionTarget.toUrl().isValid() && (redirectionTarget.toUrl().toString().contains("http://") || redirectionTarget.toUrl().toString().contains("https://")))
    {
        targetUrl =  redirectionTarget.toUrl();
    }
    else
    {
        targetUrl = (QUrl)(pReply->url().toString() + redirectionTarget.toUrl().toString());
    }

    // First reply is proxified redirection to target page
    if (!pReply->error() && !redirectionTarget.toUrl().toString().isEmpty())
    {
        qDebug() << "WEBPROXY: REDIRECTION - LOADING PROXIFIED URL " << redirectionTarget.toUrl().toString();

        // following redirection to target page
        mpWebProxyPageTimer->start(WAIT_TIME);
        connect(mpProxyManager, SIGNAL(finished(QNetworkReply*)) ,this,  SLOT(slot_proxyTargetPageLoaded(QNetworkReply*)));
        mpRequest->setUrl(targetUrl);

        if(mpReply != NULL)
            mpReply->deleteLater();
        mpReply = mpProxyManager->get(*mpRequest);
    }
    else if (pReply->error() || pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty())
    {
        qDebug() << "WEBPROXY: PROXY REQUEST FAILED ON TARGET - ERROR " << pReply->error();

        qDebug() << "readAll " << pReply->readAll();

        // Request failed - start new request
        emit updateWebProxyState(QString("WebProxy: Http webproxy request failed...."));
        getNewWebProxy(msTargetUrl);
    }
    else
    {
        // WORKARROUND: Remove iFrames from webpage, which might causes webpage to crash
        QString sHtml=  QString::fromUtf8(pReply->readAll(), pReply->size());
        sHtml.replace("iFrame","a", Qt::CaseInsensitive);
        // Disable google scripts
        sHtml.replace("google","elgoog");
        mpWebProxyPage->mainFrame()->setContent(sHtml.toUtf8(), "text/html",pReply->url());
        extractWebProxyPrefix();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Extract WebProxy prefix to generate Hotlinking
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::extractWebProxyPrefix()
{
    qDebug() << "WEBPROXY: EXTRACT WEBPROXY PREFIX FROM " << mpWebProxyPage->mainFrame()->url().toString();
     qDebug() << "all  " << mpWebProxyPage->mainFrame()->toHtml();

    // Stop any action of the loaded page - required to avoid auto-redirects
    mpWebProxyPage->action(QWebPage::StopScheduledPageRefresh);

    // Check if there is any known WebProxy script
    bool bValidPorxyUrl  = true;
    QString sWebproxyUrl = mpWebProxyPage->mainFrame()->url().toString();

    // Identify used WebProxy script
    if ( sWebproxyUrl.contains(QString("hidemyass")) )
    {
        // Special handling for hidemyass.com
        // Hidemyass.com-URL-structure:
        // "http://" + server number + ".hidemyass.com/ip-" + IP number" + "/encoded/" + base64 encoded URL
        // encoded URL does not have 'http' prefix
        int iContentTag = sWebproxyUrl.indexOf("/encoded/");
        if (iContentTag > 0)
        {
            sWebproxyUrl = sWebproxyUrl.left(iContentTag + QString("/encoded/").length());
        }
        else
        {
            bValidPorxyUrl = false;
        }
    }
    else if (sWebproxyUrl.contains(QString("index.php?q=")))
    {
        sWebproxyUrl = sWebproxyUrl.left(sWebproxyUrl.indexOf("index.php?q=") + QString("index.php?q=").length());
    }
    else if (sWebproxyUrl.contains(QString("browse.php?u=")))
    {
        sWebproxyUrl = sWebproxyUrl.left(sWebproxyUrl.indexOf("browse.php?u=") + QString("browse.php?u=").length());
    }
    else if (sWebproxyUrl.contains(QString("secure.php?u=")))
    {
        sWebproxyUrl = sWebproxyUrl.left(sWebproxyUrl.indexOf("secure.php?u=") + QString("secure.php?u=").length());
    }
    else if (sWebproxyUrl.contains(QString("b.php?u=")))
    {
        sWebproxyUrl = sWebproxyUrl.left(sWebproxyUrl.indexOf("b.php?u=") + QString("b.php?u=").length());
    }
    else if (sWebproxyUrl.contains(QString("php.php?u=")))
    {
        sWebproxyUrl = sWebproxyUrl.left(sWebproxyUrl.indexOf("php.php?u=") + QString("php.php?u=").length());
    }
    else
    {
        // unknown scritp
        bValidPorxyUrl = false;

        qDebug() << "WEBPROXY: UNKNWON WEBPROXY TPYE " << msCurProxyUrl;

        // Add to blacklist
        if (!msCurProxyUrl.isEmpty() && !msBlockedProxyList.contains(msCurProxyUrl))
            msBlockedProxyList.append(msCurProxyUrl);
    }

    qDebug() << "WEBPROXY: URL PREFIX IS: " << sWebproxyUrl;

    // Check if document is not blocked due to copyright
    QWebElement document = mpWebProxyPage->mainFrame()->documentElement();

    // Check if Hathitrust document is restricted
    if (!checkBookRestriction(&document))
    {
         qDebug() << "WEBPROXY: RECEIVED EMPTY DOCUMENT (REASON: COPYRIGHT)";

        emit updateWebProxyState(QString("Received empty document due to copyright restrictions..."));
        bValidPorxyUrl = false;
    }

    if(bValidPorxyUrl && mbWebProxyIsEnabled)
    {
        // Check if WebProxy is already known
        int i = mpWebProxyQueuingList->sHostList.indexOf(sWebproxyUrl);
        if ( i < 0 )
        {
            qDebug() << "WEBPROXY: DOWNLOAD REQUEST ACCEPTED";

            // New WebProxy : Go on
            msWebproxyPrefix = sWebproxyUrl;

            emit updateWebProxyState(QString("WebProxy: <font color='green'>Download request accepted...</font>downloading data."));
        }
        else
        {
            // Check if download capability is already exhausted
            QDateTime CurrentDate = QDateTime::currentDateTime();
            if ( mpWebProxyQueuingList->uiHoldTimeList.at(i) < CurrentDate.toTime_t() )
            {
                // Download capability is available
                emit updateWebProxyState(QString("WebProxy: <font color='green'>Download request accepted...</font>downloading data."));
                msWebproxyPrefix = sWebproxyUrl;
            }
            else
            {
                // Proxy is still blocked - wait for new WebProxy request
                emit updateWebProxyState(QString("WebProxy: Download limitation: WebProxy still blocked...[%1 sec left]").arg(mpWebProxyQueuingList->uiHoldTimeList.at(i) - CurrentDate.toTime_t()));
                msWebproxyPrefix = "";
            }
        }
        // Copy cookie session to download network manager
        mpDownloadManager->setCookieJar(mpProxyManager->cookieJar());
    }

    // Delete current loaded webpage and re-enable java script
    mpWebProxyPage->mainFrame()->setHtml(QString(""));
    mpWebProxyPage->mainFrame()->load(QUrl("about:blank"));

    if (bValidPorxyUrl)
    {
        QDateTime currentDate = QDateTime::currentDateTime();

        // Toggle flag to enable download
        mbWebProxyRequestActivated = false;
        // Set timestamp
        muiWebproxyTimestamp = currentDate.toTime_t();
    }
    else
    {
        qDebug() << "WEBPROXY: INVALID PROXY URL - RESTART";

        // Start new WebProxy request
        getNewWebProxy(msTargetUrl);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Check if document is accessible / country restriction due to copyright
 *-----------------------------------------------------------------------------------------------*/
bool WebProxy::checkBookRestriction(QWebElement *pDocument)
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
 * Handle Ssl errors
 *-----------------------------------------------------------------------------------------------*/
void WebProxy::slot_handleSslErrors(QNetworkReply* pReply, const QList<QSslError> &errors)
{
    qDebug() << "WEBPROXY: SSL ERROR DETECTED";
    foreach (QSslError e, errors)
    {
        qDebug() << "SSL ERROR: " << e;
    }
    pReply->ignoreSslErrors();
}


/*===============================================================================================*
 *  Web Page methods
 *===============================================================================================*/

/*-----------------------------------------------------------------------------------------------*
 * Constructor for re-implementation of QWebPage
 *-----------------------------------------------------------------------------------------------*/
WebProxyWebPage::WebProxyWebPage(QObject* /*parent - suppress unused warning*/)
{
    // init
}

/*-----------------------------------------------------------------------------------------------*
 * Destructor for re-implementation of QWebPage
 *-----------------------------------------------------------------------------------------------*/
WebProxyWebPage::~WebProxyWebPage(void)
{
    // Deinit
}

/*-----------------------------------------------------------------------------------------------*
 * createWindow for re-implementation of QWebPage: blocks all new windows
 *-----------------------------------------------------------------------------------------------*/
QWebPage* WebProxyWebPage::createWindow(QWebPage::WebWindowType type)
{
    if (type != 0){}; /* suppress unused warning*/
    // Do nothing == POP-UP BLOCKED

    return NULL;
}
