
/*-----------------------------------------------------------------------------------------------*
 * Class definition:    AutoProxy                                                                *
 * The AutoProxy class is used to establish a connection with a remote proxy server. Therefore   *
 * QNetworkProxy is used.                                                                        *
 * Two modes are available. 1) User defined proxy                                                *
 *                          2) Automatic proxy                                                   *
 *                                                                                               *
 * For 1) 'User defined proxy' the user has to set at least host-IP, port and proxy type. In     *
 * addition username and password might be set for authentification purposes.                    *
 * Mode 2) automatically obtains proxy data from the internet (http / socks only).               *
 * To verify that a connection has been established hathitrust.org homepage will be loaded.      *
 *                                                                                               *
 *-----------------------------------------------------------------------------------------------*/
//
// NOTE: DOES NOT WORK PROPERLY WITH QT < QT_VERSION 4.8.0
// Most network requests are replied with QNetworkReply::UnknownNetworkError
//
// NOTE: There seems to be a bug for network request in Qt4.8. The following terminal output occurs:
//
// QNetworkReplyImplPrivate::error: Internal problem, this method must only be called once.
//
// Since this seems to be a bug in Qt framework which does not affect the application it is ignored


#include "autoproxy.h"
#include "common.h"

#include <QWebPage>
#include <QWebFrame>
#include <QWebElementCollection>
#include <QStringList>
#include <QDateTime>
#include <QDebug>

// Avoid redefinition of same connection between same elements
#define connect(src, sig, trgt, slt) connect(src, sig, trgt, slt, Qt::UniqueConnection)

#define MAXNUMPROXIES     20        ///> Maximum number of proxies extracted from proxy source list
#define MAX_NETWORKERRORS 10        ///> Maximum number of unknown network errors in a row before (on verifying connection ) before resetting proxy connection

// Default values for proxy login
#define PROXY_USER "none"
#define PROXY_PASS "none"

#define AUTOPROXY_RESET_ALL_PROXYLISTS             0
#define AUTOPROXY_RESET_US_PROXYLIST               1
#define AUTOPROXY_RESET_OTHER_PROXYLIST            2

#define AUTOPROXY_TIMEOUT    12000   // ms
#define AUTOPROXY_TIMEOUT_1S 1000    // 1s
#define AUTOPROXY_RESTART    35      // s

#define AUTO_PROXY_FILE_VERSION "1.1"

#if QT_VERSION < QT_VERSION_CHECK(4, 8, 0)
    #warning AUTO PROXY FEATURE DOES NOT WORK PROPERLY WITH QT < QT_VERSION 4.8.0
#endif

/*-----------------------------------------------------------------------------------------------*
 * Constructor of AutoProxy
 *-----------------------------------------------------------------------------------------------*/
AutoProxy::AutoProxy(QWidget *parent):
    mpParent(parent)
{
    initAutoProxy();
}

