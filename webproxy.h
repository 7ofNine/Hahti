#ifndef WEBPROXY_H
#define WEBPROXY_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebPage>
#include <QWebFrame>
#include <QWebElement>
#include <QTimer>
#include <QDateTime>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QWebView>
#include <QDebug>


typedef struct {
    QStringList     sHostList;
    QVector<uint>   uiHoldTimeList;
    QVector<int>    iQuality;
}tWebProxyQueuingList;


#define USE_POST_REDIRECT 1

/*-----------------------------------------------------------------------------------------------*
 * Derived QWebPage class - realizes pop-up blocker
 *-----------------------------------------------------------------------------------------------*/
class WebProxyWebPage: public QWebPage{

    Q_OBJECT
public:
    explicit WebProxyWebPage(QObject *parent = 0);
    ~WebProxyWebPage();

private:
   QWebPage* createWindow(QWebPage::WebWindowType type);
};

/*-----------------------------------------------------------------------------------------------*
 * Derived QNetworkAccessManager class - realizes post request grabber
 *-----------------------------------------------------------------------------------------------*/
class HdhNetworkAccessManager: public QNetworkAccessManager{

    Q_OBJECT

protected:

    virtual QNetworkReply* createRequest ( Operation op,
                                            const QNetworkRequest &req,
                                            QIODevice *outgoingData = 0 ) {
//       // qDebug() << "Trying to read POST data...";
//        if(outgoingData && (op == PostOperation)){
//         if(outgoingData){
//            QByteArray outgoingByteArray = outgoingData -> readAll();
//            QString postData ( outgoingByteArray );
//            qDebug() << "POST data " << postData;
//         }
//         qDebug() << "GET REQ: " << req.url().toString();
////            QByteArray outgoingByteArray2 = outgoingData -> readAll();
////            QString postData2 ( outgoingByteArray2 );
////            qDebug() << "POST data2 " << postData2;
//        }

        return QNetworkAccessManager::createRequest ( op, req, outgoingData);
    }
};


/*-----------------------------------------------------------------------------------------------*
 * WebProxy class
 *-----------------------------------------------------------------------------------------------*/
class WebProxy: public QObject
{
    Q_OBJECT

public:
    WebProxy(QNetworkAccessManager* pDownloadManager=0);
    ~WebProxy(void);

    void initWebProxy(void);

    void getNewWebProxy(QString);
    void setNumParallelDownloads(int iNumParallelDownloads);
    void updateWebProxyQueue(uint iUntilTime);
    void abortWebProxyLoadPage(void);
   // void enableWebProxy(bool);
    void setWebProxyEnableState(bool);
    bool getWebProxyEnableState(void);
    QString getWebProxyPrefix();
    void setWebProxyPrefix(QString);
    QString getProxifiedUrl(QString sTargetUrl);
    QStringList getLogOfUsedWebProxies(void);
    void cancelRequest(void);
    void reset(void);
    void resetWebProxy(void);
    bool mbWebProxyIsEnabled;
    bool mbWebProxyRequestActivated;

signals:
   void updateWebProxyState(QString);

private slots:   
    void slot_abortWebProxyLoadPage(void);
    void slot_proxyFormPageLoaded(bool);
    void slot_followRedirectionToWebpageForm(QNetworkReply* pReply);
    void slot_proxyTargetPageLoaded(QNetworkReply* pReply);
    void slot_updateDownloadCounter(QNetworkReply* pReply);
    void slot_handleSslErrors(QNetworkReply* pReply, const QList<QSslError> &errors);

private:
    bool checkBookRestriction(QWebElement *pDocument);
    void extractWebProxyPrefix(void);
    void updateWebProxyList(void);
    void generateDefaultProxyList(void);
    void generateProxyFile(void);
    void readProxyCollectionFile(QString* psProxyCollection);
    void updateWebProxyFile(void);

   // HdhNetworkAccessManager *mpProxyManager;
   HdhNetworkAccessManager  *mpProxyManager;
    QNetworkAccessManager   *mpDownloadManager;
    QNetworkRequest         *mpRequest;
    QNetworkReply           *mpReply;
    WebProxyWebPage         *mpWebProxyPage;
    QTimer                  *mpWebProxyPageTimer;
    tWebProxyQueuingList    *mpWebProxyQueuingList;
    QString                 msWebproxyPrefix;
    QString                 msTargetUrl;
    QString                 msCurProxyUrl;
    QStringList             msUrlRedirects;
    QStringList             msWebProxyList;
    QStringList             msRandomProxyList;
    QStringList             msBlockedProxyList;
    QVector<int>            miWebProxyScore;
    int                     miNumDownloads;
    QString                 msPrevWebProxyPrefix;
    uint                    muiWebproxyTimestamp;
    int                     miNumParallelDownloads;
    bool                    mbWebProxyCanceled;

};

#endif // WEBPROXY_H
