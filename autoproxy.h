#ifndef AUTOPROXY_H
#define AUTOPROXY_H

#include <QNetworkReply>
#include <QNetworkProxy>
#include <QSslError>
#include <QNetworkAccessManager>
#include <QWebPage>
#include <QTimer>
#include <QWidget>
#include <QWebElement>
#include <QList>
#include <QStringList>

#include <QAuthenticator>

#define ENABLE_HTTPS_PROXY  1   ///> enable https proxy sources
#define ENABLE_SOCKS_PROXY  0   ///> enable sockets proxy sources

typedef struct {
    QStringList            sHostList;
    QList<QUrl>            proxyList;
    QVector<uint>          uiHoldTimeList;
    QVector<uint>          uiNumDownloads;
}tProxyQueuingList;

/*-----------------------------------------------------------------------------------------------*
 * AutoProxy class
 *-----------------------------------------------------------------------------------------------*/
class AutoProxy: public QObject
{
    Q_OBJECT

public:
    AutoProxy(QWidget *parent);
    void initDefaultProxySources(int iList = 0);
    void generateAutoProxySourceFile(void);
    void getProxyServerList(void);
    void checkProxyServerSource(QString sUrl);
    void applyProxyServer(QNetworkProxy *pUserProxy = 0);
    bool getAutoProxyState(void);
    bool getProxyBusyFlagState(void);
    void setAutoProxyState(bool bState);
    void setUsProxyOnlyState(bool bState);
    bool getUsProxyOnlyState(void);
    void clearProxyList(void);
    void clearFailures(void);
    void abortAutoProxyRequest(void);
    void updateAutoProxyQueue(uint iUntilTime);
    void enableLocalHost(bool bEnableLocalHost);
    QStringList getProxySourceList(void);
    void setProxySourceList(QStringList sProxySourceList);
    QStringList getUsProxySourceList(void);
    void setUsProxySourceList(QStringList sUsProxySourceList);
    void setTargetUrl(QString sTargetUrl);
    bool mbAutoProxyIsBusy;

private slots:
    void slot_extractProxyList(QNetworkReply* pReply);
    void slot_extractProxyList(void);
    void slot_checkProxySourceReply(QNetworkReply* pReply);
    void slot_checkProxySourceReply(void);
    void slot_verifyProxyServerConnection(QNetworkReply* pReply);
    void slot_restartAutoProxyRequest(void);
    void slot_cancelAutoProxyRequest(void);
    void slot_checkProxyOnTargetUrl(QNetworkReply* pReply);
    void slot_setTargetUrl(QString sTargetUrl);
    void slot_updateDownloadCounter(QNetworkReply* pReply);
    void slot_generateAutoProxySourceFile(void);
    void slot_loadAutoProxySourceFile(void);
    void slot_handleSslErrors(QNetworkReply* pReply, const QList<QSslError> &errors);
    void slot_onProxyAuthenticationRequired(const QNetworkProxy &p, QAuthenticator *auth);
    void slot_onNetworkSessionConnected(void);

signals:
    void updateAutoProxyState(QString);
    void updateAutoProxyProgressBar(bool);
    void finished(bool);
    void updateAutoProxyCheckState(QString);
    void updateAutoProxyCheckStateResult(bool);

private:
    void initAutoProxy(void);
    void readAutoProxySourceFile(QString* psProxySources);
    bool checkBookRestriction(QWebElement *pDocument);

    QWidget                 *mpParent;
    QNetworkProxy           *mpProxy;
    QNetworkAccessManager   *mpNetwProxyManager;
    QNetworkReply           *mpProxyReply;
    QNetworkReply           *mpProxySrcChkReply;
    QWebPage                *mpProxySrcPage;
    QTimer                  *mpRetryTimer;
    QTimer                  *mpRestartAutoProxyTimer;
    QStringList             msProxySourcesUrls;
    QStringList             msProxySourcesUrlsUsOnly;
    QStringList             msProxySourcesUrlsBlacklist;
    QList<int>              miProxySourcesScore;
    QList<QUrl>             mUrlProxyList;
    QList<QUrl>             mCaptchaProtectedUrl;
    QString                 msTargetUrl;
    bool                    mbUseAutoProxy;
    bool                    mbAbortRequest;
    bool                    mbUsProxyOnly;
    bool                    mbCheckProxyOnTargetUrl;
    bool                    mbLocalHostEnabled;
    int                     miTrials ;
    int                     miNumDownloads;
    int                     miUnknownNetworkErrors;
    tProxyQueuingList       *mpProxyQueuingList;
    tProxyQueuingList       *mpUsProxyQueuingList;
    QString                 msMsgPrefix;
};

#endif // AUTOPROXY_H