/*-----------------------------------------------------------------------------------------------*
 * Init AutoProxy
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::initAutoProxy()
{
    // Use independet network access manager for proxy requests
    mpProxy            = new QNetworkProxy;
    mpNetwProxyManager = new QNetworkAccessManager(this);
    connect(mpNetwProxyManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(slot_handleSslErrors(QNetworkReply*,QList<QSslError>)));
    connect(mpNetwProxyManager, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this, SLOT(slot_onProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));
    connect(mpNetwProxyManager, SIGNAL(networkSessionConnected()), this, SLOT(slot_onNetworkSessionConnected()));

    mpProxySrcPage = new QWebPage();
    QWebSettings* pWebProxyPageSettings = mpProxySrcPage->settings();
    pWebProxyPageSettings->setAttribute( QWebSettings::JavascriptEnabled, false);
    pWebProxyPageSettings->setAttribute( QWebSettings::JavascriptCanOpenWindows, false);
    pWebProxyPageSettings->setAttribute( QWebSettings::JavaEnabled, false);
    pWebProxyPageSettings->setAttribute( QWebSettings::AutoLoadImages, false);
    pWebProxyPageSettings->setAttribute( QWebSettings::LocalContentCanAccessFileUrls, false);
    pWebProxyPageSettings->setAttribute( QWebSettings::XSSAuditingEnabled, true);
    pWebProxyPageSettings->setAttribute( QWebSettings::PluginsEnabled, false);

    // Timer for download retries
    mpRetryTimer = new QTimer();
    mpRetryTimer->setSingleShot(true);
    connect(mpRetryTimer , SIGNAL(timeout()), this, SLOT(slot_cancelAutoProxyRequest()));


    // Timer for restarting autoproxy on network error
    mpRestartAutoProxyTimer = new QTimer();
    mpRestartAutoProxyTimer->setSingleShot(true);
    connect(mpRestartAutoProxyTimer , SIGNAL(timeout()), this, SLOT(slot_restartAutoProxyRequest()));


    mpProxyReply            = NULL;
    mpProxySrcChkReply      = NULL;
    mbUsProxyOnly           = true;
    mbAutoProxyIsBusy       = false;
    mbCheckProxyOnTargetUrl = true;
    mbAbortRequest          = false;
    mbLocalHostEnabled      = true;
    miTrials                = 0;
    miNumDownloads          = 0;
    miUnknownNetworkErrors  = 0;
    msMsgPrefix             = "Auto";

    QDateTime currentDate = QDateTime::currentDateTime();

    // Queuing list for all countries proxies
    mpProxyQueuingList       = new tProxyQueuingList();     // queuing list
    mpProxyQueuingList->sHostList.append("localhost");      // add local host
    mpProxyQueuingList->uiHoldTimeList.append(currentDate.toTime_t());
    mpProxyQueuingList->uiNumDownloads.append((uint) 0);
    mpProxyQueuingList->proxyList.append(QUrl("localhost"));

    // Queuing list for US only proxies    
    mpUsProxyQueuingList  = new tProxyQueuingList();     // queuing list
    mpUsProxyQueuingList->sHostList.append("localhost"); // add local host
    mpUsProxyQueuingList->uiHoldTimeList.append(currentDate.toTime_t());
    mpUsProxyQueuingList->uiNumDownloads.append((uint) 0);
    mpUsProxyQueuingList->proxyList.append(QUrl("localhost"));

    // Load list of webpages with proxy server from file
    slot_loadAutoProxySourceFile();
}

/*-----------------------------------------------------------------------------------------------*
 * Handle Ssl errors on book info request
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_handleSslErrors(QNetworkReply* pReply, const QList<QSslError> &errors)
{
    qDebug() << "AUTO PROXY: SSL ERROR DETECTED";
    foreach (QSslError e, errors)
    {
        qDebug() << "SSL ERROR: " << e;
    }
    pReply->ignoreSslErrors();
}

/*-----------------------------------------------------------------------------------------------*
 * Handle authentication request
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_onProxyAuthenticationRequired(const QNetworkProxy &p, QAuthenticator *auth)
{
    QNetworkProxy proxy = QNetworkProxy::applicationProxy();
    auth->setUser(proxy.user());
    auth->setPassword(proxy.password());
}


/*-----------------------------------------------------------------------------------------------*
 * Handle session connected
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_onNetworkSessionConnected(void)
{
    qDebug() << "AUTO PROXY: SESSION CONNECTED";
}

/*-----------------------------------------------------------------------------------------------*
 * Generate default proxy source list for:
 *  0: general reset,
 *  1: only all country proxy list,
 *  2: only US proxy list
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::initDefaultProxySources(int iList)
{
    if ((iList == AUTOPROXY_RESET_ALL_PROXYLISTS) || (iList == AUTOPROXY_RESET_OTHER_PROXYLIST))
    {
        // ALL COUNTRIES PROXY SOURCES
        msProxySourcesUrls.clear();
        msProxySourcesUrls
#if ENABLE_HTTPS_PROXY
            // HTTPS proxies
            << "http://www.idcloak.com/proxylist/ssl-proxy-list.html"
            << "http://list.proxylistplus.com/SSL-Proxy"
            << "http://www.freeproxylists.net/?c=&pt=&pr=HTTPS&a[]=0&a[]=1&a[]=2&u=90"
            << "http://www.xroxy.com/proxylist.php?port=&type=&ssl=ssl&country=&latency=&reliability=#table"
            << "http://proxyhttp.net/free-list/proxy-https-security-anonymous-proxy/"
            << "http://proxy.moo.jp/?c=&pt=&pr=HTTPS&a[]=0&a[]=1&a[]=2&u=90"
            << "https://api.getproxylist.com/proxy?allowsHttps=1&minUptime=75"
            << "http://gimmeproxy.com/api/getProxy?supportsHttps=false&maxCheckPeriod=600"
#endif
#if ENABLE_SOCKS_PROXY
            // SOCKET 5 proxies
            << "http://www.socks-proxy.net/"
            << "http://proxylist.hidemyass.com/search-1302136#listable"
            << "http://www.xroxy.com/proxylist.php?port=&type=Socks5&ssl=ssl&country=&latency=1000&reliability=9000#table"
            << "http://gatherproxy.com/sockslist"
            << "http://sockslist.net/list/proxy-socks-5-list/"
            << "http://list.proxylistplus.com/Socks-Proxy";
#endif
;
    }
    if ((iList == AUTOPROXY_RESET_ALL_PROXYLISTS) || (iList == AUTOPROXY_RESET_US_PROXYLIST))
    {
        // US ONLY PROXY SOURCES
        msProxySourcesUrlsUsOnly.clear();
        msProxySourcesUrlsUsOnly
#if ENABLE_HTTPS_PROXY

            << "http://www.xroxy.com/proxylist.php?port=&type=All_http&ssl=ssl&country=US&latency=5000&reliability=9000#table"
            << "http://www.freeproxylists.net/?c=US&pt=&pr=HTTPS&a[]=0&a[]=1&a[]=2&u=70"
            << "http://proxy.moo.jp/?c=US&pt=&pr=HTTPS&a[]=0&a[]=1&a[]=2&u=80"
            << "https://api.getproxylist.com/proxy?allowsHttps=1&minUptime=75&country[]=US"
            << "http://gimmeproxy.com/api/getProxy?supportsHttps=false&maxCheckPeriod=600&country=US"
#endif
#if ENABLE_SOCKS_PROXY
            << "http://www.xroxy.com/proxylist.php?port=&type=Socks5&ssl=ssl&country=US&latency=1000&reliability=9000#table"
#endif
      ;
    }
            // Candidates: requires further features to match key words, e.g. https
            //<< "http://www.idcloak.com/proxylist/free-proxy-list-usa.html" // http + https
            //<< "https://nordvpn.com/free-proxy-list/?country=United+States&ports=&speed[1]=on&anon[High]=on&by=c&order=ASC&perpage=25"; // http + https
            //<< "http://proxygaz.com/country/united-states-of-america-proxy"/ // http + https

    // Blacklisted proxy sources which became incompatible with HDH
    msProxySourcesUrlsBlacklist.clear();
    msProxySourcesUrlsBlacklist
            << "http://proxylist.hidemyass.com/search-1297492#listable"
            << "http://proxylist.hidemyass.com/search-1304558#listable"
            << "http://www.sslproxies.org/"
            << "http://proxylist.hidemyass.com/search-1308329#listable"
            << "http://www.us-proxy.org/"
            << "http://www2.waselproxy.com/free-usa-proxy-list-2/"
            << "http://free-proxy-list.net/"
            << "http://www.xroxy.com/proxylist.php?port=&type=&ssl=&country=&latency=1000&reliability=9000#table"
            << "http://www.proxylisty.com/ip-proxylist"
            << "http://www.idcloak.com/proxylist/proxy-list.html"
            << "https://nordvpn.com/free-proxy-list/1/?allc=all&allp=all&port&sp[0]=1&protocol[0]=HTTP&protocol[1]=HTTPS&protocol[2]=SOCKS4&protocol[3]=SOCKS4%2F5&protocol[4]=SOCKS5&ano[0]=High&sortby=0&way=0&pp=1"
            << "http://proxylist.hidemyass.com/search-1305984#listable"
            << "http://www.us-proxy.org/"
            << "http://www.xroxy.com/proxylist.php?port=&type=&ssl=&country=US&latency=1000&reliability=9000#table"
            << "http://www.proxylisty.com/country/United%20States-ip-list"
            << "http://www.idcloak.com/proxylist/free-proxy-list-usa.html"
            << "https://nordvpn.com/free-proxy-list/1/?country[0]=US&allp=all&port&sp[0]=1&protocol[0]=HTTP&protocol[1]=HTTPS&protocol[2]=SOCKS4&protocol[3]=SOCKS4%2F5&protocol[4]=SOCKS5&ano[0]=High&sortby=0&way=0&pp=1"
#if !ENABLE_SOCKS_PROXY
            // SOCKET 5 proxies
            << "http://www.socks-proxy.net/"
            << "http://proxylist.hidemyass.com/search-1302136#listable"
            << "http://www.xroxy.com/proxylist.php?port=&type=Socks5&ssl=ssl&country=&latency=1000&reliability=9000#table"
            << "http://gatherproxy.com/sockslist"
            << "http://sockslist.net/list/proxy-socks-5-list/"
            << "http://list.proxylistplus.com/Socks-Proxy"
#endif
#if !ENABLE_SOCKS_PROXY
            << "http://www.xroxy.com/proxylist.php?port=&type=Socks5&ssl=ssl&country=US&latency=1000&reliability=9000#table"
#endif
            << "http://proxylist.hidemyass.com/search-1305711#listable"
            << "http://www.xroxy.com/proxylist.php?port=&type=&ssl=ssl&country=&latency=1000&reliability=9000#table"
            ;
}

/*-----------------------------------------------------------------------------------------------*
 * Generate proxy source file prxyscrs.xml
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_generateAutoProxySourceFile(void)
{
    generateAutoProxySourceFile();
}

/*-----------------------------------------------------------------------------------------------*
 * Load proxy source file prxyscrs.xml
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_loadAutoProxySourceFile(void)
{
    QString sProxySources = readFile("prxysrcs.xml");
    if (sProxySources.isEmpty())
    {
        // Init default proxy sources
        initDefaultProxySources();
        // Generate default proxy file
        generateAutoProxySourceFile();
    }

    // Generate
    readAutoProxySourceFile(&sProxySources);
}

/*-----------------------------------------------------------------------------------------------*
 * Generate proxy source file prxyscrs.xml
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::generateAutoProxySourceFile(void)
{
    QString* psProxySources = new QString();

    psProxySources->clear();
    psProxySources->append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    psProxySources->append("<proxyversion>" + QString(AUTO_PROXY_FILE_VERSION) + "</proxyversion>\n");
    psProxySources->append("<proxysources>\n");
    // Add proxy sources URLs
    for (int i=0; i < msProxySourcesUrls.count(); i++)
    {
        psProxySources->append("<proxysource>");
        psProxySources->append("<url>");
        psProxySources->append(msProxySourcesUrls.at(i));
        psProxySources->append("</url>");
        psProxySources->append("<score>10");
        psProxySources->append("</score>");
        psProxySources->append("</proxysource>\n");
    }
    psProxySources->append("</proxysources>\n");
    psProxySources->append("<proxysourcesusonly>\n");
    // Add proxy sources URLs
    for (int i=0; i < msProxySourcesUrlsUsOnly.count(); i++)
    {
        psProxySources->append("<proxysourceusonly>");
        psProxySources->append("<url>");
        psProxySources->append(msProxySourcesUrlsUsOnly.at(i));
        psProxySources->append("</url>");
        psProxySources->append("<score>10");
        psProxySources->append("</score>");
        psProxySources->append("</proxysourceusonly>\n");
    }
    psProxySources->append("</proxysourcesusonly>\n");

    writeFile(QString("prxysrcs.xml"), psProxySources);
}

/*-----------------------------------------------------------------------------------------------*
 * Read proxy source file prxysrcs.xml
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::readAutoProxySourceFile(QString* psProxySources)
{
    QString sXmlItem, sUrl, sScore;
    int iIdxNode = 0, iIdxItem = 0;
    bool bRemoveBlacklisted = false;
    bool bUseDefaultOnly = false;

    // File version
    if((sXmlItem = myXmlReader(psProxySources, "proxyversion", &iIdxNode)).isEmpty())
    {
        sXmlItem = "1.0";
        bUseDefaultOnly = true; // Force update - all user proxies will stored in backup file
        writeFile(QString("prxysrcs.xml.bak"), psProxySources);
    }

    // Update required if version is outdated
    if (sXmlItem != AUTO_PROXY_FILE_VERSION)
    {
        // Add default proxies
        initDefaultProxySources();
        // Remove blacklisted proxy sources
        bRemoveBlacklisted = true;
    }

    if (!bUseDefaultOnly)
    {
        // Proxy sources URLs
        while(!(sXmlItem = myXmlReader(psProxySources, "proxysource", &iIdxNode)).isEmpty())
        {
            iIdxItem    = 0;
            sUrl        = myXmlReader(&sXmlItem, "url", &iIdxItem);
            sScore      = myXmlReader(&sXmlItem, "score", &iIdxItem);

            if (!sUrl.isEmpty() && !sScore.isEmpty())
            {
                if (bRemoveBlacklisted && msProxySourcesUrlsBlacklist.contains(sUrl))
                {
                    // Do nothing
                }
                else if (!msProxySourcesUrls.contains(sUrl))
                {
                    msProxySourcesUrls.append(sUrl);
                    miProxySourcesScore.append(sScore.toInt());
                }
            }
        }

        // Proxy sources URLs US only
        iIdxNode = 0;
        while(!(sXmlItem = myXmlReader(psProxySources, "proxysourceusonly", &iIdxNode)).isEmpty())
        {
            iIdxItem    = 0;
            sUrl        = myXmlReader(&sXmlItem, "url", &iIdxItem);
            sScore      = myXmlReader(&sXmlItem, "score", &iIdxItem);

            if (!sUrl.isEmpty() && !sScore.isEmpty())
            {
                if (bRemoveBlacklisted && msProxySourcesUrlsBlacklist.contains(sUrl))
                {
                    // Do nothing
                }
                else if (!msProxySourcesUrlsUsOnly.contains(sUrl))
                {
                    msProxySourcesUrlsUsOnly.append(sUrl);
                    miProxySourcesScore.append(sScore.toInt());
                }
            }
        }
    }

    if (bUseDefaultOnly || bRemoveBlacklisted)
    {
        // Update file
        generateAutoProxySourceFile();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Get proxy list from online sources
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::getProxyServerList(void)
{
    int i = 0;

    // Disable old proxy
    mpProxy->setPort(QNetworkProxy::NoProxy);
    QNetworkProxy::setApplicationProxy(*mpProxy);

    QString sProxySouceUrl;
    do
    {
        if (mbUsProxyOnly)
        {
            sProxySouceUrl = msProxySourcesUrlsUsOnly.at(qrand()%msProxySourcesUrlsUsOnly.count());
        }
        else
        {
            sProxySouceUrl = msProxySourcesUrls.at(qrand()%msProxySourcesUrls.count());
        }

        if (!mCaptchaProtectedUrl.isEmpty() && mCaptchaProtectedUrl.contains(QUrl(sProxySouceUrl)))
        {
            emit updateAutoProxyState("AutoProxy: <font color='red'>Proxy source blocked by captcha...</font>");
            qDebug() << "AUTO PROXY: PROXY SOURCE BLOCKED BY CAPTCHA (FAILS: " << i << ")";
        }

    }while(!mCaptchaProtectedUrl.isEmpty() && mCaptchaProtectedUrl.contains(QUrl(sProxySouceUrl)) && (i++ < 100));

    if (i < 100)
    {
        qDebug() << "AUTO PROXY: PROXY SOURCE URL: " << sProxySouceUrl;

        emit updateAutoProxyState("AutoProxy: Requesting proxy list ...");

        // Send get command to target URL
        QUrl proxySouceUrl(sProxySouceUrl);
        QNetworkRequest requestProxySourcePage(proxySouceUrl);
        requestProxySourcePage.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");
        if (mpProxyReply != NULL)
            mpProxyReply->deleteLater();
        mpProxyReply = mpNetwProxyManager->get(requestProxySourcePage);
        connect(mpProxyReply, SIGNAL(finished()), this, SLOT(slot_extractProxyList()));
        mpRetryTimer->start(AUTOPROXY_TIMEOUT);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Get proxy list from online sources - check online source state
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::checkProxyServerSource(QString sProxySouceUrl)
{
    emit updateAutoProxyCheckState(msMsgPrefix + "Proxy: Requesting proxy list... please wait");

    qDebug() << "AUTO PROXY: CHECKING PROXY SOURCE URL: " << sProxySouceUrl;

    // Send get command to target URL
    QUrl proxySouceUrl(sProxySouceUrl);
    QNetworkRequest requestProxySourcePage(proxySouceUrl);
    requestProxySourcePage.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");
//    requestProxySourcePage.setRawHeader("Accept-Language", QByteArray ("en-US,*")); => causes captcha check on http://www.freeproxylists.net/
    if (mpProxySrcChkReply != NULL)
        mpProxySrcChkReply->deleteLater();
    mpProxySrcChkReply = mpNetwProxyManager->get(requestProxySourcePage);
    connect(mpProxySrcChkReply, SIGNAL(finished()), this, SLOT(slot_checkProxySourceReply()));
    mpRetryTimer->start(AUTOPROXY_TIMEOUT);
}

/*-----------------------------------------------------------------------------------------------*
 * Extract proxy list from proxy request
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_extractProxyList()
{
    slot_extractProxyList(mpProxyReply);
}

/*-----------------------------------------------------------------------------------------------*
 * Extract proxy list from proxy request
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_extractProxyList(QNetworkReply* pReply)
{
    mpRetryTimer->stop();
    disconnect(mpProxyReply, SIGNAL(finished()), this, SLOT(slot_extractProxyList()));
    if (mbAbortRequest)
    {
        mbAbortRequest      = false;
        mbAutoProxyIsBusy   = false;

        emit finished(false);

        return;
    }

    // Check received download reply
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (        pReply->error()                                                             // error
            || !redirectionTarget.isNull()                                                  // redirection
            ||  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty())    // empty page
    {

        qDebug() << "AUTO PROXY: EXTRACT ERROR";

        emit updateAutoProxyState("AutoProxy: Received empty page ...");
        if (mbUseAutoProxy)
            applyProxyServer();
        else
            emit updateAutoProxyProgressBar(false);

        return;
    }

    // Extract tables from html document
    QString sHtml=  (QString)pReply->readAll();

    // Remove javascript protectionfor DECODE() protected IP addresses
    if (sHtml.contains("IPDecode(",Qt::CaseInsensitive))
    {
        // Remove script
        sHtml.remove(QRegExp("<script(.)[^>]*>IPDecode\\(\"",Qt::CaseInsensitive));
        sHtml.remove(QRegExp("\"\\)</script>",Qt::CaseInsensitive));
        // Translate from percentage
        sHtml = QUrl::fromPercentEncoding(sHtml.toLocal8Bit());
    }
    else if (sHtml.contains("recaptcha/api/noscript"))
    {
        if (mCaptchaProtectedUrl.isEmpty() || !mCaptchaProtectedUrl.contains(pReply->url()))
            mCaptchaProtectedUrl.append(pReply->url());
    }
    // Disable google scripts
    sHtml.replace("google","elgoog");

    mpProxySrcPage->mainFrame()->setHtml(sHtml, pReply->url());
    QWebElement webDocument = mpProxySrcPage->mainFrame()->documentElement();
    QWebElementCollection documentTables = webDocument.findAll(QString("table"));

    QString sProxyListRawData = "";
    QStringList sProxyListElements;
    QUrl proxyUrlCandidate;
    QHostAddress urlAddress;

    // For each table
    qDebug() << "AUTO PROXY: EXTRACT LIST ELEMENTS";
    foreach (QWebElement webTable, documentTables)
    {        
        // Convert to csv plain text
        sProxyListRawData = webTable.toPlainText();
        sProxyListRawData.replace(QRegExp("\t"), ",");
        sProxyListRawData.replace(QRegExp("\n"), ",");

        // Check for valid data separated by tab or space
        sProxyListElements = sProxyListRawData.split(",",QString::SkipEmptyParts);

        int state = 0;
        int iPortNumber = 0;
        bool bOk = false;
        // Identify proxy data: IP + Port + Type
        for (int i = 0; (i < sProxyListElements.count()) && (mUrlProxyList.count() < MAXNUMPROXIES); i++)
        {
            // Check for valid IP. Restart search in case of a match when previous extraction has not been finished
            if (urlAddress.setAddress(sProxyListElements.at(i)))
            {
                if (state != 0)
                {
                    if (proxyUrlCandidate.port() < 0)
                    {
                        proxyUrlCandidate.setPort(80);          // default port
                        proxyUrlCandidate.setUserInfo("http");  // default proxy type
                    }

                    if (!proxyUrlCandidate.toString().contains(QString::number(proxyUrlCandidate.port())))
                        proxyUrlCandidate.setUrl(proxyUrlCandidate.toString() + ":" + proxyUrlCandidate.port());

                    mUrlProxyList.append(proxyUrlCandidate);
                }
                state = 0;
            }

            switch(state)
            {
            //--------------------------------------------------------------------------
            case 0: // Find IP
                // Check for IP
                if (urlAddress.setAddress(sProxyListElements.at(i)))
                {
                    proxyUrlCandidate.clear();
                    proxyUrlCandidate.setHost(sProxyListElements.at(i));
                    state = 1;
                    // no-break;
                }
                else
                    break;
            //--------------------------------------------------------------------------
            case 1: // Find Port
                // Check if port was defined already
                if (proxyUrlCandidate.port() > -1)
                {
                    // Set default proxy type to http                     
                    proxyUrlCandidate.setUserInfo("http");
                    state = 2;
                    // no-break;
                }
                else
                {
                    // Check for port number
                    iPortNumber = sProxyListElements.at(i).toInt(&bOk);
                    if (bOk)
                    {
                        proxyUrlCandidate.setPort(iPortNumber);
                        // Set default proxy type
                        proxyUrlCandidate.setUserInfo("http");
                        state = 2;
                    }

                    break;
                }
            //--------------------------------------------------------------------------
            case 2: // Find Type: http or socket
                if (sProxyListElements.at(i).contains("http",Qt::CaseInsensitive))
                {
                    proxyUrlCandidate.setUserInfo("http");

                    if (proxyUrlCandidate.port() < 0)
                        proxyUrlCandidate.setPort(80);          // default port

                    if (!proxyUrlCandidate.toString().contains(QString::number(proxyUrlCandidate.port())))
                        proxyUrlCandidate.setUrl(proxyUrlCandidate.toString() + ":" + proxyUrlCandidate.port());

                    mUrlProxyList.append(proxyUrlCandidate);
                    state = 0;
                }
                else if (sProxyListElements.at(i).contains("sock",Qt::CaseInsensitive))
                {
                    proxyUrlCandidate.setUserInfo("socks");

                    if (proxyUrlCandidate.port() < 0)
                        proxyUrlCandidate.setPort(80);          // default port

                    if (!proxyUrlCandidate.toString().contains(QString::number(proxyUrlCandidate.port())))
                        proxyUrlCandidate.setUrl(proxyUrlCandidate.toString() + ":" + proxyUrlCandidate.port());

                    mUrlProxyList.append(proxyUrlCandidate);
                    state = 0;
                }
                break;
            //--------------------------------------------------------------------------
            default:
                break;
            }
        }
    }

    // Check if any proxies have been determined
    if (mUrlProxyList.isEmpty())
    {

        // Check plain text for proxy server URLs
        int state = 0;
        int iPortNumber = 0;
        bool bOk = false;
        sProxyListRawData = mpProxySrcPage->mainFrame()->toPlainText();
        sProxyListRawData.replace(QRegExp("\t"), ",");
        sProxyListRawData.replace(QRegExp("\n"), ",");
        sProxyListRawData.remove(QRegExp(" "));
        sProxyListElements = sProxyListRawData.split(",",QString::SkipEmptyParts);
        // Identify proxy data: IP + Port + Type
        for (int i = 0; (i < sProxyListElements.count()) && (mUrlProxyList.count() < MAXNUMPROXIES); i++)
        {
            // check for valid IP. Restart search in case of a match when previous extraction has not been finished
            if (urlAddress.setAddress(sProxyListElements.at(i)))
            {
                if (state != 0)
                {
                    if (proxyUrlCandidate.port() < 0)
                    {
                        proxyUrlCandidate.setPort(80);          // default port
                        proxyUrlCandidate.setUserInfo("http");  // default proxy type
                    }

                    if (!proxyUrlCandidate.toString().contains(QString::number(proxyUrlCandidate.port())))
                        proxyUrlCandidate.setUrl(proxyUrlCandidate.toString() + ":" + proxyUrlCandidate.port());

                    mUrlProxyList.append(proxyUrlCandidate);
                }
                state = 0;
            }

            switch(state)
            {
            //--------------------------------------------------------------------------
            case 0: // Find IP
                // Check for IP
                if (urlAddress.setAddress(sProxyListElements.at(i)))
                {
                    proxyUrlCandidate.clear();
                    proxyUrlCandidate.setHost(sProxyListElements.at(i));
                    state = 1;
                    // no-break;
                }
                else
                    break;
                //--------------------------------------------------------------------------
            case 1: // Find Port
                // Check if port was defined already
                if (proxyUrlCandidate.port() > -1)
                {
                    // Set default proxy type to http
                    proxyUrlCandidate.setUserInfo("http");
                    state = 2;
                    // no-break;
                }
                else
                {
                    // Check for port number
                    iPortNumber = sProxyListElements.at(i).toInt(&bOk);
                    if (bOk)
                    {
                        proxyUrlCandidate.setPort(iPortNumber);
                        // Set default proxy type
                        proxyUrlCandidate.setUserInfo("http");
                        state = 2;
                    }
                    break;
                }
                //--------------------------------------------------------------------------
            case 2: // Find Type: http or socket
                if (sProxyListElements.at(i).contains("http",Qt::CaseInsensitive))
                {
                    proxyUrlCandidate.setUserInfo("http");

                    if (proxyUrlCandidate.port() < 0)
                        proxyUrlCandidate.setPort(80);          // default port

                    if (!proxyUrlCandidate.toString().contains(QString::number(proxyUrlCandidate.port())))
                        proxyUrlCandidate.setUrl(proxyUrlCandidate.toString() + ":" + proxyUrlCandidate.port());

                    mUrlProxyList.append(proxyUrlCandidate);
                    state = 0;
                }
                else if (sProxyListElements.at(i).contains("sock",Qt::CaseInsensitive))
                {
                    proxyUrlCandidate.setUserInfo("socks");

                    if (proxyUrlCandidate.port() < 0)
                        proxyUrlCandidate.setPort(80);          // default port

                    if (!proxyUrlCandidate.toString().contains(QString::number(proxyUrlCandidate.port())))
                        proxyUrlCandidate.setUrl(proxyUrlCandidate.toString() + ":" + proxyUrlCandidate.port());

                    mUrlProxyList.append(proxyUrlCandidate);
                    state = 0;
                }
                break;
                //--------------------------------------------------------------------------
            default:
                break;
            }
        }
    }

    // JSON Api parser -> convert to csv data
    if (mUrlProxyList.isEmpty())
    {
        int state = 0;
        int iPortNumber = 0;
        bool bOk = false;
        sProxyListRawData = mpProxySrcPage->mainFrame()->toPlainText();
        sProxyListRawData.remove(QRegExp("\""));
        sProxyListRawData.replace(QRegExp("\n"), ",");
        sProxyListRawData.replace(QRegExp(":"), ",");
        sProxyListRawData.remove(QRegExp(" "));
        sProxyListRawData.remove(QRegExp("{"));
        sProxyListRawData.remove(QRegExp("}"));
        sProxyListElements = sProxyListRawData.split(",",QString::SkipEmptyParts);

        // Identify proxy data: IP + Port + Type
        for (int i = 0; (i < sProxyListElements.count()); i++)
        {
            // check for valid IP. Restart search in case of a match when previous extraction has not been finished
            if (urlAddress.setAddress(sProxyListElements.at(i)))
            {
                if (state != 0)
                {
                    if (proxyUrlCandidate.port() < 0)
                    {
                        proxyUrlCandidate.setPort(80);          // default port
                        proxyUrlCandidate.setUserInfo("http");  // default proxy type
                    }

                    if (!proxyUrlCandidate.toString().contains(QString::number(proxyUrlCandidate.port())))
                        proxyUrlCandidate.setUrl(proxyUrlCandidate.toString() + ":" + proxyUrlCandidate.port());

                    mUrlProxyList.append(proxyUrlCandidate);;
                }
                state = 0;
            }

            switch(state)
            {
            //--------------------------------------------------------------------------
            case 0: // Find IP
                // Check for IP
                if (urlAddress.setAddress(sProxyListElements.at(i)))
                {
                    proxyUrlCandidate.clear();
                    proxyUrlCandidate.setHost(sProxyListElements.at(i));
                    state = 1;
                    // no-break;
                }
                else
                    break;
                //--------------------------------------------------------------------------
            case 1: // Find Port
                // Check if port was defined already
                if (proxyUrlCandidate.port() > -1)
                {
                    // Set default proxy type to http
                    proxyUrlCandidate.setUserInfo("http");
                    state = 2;
                    // no-break;
                }
                else
                {
                    // Check for port number
                    iPortNumber = sProxyListElements.at(i).toInt(&bOk);
                    if (bOk)
                    {
                        proxyUrlCandidate.setPort(iPortNumber);
                        // Set default proxy type
                        proxyUrlCandidate.setUserInfo("http");
                        state = 2;
                    }
                    break;
                }
                //--------------------------------------------------------------------------
            case 2: // Find Type: http or socket
                if (sProxyListElements.at(i).contains("http",Qt::CaseInsensitive))
                {
                    proxyUrlCandidate.setUserInfo("http");

                    if (proxyUrlCandidate.port() < 0)
                        proxyUrlCandidate.setPort(80);          // default port

                    if (!proxyUrlCandidate.toString().contains(QString::number(proxyUrlCandidate.port())))
                        proxyUrlCandidate.setUrl(proxyUrlCandidate.toString() + ":" + proxyUrlCandidate.port());

                    mUrlProxyList.append(proxyUrlCandidate);
                    state = 0;
                }
                else if (sProxyListElements.at(i).contains("sock",Qt::CaseInsensitive))
                {
                    proxyUrlCandidate.setUserInfo("socks");

                    if (proxyUrlCandidate.port() < 0)
                        proxyUrlCandidate.setPort(80);          // default port

                    if (!proxyUrlCandidate.toString().contains(QString::number(proxyUrlCandidate.port())))
                        proxyUrlCandidate.setUrl(proxyUrlCandidate.toString() + ":" + proxyUrlCandidate.port());

                    mUrlProxyList.append(proxyUrlCandidate);
                    state = 0;
                }
                break;
                //--------------------------------------------------------------------------
            default:
                break;
            }
        }
    }

    // Check if any proxies have been determined
    if (!mUrlProxyList.isEmpty())
    {
        foreach(QUrl sUrl, mUrlProxyList)
        {
            qDebug() << "AUTO PROXY - URL: " << sUrl;
        }

        // Apply a proxy server from the list
        if (mbUseAutoProxy)
            applyProxyServer();
        else
            emit updateAutoProxyProgressBar(false);
    }
    else
    {
        // Restart request for same URL
        getProxyServerList();
    }

    mpProxySrcPage->mainFrame()->setHtml("");
}

/*-----------------------------------------------------------------------------------------------*
 * Check if proxy source has any proxy lists
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_checkProxySourceReply()
{    
    slot_checkProxySourceReply(mpProxySrcChkReply);
}

/*-----------------------------------------------------------------------------------------------*
 * Check if proxy source has any proxy lists  // TODO: combine with slot_extractProxyList()
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_checkProxySourceReply(QNetworkReply* pReply)
{
    static int iTrials = 0;
    bool bCaptchaProtected = false;

    mpRetryTimer->stop();
    disconnect(mpProxySrcChkReply, SIGNAL(finished()), this, SLOT(slot_checkProxySourceReply()));

    // Check received download reply
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (        pReply->error()                                                             // error
                || !redirectionTarget.isNull()                                                  // redirection
                ||  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty())    // empty page
    {
        qDebug() << "AUTO PROXY: PROXY CHECK FAILED FOR URL " << pReply->url().toString();

        qDebug() << "pReply->error() " << pReply->error();
        qDebug() << "!redirectionTarget.isNull()  " << !redirectionTarget.isNull();
        qDebug() << "redirectionTarget  " << redirectionTarget;
        qDebug() << "pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty() " << pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty();

        emit updateAutoProxyCheckState("Received empty page ...");
        emit updateAutoProxyCheckStateResult(false);

         return;
    }

    // Extract tables from html document
    QString sHtml = (QString) pReply->readAll();

    // Remove javascript protection for DECODE() protected IP addresses
    if (sHtml.contains("IPDecode(",Qt::CaseInsensitive))
    {
        qDebug() << "AUTO PROXY - CHECK: IPDECODE DETECTED...DECODING";

        // Remove script
        sHtml.remove(QRegExp("<script(.)[^>]*>IPDecode\\(\"",Qt::CaseInsensitive));
        sHtml.remove(QRegExp("\"\\)</script>",Qt::CaseInsensitive));
        // Translate from percentage
        sHtml = QUrl::fromPercentEncoding(sHtml.toLocal8Bit());
    }
    else if (sHtml.contains("recaptcha/api/noscript"))
    {
        emit updateAutoProxyCheckState("<font color='red'>Proxy source failed. Captcha protection detected!</font>");
        emit updateAutoProxyCheckStateResult(false);

        iTrials = 0;
        bCaptchaProtected = true;
    }

    if (!bCaptchaProtected)
    {
        QWebPage proxyPageWithList;
        QWebSettings* pWebProxyPageSettings = proxyPageWithList.settings();
        pWebProxyPageSettings->setAttribute(QWebSettings::JavascriptEnabled, false);
        proxyPageWithList.mainFrame()->setHtml(sHtml, pReply->url());
        QWebElement webDocument = proxyPageWithList.mainFrame()->documentElement();
        QWebElementCollection documentTables = webDocument.findAll(QString("table"));

        QString sProxyListRawData = "";
        QStringList sProxyListElements;
        QList<QUrl> sPorxiesFound;
        QUrl proxyUrlCandidate;
        QHostAddress urlAddress;

        // For each table
        qDebug() << "AUTO PROXY - CHECK: EXTRACT LIST ELEMENTS";
        foreach (QWebElement webTable, documentTables)
        {
            // Convert to csv plain text
            sProxyListRawData = webTable.toPlainText();
            sProxyListRawData.replace(QRegExp("\t"), ",");
            sProxyListRawData.replace(QRegExp("\n"), ",");

            // Check for valid data separated by tab or space
            sProxyListElements = sProxyListRawData.split(",",QString::SkipEmptyParts);

            int state = 0;
            int iPortNumber = 0;
            bool bOk = false;
            // Identify proxy data: IP + Port + Type
            for (int i = 0; (i < sProxyListElements.count()); i++)
            {
                // check for valid IP. Restart search in case of a match when previous extraction has not been finished
                if (urlAddress.setAddress(sProxyListElements.at(i)))
                {
                    if (state != 0)
                    {
                        sPorxiesFound.append(proxyUrlCandidate);
                    }
                    state = 0;
                }

                switch(state)
                {
                //--------------------------------------------------------------------------
                case 0: // Find IP
                    // Check for IP
                    if (urlAddress.setAddress(sProxyListElements.at(i)))
                    {
                        proxyUrlCandidate.clear();
                        proxyUrlCandidate.setHost(sProxyListElements.at(i));
                        state = 1;
                        // no-break;
                    }
                    else
                        break;
                //--------------------------------------------------------------------------
                case 1: // Find Port
                    // Check if port was defined already
                    if (proxyUrlCandidate.port() > -1)
                    {
                        // Set default proxy type to http
                        proxyUrlCandidate.setUserInfo("http");
                        state = 2;
                        // no-break;
                    }
                    else
                    {
                        // Check for port number
                        iPortNumber = sProxyListElements.at(i).toInt(&bOk);
                        if (bOk)
                        {
                            proxyUrlCandidate.setPort(iPortNumber);
                            // Set default proxy type
                            proxyUrlCandidate.setUserInfo("http");
                            state = 2;
                        }
                        break;
                    }
                //--------------------------------------------------------------------------
                case 2: // Find Type: http or socket
                    if (sProxyListElements.at(i).contains("http",Qt::CaseInsensitive))
                    {
                        proxyUrlCandidate.setUserInfo("http");
                        sPorxiesFound.append(proxyUrlCandidate);
                        state = 0;
                    }
                    else if (sProxyListElements.at(i).contains("sock",Qt::CaseInsensitive))
                    {
                        proxyUrlCandidate.setUserInfo("socks");
                        sPorxiesFound.append(proxyUrlCandidate);
                        state = 0;
                    }
                    break;
                //--------------------------------------------------------------------------
                default:
                    break;
                }
            }
        }

        // Check if any proxies have been determined
        if (sPorxiesFound.isEmpty())
        {
            // Check plain text for proxy server URLs
            int state = 0;
            int iPortNumber = 0;
            bool bOk = false;
            sProxyListRawData = proxyPageWithList.mainFrame()->toPlainText();
            sProxyListRawData.replace(QRegExp("\t"), ",");
            sProxyListRawData.replace(QRegExp("\n"), ",");
            sProxyListRawData.remove(QRegExp(" "));
            sProxyListElements = sProxyListRawData.split(",",QString::SkipEmptyParts);
            // Identify proxy data: IP + Port + Type
            for (int i = 0; (i < sProxyListElements.count()); i++)
            {
                // check for valid IP. Restart search in case of a match when previous extraction has not been finished
                if (urlAddress.setAddress(sProxyListElements.at(i)))
                {
                    if (state != 0)
                    {
                        sPorxiesFound.append(proxyUrlCandidate);
                    }
                    state = 0;
                }

                switch(state)
                {
                //--------------------------------------------------------------------------
                case 0: // Find IP
                    // Check for IP
                    if (urlAddress.setAddress(sProxyListElements.at(i)))
                    {
                        proxyUrlCandidate.clear();
                        proxyUrlCandidate.setHost(sProxyListElements.at(i));
                        state = 1;
                        // no-break;
                    }
                    else
                        break;
                    //--------------------------------------------------------------------------
                case 1: // Find Port
                    // Check if port was defined already
                    if (proxyUrlCandidate.port() > -1)
                    {
                        // Set default proxy type to http
                        proxyUrlCandidate.setUserInfo("http");
                        state = 2;
                        // no-break;
                    }
                    else
                    {
                        // Check for port number
                        iPortNumber = sProxyListElements.at(i).toInt(&bOk);
                        if (bOk)
                        {
                            proxyUrlCandidate.setPort(iPortNumber);
                            // Set default proxy type
                            proxyUrlCandidate.setUserInfo("http");
                            state = 2;
                        }
                        break;
                    }
                    //--------------------------------------------------------------------------
                case 2: // Find Type: http or socket
                    if (sProxyListElements.at(i).contains("http",Qt::CaseInsensitive))
                    {
                        proxyUrlCandidate.setUserInfo("http");
                        sPorxiesFound.append(proxyUrlCandidate);
                        state = 0;
                    }
                    else if (sProxyListElements.at(i).contains("sock",Qt::CaseInsensitive))
                    {
                        proxyUrlCandidate.setUserInfo("socks");
                        sPorxiesFound.append(proxyUrlCandidate);
                        state = 0;
                    }
                    break;
                    //--------------------------------------------------------------------------
                default:
                    break;
                }
            }
        }

        // JSON Api parser -> convert to csv data
        if (sPorxiesFound.isEmpty())
        {
            int state = 0;
            int iPortNumber = 0;
            bool bOk = false;
            sProxyListRawData = proxyPageWithList.mainFrame()->toPlainText();
            sProxyListRawData.remove(QRegExp("\""));
            sProxyListRawData.replace(QRegExp("\n"), ",");
            sProxyListRawData.replace(QRegExp(":"), ",");
            sProxyListRawData.remove(QRegExp(" "));
            sProxyListRawData.remove(QRegExp("{"));
            sProxyListRawData.remove(QRegExp("}"));
            sProxyListElements = sProxyListRawData.split(",",QString::SkipEmptyParts);

            // Identify proxy data: IP + Port + Type
            for (int i = 0; (i < sProxyListElements.count()); i++)
            {
                // check for valid IP. Restart search in case of a match when previous extraction has not been finished
                if (urlAddress.setAddress(sProxyListElements.at(i)))
                {
                    if (state != 0)
                    {
                        sPorxiesFound.append(proxyUrlCandidate);
                    }
                    state = 0;
                }

                switch(state)
                {
                //--------------------------------------------------------------------------
                case 0: // Find IP
                    // Check for IP
                    if (urlAddress.setAddress(sProxyListElements.at(i)))
                    {
                        proxyUrlCandidate.clear();
                        proxyUrlCandidate.setHost(sProxyListElements.at(i));
                        state = 1;
                        // no-break;
                    }
                    else
                        break;
                    //--------------------------------------------------------------------------
                case 1: // Find Port
                    // Check if port was defined already
                    if (proxyUrlCandidate.port() > -1)
                    {
                        // Set default proxy type to http
                        proxyUrlCandidate.setUserInfo("http");
                        state = 2;
                        // no-break;
                    }
                    else
                    {
                        // Check for port number
                        iPortNumber = sProxyListElements.at(i).toInt(&bOk);
                        if (bOk)
                        {
                            proxyUrlCandidate.setPort(iPortNumber);
                            // Set default proxy type
                            proxyUrlCandidate.setUserInfo("http");
                            state = 2;
                        }
                        break;
                    }
                    //--------------------------------------------------------------------------
                case 2: // Find Type: http or socket
                    if (sProxyListElements.at(i).contains("http",Qt::CaseInsensitive))
                    {
                        proxyUrlCandidate.setUserInfo("http");
                        sPorxiesFound.append(proxyUrlCandidate);
                        state = 0;
                    }
                    else if (sProxyListElements.at(i).contains("sock",Qt::CaseInsensitive))
                    {
                        proxyUrlCandidate.setUserInfo("socks");
                        sPorxiesFound.append(proxyUrlCandidate);
                        state = 0;
                    }
                    break;
                    //--------------------------------------------------------------------------
                default:
                    break;
                }
            }
        }


        // Check if any proxies have been determined
        if (!sPorxiesFound.isEmpty())
        {
             emit updateAutoProxyCheckState("<font color='green'>Proxy source successfuly checked.</font>");
             emit updateAutoProxyCheckStateResult(true);
        }
        else
        {
            iTrials++;
            if (iTrials < 3)
                checkProxyServerSource(pReply->url().toString());
            else
            {
                iTrials = 0;
                emit updateAutoProxyCheckState("<font color='red'>Proxy source failed or incompatible.</font>");
                emit updateAutoProxyCheckStateResult(false);
            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Apply proxy server from proxyServerList
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::applyProxyServer(QNetworkProxy* pUserProxy)
{
    int iProxy = 1;
    QDateTime currentDate = QDateTime::currentDateTime();

    mbAutoProxyIsBusy = true;
    mbAbortRequest = false;

    emit updateAutoProxyProgressBar(true);

    // Check if UserProxy data are available
    if (pUserProxy != 0)
    {
       qDebug() << "AUTO PROXY: APPLYING MANUAL PROXY";

       // Apply proxy
       *mpProxy = *pUserProxy;
       QNetworkProxy::setApplicationProxy(*mpProxy);
       msMsgPrefix = "";
       mbUseAutoProxy = false;
    }
    else
    {
        qDebug() << "AUTO PROXY: APPLYING AUTO PROXY";

        mpProxy->setType(QNetworkProxy::NoProxy);
        QNetworkProxy::setApplicationProxy(*mpProxy);
        msMsgPrefix = "Auto";
        mbUseAutoProxy = true;
    }

    // Check abort flag
    if (mbAbortRequest)
    {
        qDebug() << "AUTO PROXY: ABORT - PROXY DISABLED";

        mbAbortRequest      = false;
        mbAutoProxyIsBusy   = false;

        mpProxy->setHostName(PROXY_USER);
        mpProxy->setType(QNetworkProxy::NoProxy);
        QNetworkProxy::setApplicationProxy(*mpProxy);

        emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: Request canceled"));
        emit finished(false);

        return;
    }

    if (mbUseAutoProxy)
    {
        tProxyQueuingList *pProxyQueuingList;
        if (this->mbUsProxyOnly)
            pProxyQueuingList = mpUsProxyQueuingList;
        else
            pProxyQueuingList = mpProxyQueuingList;

        // Check if local host is available for download requests
        if (mbLocalHostEnabled && (pProxyQueuingList->uiHoldTimeList.at(0) < currentDate.toTime_t()))
        {
            qDebug() << "AUTO PROXY: LOCALHOST";

            mpProxy->setType(QNetworkProxy::NoProxy);
            mpProxy->setHostName(PROXY_USER);
            QNetworkProxy::setApplicationProxy(*mpProxy);

            // Default delay to avoid infinite error loop in case of failure
             pProxyQueuingList->uiHoldTimeList.replace(0, currentDate.toTime_t() + 60);
            mbAutoProxyIsBusy = false;

            emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: Using host IP address"));
            emit finished(true);

            return;
        }
        // Check if previously used proxies are still blocked
        else if( pProxyQueuingList->sHostList.count() > 1)
        {
            for(iProxy = 1; iProxy < pProxyQueuingList->sHostList.count(); iProxy++)
                if (pProxyQueuingList->uiHoldTimeList.at(iProxy) < currentDate.toTime_t())
                {
                   // qDebug() << QString::number(iProxy) << " : [time] < [until] : " << QString::number(pProxyQueuingList->uiHoldTimeList.at(iProxy)) << " < "  << QString::number(currentDate.toTime_t());
                    break; // stop loop - proxy is avaiable
                }
        }
        // Check for previous proxy
        if (iProxy < pProxyQueuingList->sHostList.count())
        {
             qDebug() << "AUTO PROXY: FROM QUEUEING LIST";

            mpProxy->setHostName(pProxyQueuingList->proxyList.at(iProxy).host());
            mpProxy->setPort(pProxyQueuingList->proxyList.at(iProxy).port());
            mpProxy->setUser(PROXY_USER);          // MUST NOT BE EMPTY TO AVOID ERROR: ASSERT failure in QNetworkAccessManager: "Internal inconsistency: found a cache key for a proxy, but it's empty", file access\qnetworkaccessauthenticationmanager.cpp, line 217
            mpProxy->setPassword(PROXY_PASS);      // MUST NOT BE EMPTY TO AVOID ERROR: ASSERT failure in QNetworkAccessManager: "Internal inconsistency: found a cache key for a proxy, but it's empty", file access\qnetworkaccessauthenticationmanager.cpp, line 217
            //mpProxy->setType(mpProxyQueuingList->proxyList.at(iProxy).type());

            if (pProxyQueuingList->proxyList.at(iProxy).userInfo() == "http")
            {
                mpProxy->setType(QNetworkProxy::HttpProxy);
            }
            else
            {
                mpProxy->setType(QNetworkProxy::Socks5Proxy);          
            }

            qDebug() << "AUTO PROXY: TYPE: " << pProxyQueuingList->proxyList.at(iProxy).userInfo();
            qDebug() << "AUTO PROXY: HOST: " << mpProxy->hostName();
            qDebug() << "AUTO PROXY: PORT: " << mpProxy->port();
            qDebug() << "AUTO PROXY: USERNAME: " << mpProxy->user();
            //qDebug() << "AUTO PROXY: PASSWORD: " << mpProxy->password();

            // Apply proxy
            QNetworkProxy::setApplicationProxy(*mpProxy);

            // Default delay of 5 minutes to avoid infinite error loop in case of failure
            pProxyQueuingList->uiHoldTimeList.replace(iProxy, currentDate.toTime_t() + 300);

        }
        // Check for new proxy from list
        else if (!mUrlProxyList.isEmpty())
        {
            qDebug() << "AUTO PROXY: FROM LIST. REMAINING : "  << QString::number(mUrlProxyList.count());

            // Auto proxy
            mpProxy->setHostName(mUrlProxyList.at(0).host());
            mpProxy->setPort(mUrlProxyList.at(0).port());
            mpProxy->setUser(PROXY_USER);      // MUST NOT BE EMPTY TO AVOID ERROR: ASSERT failure in QNetworkAccessManager: "Internal inconsistency: found a cache key for a proxy, but it's empty", file access\qnetworkaccessauthenticationmanager.cpp, line 217
            mpProxy->setPassword(PROXY_PASS);  // MUST NOT BE EMPTY TO AVOID ERROR: ASSERT failure in QNetworkAccessManager: "Internal inconsistency: found a cache key for a proxy, but it's empty", file access\qnetworkaccessauthenticationmanager.cpp, line 217
            if (mUrlProxyList.at(0).userInfo() == "http")
            {
                mpProxy->setType(QNetworkProxy::HttpProxy);
            }
            else
            {
                mpProxy->setType(QNetworkProxy::Socks5Proxy);
            }

            qDebug() << "AUTO PROXY: TYPE: " << mUrlProxyList.at(0).userInfo();
            qDebug() << "AUTO PROXY: HOST: " << mpProxy->hostName();
            qDebug() << "AUTO PROXY: PORT: " << mpProxy->port();
            qDebug() << "AUTO PROXY: USERNAME: " << mpProxy->user();
            //qDebug() << "AUTO PROXY: PASSWORD: " << mpProxy->password();

            // Apply proxy
            QNetworkProxy::setApplicationProxy(*mpProxy);
        }
        // Request new proxy list
        else
        {
            qDebug() << "AUTO PROXY: REQUESTING PROXY LIST";

            // All proxies have been used, start new proxy list request
            getProxyServerList();

            return;
        }
    }

    // Check  if  proxy settings have been applied
    if(*mpProxy == QNetworkProxy::applicationProxy())
    {
        if (mpProxy->type() == QNetworkProxy::NoProxy)
        {
            qDebug() << "AUTO PROXY: NOPROXY";
            emit updateAutoProxyState(QString("Proxy disabled!"));
        }
        else
        {
            emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: Verifying connection to <font color='blue'>%1</font>... (trials: %2)").arg(mpProxy->hostName()).arg(miTrials++));

            // Check IP online
            connect(mpNetwProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_verifyProxyServerConnection(QNetworkReply*)));
            //Verifiy connection
            QUrl verifyUrl("https://www.hathitrust.org/");//https://www.google.com/");//https://www.hathitrust.org/");
            QNetworkRequest requestVerifyUrl(verifyUrl);
            requestVerifyUrl.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");

            qDebug() << "AUTO PROXY: TARGET URL: " << verifyUrl.toString();

            mpProxyReply = mpNetwProxyManager->get(requestVerifyUrl);
            mpRetryTimer->start(AUTOPROXY_TIMEOUT);
        }
    }
    else
    {
        qDebug() << "PROXY: SETTING UP PROXY FAILED";
        emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: Proxy failed ..."));
        mbAutoProxyIsBusy = false;

        if (mbUseAutoProxy)
            mpRetryTimer->start(1000);
        else
            emit finished(false);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Verify proxy server connection
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_verifyProxyServerConnection(QNetworkReply* pReply)
{
    qDebug() << "PROXY: VERIFICATION";

    mpRetryTimer->stop();
    disconnect(mpNetwProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_verifyProxyServerConnection(QNetworkReply*)));

    if (mbAbortRequest)
    {
        mbAbortRequest      = false;
        mbAutoProxyIsBusy   = false;

        emit finished(false);

        return;
    }

    // Check received download reply
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (        pReply->error()                                                                 // error
                || !redirectionTarget.isNull()                                                  // redirection
                ||  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty())    // empty page
    {
        qDebug() << "PROXY: VERIFICATION ERROR";
        qDebug() << " pReply->url) " <<  pReply->url() ;
        qDebug() << " pReply->error() " <<  pReply->error() ;
        qDebug() << " redirectionTarget    " <<  redirectionTarget.toString();
        qDebug() << " pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty()) " <<  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty();

        // Check for QNetworkReply::UnknownNetworkError 99
        if (pReply->error() == QNetworkReply::UnknownNetworkError)
            miUnknownNetworkErrors++;

        if (!mUrlProxyList.isEmpty())
            mUrlProxyList.removeAt(0);

        if (mbUseAutoProxy)
        {
            emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: Proxy verification failed ..."));

            if (miUnknownNetworkErrors >= MAX_NETWORKERRORS)
            {
                // Disable autoproxy
                slot_cancelAutoProxyRequest();
                mbAutoProxyIsBusy = true;
                // Start auto-restart timer
                 mpRestartAutoProxyTimer->start(AUTOPROXY_TIMEOUT_1S);
            }
            else
                applyProxyServer();
        }
        else
        {
            emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: Proxy verification failed... check proxy setup"));
            emit finished(false);
            emit updateAutoProxyProgressBar(false);
        }

        return;
    }

    QWebPage pageIpVerification;
    pageIpVerification.mainFrame()->setHtml(pReply->readAll());

    if (mbCheckProxyOnTargetUrl && !msTargetUrl.isEmpty() && msTargetUrl.contains("hathitrust"))
    {
        emit updateAutoProxyState(msMsgPrefix + "Proxy: Verifying connection to <font color='blue'>hathitrust.org</font> target URL");
        qDebug() << "PROXY: VERIFYIMG CONNECTION TO HATHITRUST.ORG TARGET URL: " << msTargetUrl;
        connect(mpNetwProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_checkProxyOnTargetUrl(QNetworkReply*)));

        // Verifiy connection to hathitrust.org
        QUrl targetUrl(msTargetUrl);
        QNetworkRequest requestTargetUrl(targetUrl);
        requestTargetUrl.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");
        if (mpProxyReply != NULL)
            mpProxyReply->deleteLater();
        mpProxyReply = mpNetwProxyManager->get(requestTargetUrl);
        mpRetryTimer->start(AUTOPROXY_TIMEOUT);
    }
    else
    {
        qDebug() << "PROXY: CONNECTION  ESTABLISHED";
        miUnknownNetworkErrors = 0;
        miTrials  = 0;
        mbAutoProxyIsBusy = false;

        emit updateAutoProxyProgressBar(false);
        emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: <font color='green'>Connection established.</font> Proxy IP: <font color='blue'> " + mpProxy->hostName() + "</font>"));
        emit finished(true);

        if (!mUrlProxyList.isEmpty())
            mUrlProxyList.removeAt(0);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Verify proxy server connection
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_checkProxyOnTargetUrl(QNetworkReply* pReply)
{
    qDebug() << "PROXY: CHECKING RESTRICTIONS ON TARGET URL";

    mpRetryTimer->stop();
    disconnect(mpNetwProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_checkProxyOnTargetUrl(QNetworkReply*)));

    if (mbAbortRequest)
    {
        mbAbortRequest      = false;
        mbAutoProxyIsBusy   = false;

        emit finished(false);

        return;
    }

    // Check received download reply
    QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (        pReply->error()                                                                 // error
                || !redirectionTarget.isNull()                                                  // redirection
                ||  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty())    // empty page
    {

        qDebug() << "PROXY: CHECK ON TARGET URL FAILED";

//        qDebug() << " pReply->url) " <<  pReply->url() ;
//        qDebug() << " pReply->error() " <<  pReply->error() ;
//        qDebug() << " redirectionTarget    " <<  redirectionTarget.toString();
//        qDebug() << " pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty()) " <<  pReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty() ;

        emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: Proxy verification failed ..."));

        if (!mUrlProxyList.isEmpty())
            mUrlProxyList.removeAt(0);

        if (mbUseAutoProxy)
            applyProxyServer();
        else
        {
            emit finished(false);
            emit updateAutoProxyProgressBar(false);
        }

        return;
    }

    QWebPage pageCheckProxyfiedPage;
    pageCheckProxyfiedPage.mainFrame()->setHtml(pReply->readAll());
    QWebElement document = pageCheckProxyfiedPage.mainFrame()->documentElement();
    if (!checkBookRestriction(&document))
    {
         qDebug() << "PROXY: BLOCKED BY RESTRICTIONS";
        emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: Proxy verification failed ..."));

        if (!mUrlProxyList.isEmpty())
            mUrlProxyList.removeAt(0);

        if(mbLocalHostEnabled && QNetworkProxy::applicationProxy().hostName().isEmpty())
            mbLocalHostEnabled = false;

        if (mbUseAutoProxy)
            applyProxyServer();
        else
        {
            emit updateAutoProxyProgressBar(false);
            emit finished(false);
        }
    }
    else
    {
         qDebug() << "PROXY: NO  RESTRICTIONS";
        miTrials  = 0;
        mbAutoProxyIsBusy = false;


        emit updateAutoProxyProgressBar(false);
        emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: <font color='green'>Connection established.</font> Proxy IP: <font color='blue'> " + mpProxy->hostName() + "</font>"));
        emit finished(true);

        if (!mUrlProxyList.isEmpty())
            mUrlProxyList.removeAt(0);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * restart auto Proxy when stucked on unknnown network error: code 99
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_restartAutoProxyRequest(void)
{
    static int iTicks = AUTOPROXY_RESTART;

    if ((iTicks == 0) && !mbAbortRequest)
    {
        qDebug() << "AUTO PROXY: RESTART";

        iTicks = AUTOPROXY_RESTART;
        miUnknownNetworkErrors = 0;
        // Restart auto proxy
        applyProxyServer();
    }
    else if (!mbAbortRequest)
    {
        qDebug() << "AUTO PROXY: WAIT FOR RESTART " << iTicks;

        // Wait some time
        emit updateAutoProxyState(QString("Auto Proxy: resolving internal network error ...please wait [00:%1]").arg(iTicks,2,10, QChar('0')));
        iTicks--;
        mpRestartAutoProxyTimer->start(AUTOPROXY_TIMEOUT_1S);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Cancel load html page action
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_cancelAutoProxyRequest(void)
{
    qDebug() << "PROXY: REQUEST CANCELED";

    // Abort by user
    if (mbAbortRequest)
    {
        mpRetryTimer->stop();
        emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: Http request canceled ..."));
    }
    else if (!mpRetryTimer->isActive())
    {
        mpRetryTimer->stop();
        if (mbUseAutoProxy)
            emit updateAutoProxyState(QString(msMsgPrefix + "Proxy: timeout - proxy request failed ..."));
        else
            emit updateAutoProxyState(QString("timeout: proxy request failed ...check proxy setup"));
    }
    else
    {
        mpRetryTimer->stop();
        emit updateAutoProxyState(QString("AutoProxy: error: proxy request failed ..."));
        mbLocalHostEnabled = true;
    }

    disconnect(mpNetwProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_verifyProxyServerConnection(QNetworkReply*)));
    disconnect(mpNetwProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_extractProxyList(QNetworkReply*)));
    if (mpProxyReply!= NULL)
        disconnect(mpProxyReply, SIGNAL(finished()), this, SLOT(slot_extractProxyList()));
    if (mpProxySrcChkReply!=NULL)
        disconnect(mpProxySrcChkReply, SIGNAL(finished()), this, SLOT(slot_checkProxySourceReply()));
    disconnect(mpNetwProxyManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slot_checkProxyOnTargetUrl(QNetworkReply*)));

    if( (mpProxyReply != NULL) && (mpProxyReply->isOpen()) )
    {
        mpProxyReply->abort();
    }

    if( (mpProxySrcChkReply != NULL) && (mpProxySrcChkReply->isOpen()) )
    {
        mpProxySrcChkReply->abort();
        if (!mbAbortRequest)
            emit updateAutoProxyCheckStateResult(false);
    }

    if (!mUrlProxyList.isEmpty())
        mUrlProxyList.removeAt(0);

    if (mbUseAutoProxy && !mbAbortRequest)
    {
        if (miUnknownNetworkErrors < MAX_NETWORKERRORS)
            // Restart request
            applyProxyServer();
    }
    else
    {
        qDebug() << "PROXY: DISABLED";
        emit updateAutoProxyProgressBar(false);
        mbAutoProxyIsBusy = false;
        enableLocalHost(true);
        if (!mbAbortRequest)
        {
            qDebug() << "PROXY: FAILED";
            emit finished(false);

        }
    }
    mbAutoProxyIsBusy = false;
    miNumDownloads = 0;
}

/*-----------------------------------------------------------------------------------------------*
 * Return auto proxy enable state
 *-----------------------------------------------------------------------------------------------*/
bool AutoProxy::getAutoProxyState(void)
{
    return mbUseAutoProxy;
}

/*-----------------------------------------------------------------------------------------------*
 * Set auto proxy enable state
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::setAutoProxyState(bool bState)
{
    qDebug() << "AUTO PROXY: ENABLE AUTOPROXY " << bState;
    mbUseAutoProxy = bState;
}

/*-----------------------------------------------------------------------------------------------*
 * Get US proxies only flag
 *-----------------------------------------------------------------------------------------------*/
bool AutoProxy::getUsProxyOnlyState(void)
{
    return mbUsProxyOnly;
}

/*-----------------------------------------------------------------------------------------------*
 * Get busy flag
 *-----------------------------------------------------------------------------------------------*/
bool AutoProxy::getProxyBusyFlagState(void)
{
    return mbAutoProxyIsBusy;
}

/*-----------------------------------------------------------------------------------------------*
 * Set US proxies only flag
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::setUsProxyOnlyState(bool bState)
{
    mbUsProxyOnly = bState;
    qDebug() << "AUTO PROXY: setting US proxy only state " << mbUsProxyOnly;
}

/*-----------------------------------------------------------------------------------------------*
 * Clear failue counter
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::clearFailures(void)
{
    qDebug() << "PROXY: CLEAR FAILURES";
    miTrials = 0;
}

/*-----------------------------------------------------------------------------------------------*
 * Clear proxy list
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::clearProxyList(void)
{
    mUrlProxyList.clear();
    mbLocalHostEnabled = true;
    miNumDownloads = 0;
}

/*-----------------------------------------------------------------------------------------------*
 * Abort autoproxy requests
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::abortAutoProxyRequest(void)
{
    mbAbortRequest = true;
    miUnknownNetworkErrors = 0;
    slot_cancelAutoProxyRequest();
}

/*-----------------------------------------------------------------------------------------------*
 * Set hahtitrust target URL to check if book is available
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::setTargetUrl(QString sTargetUrl)
{
    slot_setTargetUrl(sTargetUrl);
}

/*-----------------------------------------------------------------------------------------------*
 * Set hahtitrust target URL to check if book is available
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_setTargetUrl(QString sTargetUrl)
{
    msTargetUrl = sTargetUrl;
}

/*-----------------------------------------------------------------------------------------------*
 * Check if document is accessible / country restriction due to copyright
 *-----------------------------------------------------------------------------------------------*/
bool AutoProxy::checkBookRestriction(QWebElement *pDocument)
{
    // Extract "div" tags
    QWebElementCollection documentSpans = pDocument->findAll(QString("div"));
    QString sTemp;
    bool bAvailable =  true;

    if (mbAbortRequest)
    {
        mbAbortRequest      = false;
        mbAutoProxyIsBusy   = false;
        emit finished(false);

        return false;
    }

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
 * Update proxy queing list + queuing times
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::updateAutoProxyQueue(uint iUntilTime)
{
//    qDebug() << "HOST: " << QNetworkProxy::applicationProxy().hostName().toLatin1();
//    qDebug() << "port: " << QString::number(QNetworkProxy::applicationProxy().port());
//    qDebug() << "Type: "  << QString::number(QNetworkProxy::applicationProxy().type());
//    qDebug() << "iUntilTime: " << QString::number(iUntilTime);

    tProxyQueuingList *pProxyQueuingList;
    if (this->mbUsProxyOnly)
        pProxyQueuingList = mpUsProxyQueuingList;
    else
        pProxyQueuingList = mpProxyQueuingList;

    QDateTime currentDate = QDateTime::currentDateTime();

    if (iUntilTime == 0)
        iUntilTime = currentDate.toTime_t();
    else if (iUntilTime < (currentDate.toTime_t() + 30) )
        iUntilTime = (currentDate.toTime_t() + 30);
    else if (iUntilTime > (currentDate.toTime_t() + 300) )
        iUntilTime = (currentDate.toTime_t() + 300);

    // LOCALHOST?
    QString sCurProxy = QNetworkProxy::applicationProxy().hostName();

    if(sCurProxy.isEmpty())
    {
        qDebug() << "AUTO PROXY: UPDATING LOCALHOST";
        pProxyQueuingList->uiHoldTimeList.replace(0, iUntilTime);
        if (miNumDownloads > 0)
            pProxyQueuingList->uiNumDownloads.replace(0, miNumDownloads);
    }
    else
    {
        // Checkif proxy already exist
        int i = pProxyQueuingList->sHostList.indexOf(sCurProxy);

        if (i < 0)
        {
            if (miNumDownloads > 0)
            {
                qDebug() << "AUTO PROXY: ADDING PROXY TO QUEUING LIST";
                // Add newProxy
                pProxyQueuingList->sHostList.append(sCurProxy);
                pProxyQueuingList->uiHoldTimeList.append(iUntilTime);
                pProxyQueuingList->uiNumDownloads.append(miNumDownloads);
                QUrl proxyUrl;
                proxyUrl.setHost(QNetworkProxy::applicationProxy().hostName().toLatin1());
                proxyUrl.setPort(QNetworkProxy::applicationProxy().port());
                if (QNetworkProxy::applicationProxy().type() == QNetworkProxy::Socks5Proxy)
                    proxyUrl.setUserInfo("socks");
                else
                    proxyUrl.setUserInfo("http");
                pProxyQueuingList->proxyList.append(proxyUrl);
            }
        }
        else
        {
            qDebug() << "AUTO PROXY: UPDATING QUEUING LIST";

            // Update hold time of current WebProxy
            pProxyQueuingList->uiHoldTimeList.replace(i, iUntilTime);
            if (miNumDownloads > 0)
                pProxyQueuingList->uiNumDownloads.replace(i, miNumDownloads);
        }

        // DEBUG OUTPUT - list all used autoproxies
        qDebug() << "========================================================================================";
        qDebug() << "   number   _|_   autoproxy URL   _|_   queuing time   _|_    pages downloaded";
        qDebug() << "========================================================================================";
        for (int j = 0; j < pProxyQueuingList->proxyList.length(); j++ )
        {
            qDebug() << QString::number(j)
                     <<  " __|__ " << pProxyQueuingList->proxyList.at(j).toString()
                     <<  " __|__ "  <<  QString::number( pProxyQueuingList->uiHoldTimeList.at(j) - currentDate.toTime_t())
                     << " (" << QString::number(pProxyQueuingList->uiHoldTimeList.at(j))
                     << ")  __|__   " << pProxyQueuingList->uiNumDownloads.at(j);
            qDebug() << "---------------------------------------------------------------------------------------------------";
        }
        qDebug() << "========================================================================================";

    }

    miNumDownloads       = 0;
}

/*-----------------------------------------------------------------------------------------------*
 * Enable local host
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::enableLocalHost(bool bEnableLocalHost)
{
    qDebug() << "AUTO PROXY: ENABLE LOCALHOST " << bEnableLocalHost;
    mbLocalHostEnabled = bEnableLocalHost;
}

/*-----------------------------------------------------------------------------------------------*
 * Increment download counter
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::slot_updateDownloadCounter(QNetworkReply* pReply)
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
 * Get proxy list of online sources
 *-----------------------------------------------------------------------------------------------*/
QStringList AutoProxy::getProxySourceList(void)
{
    return msProxySourcesUrls;
}

/*-----------------------------------------------------------------------------------------------*
 * Set proxy list of online sources
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::setProxySourceList(QStringList sProxySourceList)
{    
    // check for empty list
    if (sProxySourceList.count() > 0)
        msProxySourcesUrls = sProxySourceList;
    else
        initDefaultProxySources(AUTOPROXY_RESET_OTHER_PROXYLIST); // reset all country proxy list

    generateAutoProxySourceFile();
}

/*-----------------------------------------------------------------------------------------------*
 * Get US only proxy list of online sources
 *-----------------------------------------------------------------------------------------------*/
QStringList AutoProxy::getUsProxySourceList(void)
{
    return msProxySourcesUrlsUsOnly;
}

/*-----------------------------------------------------------------------------------------------*
 * Get US only proxy list of online sources
 *-----------------------------------------------------------------------------------------------*/
void AutoProxy::setUsProxySourceList(QStringList sUsProxySourceList)
{
    // check for empty list
    if (sUsProxySourceList.count() > 0)
        msProxySourcesUrlsUsOnly = sUsProxySourceList;
    else
        initDefaultProxySources(AUTOPROXY_RESET_US_PROXYLIST); // reset US proxy list

    generateAutoProxySourceFile();
}
