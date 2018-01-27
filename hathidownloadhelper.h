#ifndef HATHIDOWNLOADHELPER_H
#define HATHIDOWNLOADHELPER_H

#include "mainmenu.h"
#include "proxydialog.h"
#include "fileselectdialog.h"
#include "createbatchjob.h"
#include "filedownloader.h"
#include "common.h"
#include "autoproxy.h"
#include "customheaderedtable.h"

#include <QWidget>
#include <QTabWidget>
#include <QMainWindow>
#include <QWebPage>
#include <QWebFrame>
#include <QWebElement>
#include <QClipboard>
#include <QMovie>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QTimer>
#include <QTime>
#include <QPrinter>
#include <QMenuBar>
#include <QPageSetupDialog>
#include <QTextDocument>
#include <QList>
#include <QDir>
#include <QThread>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QScrollArea>
#include <QtSql/QtSql>

#define ENABLE_PROXY_CHANGEOVER 1   ///< enable changeover between autoproxy and webproxy feature during download
                                    ///< TODO:  There is a bug. When doing changeover "webproxy -> autoproxy -> webproxy"
                                    ///< qnetworkaccessmanager is producing networkerror 99: unknownerror
#define MAX_LENGTH_PATH (255-6-1-40)  // 255 - length('/pdfs/') - etc

/*-----------------------------------------------------------------------------------------------*
 * Main class
 *-----------------------------------------------------------------------------------------------*/
class HathiDownloadHelper : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit HathiDownloadHelper(QWidget *parent = 0);
    ~HathiDownloadHelper();

signals:
    void abortThread(bool);
    void bookInfoLoaded(bool);
    void bookInfoCopyright(QString);
    void cancelBatchJob(void);
    void clipboardChanged(QString);
    void downloadFinished(bool);
    void pdfMergeFinished(bool);
    void proxyConnectionEstablished(bool);
    void updateProgressbar(int);
    void enableOpenPathButton(bool);
    void updateHistoryTableContentPath(QString);

private slots:
    // Gui related slots
    void slot_adjustGuiGrid();
    void slot_copyFromClipboard(void);
    void slot_createMessageBox(QString);
    void on_pushButtonExit_clicked(void);
    void slot_saveGuiSettings(void);
    void slot_infiniteProgressBarsBookPage(bool);
    void slot_sortTableWidgetHistory(int iCol);
    void slot_loadTableWidgetHistory(void);
    void slot_updateTableWidgetHistory(int iRow, int iColumn);
    void slot_updateSelectionTableWidgetHistory(void);
    void slot_updateTableWidgetHistoryPath(QString sPath);
    void on_lineEditFilterHistory_changed(void);

    // Menu / sub menu slots
    void slot_pageSetupDialog(void);
    void slot_help();
    void slot_about(void);
    void slot_showWhatsNew(void);
    void slot_styleSetupDialog(void);
    void slot_fontSetupDialog(void);
    void slot_resetGuiSetup(void);
    void slot_exportLinksDialog(void);
    void slot_activateBatchJob(bool bState);
    void slot_selectPdfFilesToBeMergerd(void);
    void slot_selectBatchJobTab();
    void slot_createLinksFile(bool);
    void slot_sendFeeback(void);
    void slot_sendFeebackFinished();
    void slot_checkUpdate(void);
    void slot_loadVersionFile(void);
    void slot_autoUpdateRequest(void);
    void slot_autoUpdateResponse(void);
    void slot_pushButtonCancelUpdate_clicked();
    void slot_userSettingsDialog(void);
    void slot_defaultSettings(void);

    void slot_addLinkToCollectorList(QString sUrl);
    void slot_removeLinkFromCollectorList(QString sUrl);
    void on_pushButtonRemoveLink_clicked(void);
    void on_pushButtonOpenPath_clicked(void);
    void on_pushButtonRemoveAll_clicked(void);
    void on_pushButtonRemoveItem_clicked(void);
    void on_pushButtonRemoveAllItems_clicked(void);
    void slot_startLinkCollectorDownload(void);
    void slot_addLinkCollectorListToBatchJob(void);
    void slot_updateBatchJobTabTitle(int iJobs);

    // Gui related slots - book info group box
    void on_pushButtonGetInfo_clicked(void);
    void slot_pushButtonGetInfoCancel_clicked(void);
    void slot_setBookUrl(QString);
    void on_lineEditBookURL_editingFinished(void);
    void slot_disableWebProxy(bool bIsChecked);
    void slot_disableAutoProxy(bool bIsChecked);
    void slot_enableAutoProxy(bool bIsChecked);
    void slot_enableWebProxy(bool);
    void slot_checkBoxResume(bool bState);
    void on_checkBoxUseProxy_clicked(bool);
    void slot_setProxyState(bool bState);

    // Gui related slots - download settings group box
    void slot_enableDownloadSettings(bool);
    void slot_applyDownloadOptions(QString);
    void slot_updateDownloadProgressBar(int iPage, int iMaxPage);
    void slot_updateDownloadState(QString);
    void slot_updateWebProxyState(QString sState);
    void slot_toolButtonDstFolder_clicked(void);
    void on_pushButtonStartDownload_clicked(void);
    void slot_pushButtonCancelDownload_clicked(void);
    void slot_setDstFolder(QString);
    void on_lineEditDstFolder_editingFinished(void);
    void on_lineEditDstFolderUserSettings_editingFinished(void);
    void on_lineEditPages_editingFinished();
    void on_toolButtonDstFolder_clicked(void);
    void on_comboBoxZoom_currentIndexChanged(int index);

    // Gui related slots - pdf merge group box
    void on_pushButtonCreatePdf_clicked(void);
    void slot_stopPdfMergingProcess(void);
    void slot_applyMergeOptions(QString sSettings);
    void slot_updatePdfMergeProgressBar(int i);
    void slot_pdfWorkerCompleted(bool bCreatePdfBook, QString sFilename);
    void slot_updateMergeState(QString);
    void on_lineEditSrcFolder_editingFinished(void);
    void on_toolButtonSrcFolder_clicked(void);
    void on_radioButtonPdf_clicked(void);
    void on_radioButtonImages_clicked(void);
    void on_radioButtonMergePdftk_toggled(bool checked);
    void on_comboBoxPrinterRes_currentIndexChanged(int index);

    // Network related slots
    void slot_proxySetupDialog(void);
    void slot_bookPageLoaded(QNetworkReply* pReply);
    void slot_extractBookCopyright(QNetworkReply* pReply);
    void slot_fileDownloadFinished(bool);
    void slot_enablePushButtonGetBookInfo(bool bEnabled);
    void slot_showPdftkErrorMessage(QString sErrorMessage);
    void slot_restartBookInfoRequestOnProxyFailure(bool bState);
    void slot_autoProxyRequestCompleted(bool bState);
    void slot_handleSslErrors(QNetworkReply* pReply, const QList<QSslError> &errors);

private:
    // Init functions
    void initGuiComponents(void);
    void loadDefaultSettings(void);
    void getXmlGuiSetup(void);
    void setXmlGuiSetup(void);
    void setUserSettings(void);
    void setGuiLogo(void);
    void initDefaultSettings(void);
    void initNetworkComponents(void);
    void initMiscComponents(void);
    void initPrinterComponents(void);
    bool openHistoryDatabase(void);

    // Gui related functions
    void createMenus(void);
    void createActions(void);
    void createBookInfoGroupBox(void);
    void createDownloadSettingsGroupBox(void);
    void createMergeAndConversionGroupBox(void);
    QGridLayout* createLinkCollectorTab(void);
    QGridLayout* createHistoryTab(void);
#if USE_MENUS_IN_TABS
    QGridLayout* createProxyTab(void);
#endif
    void createNotificationDialog(void);
    void resizeEvent ( QResizeEvent * event );
    void changeEvent  ( QEvent  *event );
    void enablePdfMergeSettings(bool);
    void showFileInFilemanager(QString sFile);
    void removeDownloadedFileData(void);
    bool getFileList(QDir *pFileDir, QStringList *pFileList, QList<QByteArray> *pFileFormats, bool bMandatory );
    void setAppFont();
    void connectSignalsToCreateBatchJob();
    void addBookToHistory(void);
    void addHistoryLineEditFilter(QStringList* pFilterList = 0);
    void updateHistoryTableContent(QSqlQuery* pQuery);

    // Hathitrust documents related functions
    QString extractBookTitle(QWebFrame *pFrame, QWebElement *pDocument);
    QString extractBookAuthor(QWebElement *pDocument, int eSource);
    QString extractBookPublisher(QWebElement *pDocument, int eSource);
    void extractDocumentID(QWebElement *pdocument, QString *psDocumentID, QString *psDocumentCopyright, int *piMaxPageNum, int eSource);
    bool checkDownloadWholeBook(QWebElement *pdocument);
    void removeReservedCharFromString(QString *psInput, QString *psOutput = 0);
    void removeIllegalChar(QString *psPages);
    void convertAndMergeImagesToPdfFile(void);
    void convertAndMergeOcrToPdfFile(void);
    void mergePdfWithPdftk(void);
    bool checkBookRestriction(QWebElement *pDocument);
    void getBookCopyright(void);
    QString extractValueFromJSON(QString* psText, QString sTag);
    int setXmlPageSetup(QPrinter* aPrinter);

    struct Gui {
        // Scroll Area
        QScrollArea     *scrollArea;

        // Tabwidget
        QTabWidget      *tabWidget;
        QWidget         *mainTabWidget;
        CreateBatchJob  *batchJobTabWidget;
        QWidget         *linkCollectorTabWidget;
        QWidget         *historyTabWidget;
        QWidget         *settingsTabWidget;
        QWidget         *toolsTabWidget;
        QWidget         *aboutTabWidget;

        // Book info groupbox elements
        QGroupBox       *groupBoxBookInfo;
        QGridLayout     *gridLayoutBookInfo;
        QLabel          *labelBookURL;
        QLineEdit       *lineEditBookURL;
        QLabel          *labelLogoReading;
        QLabel          *labelBookTitle;
        QLabel          *labelBookTitleText;
        QLabel          *labelNumPages;
        QLabel          *labelNumPagesText;
        QLabel          *labelAuthor;
        QLabel          *labelAuthorText;
        QLabel          *labelBookID;
        QLabel          *labelBookIDText;
        QLabel          *labelPublished;
        QLabel          *labelPublishedText;
        QLabel          *labelCopyright;
        QLabel          *labelCopyrightText;
        QProgressBar    *progressBarBookPage;
        QLabel          *labelBookInfoState;
        QCheckBox       *checkBoxUseProxy;
        QPushButton     *pushButtonGetInfo;
        QMovie          *mpMovieRead;

        // Download settings groupbox elements
        QGroupBox       *groupBoxDownloadSettings;
        QGridLayout     *gridLayoutDownloadSettings;
        QLabel          *labelFormat;
        QRadioButton    *radioButtonPdf;
        QRadioButton    *radioButtonImages;
        QComboBox       *comboBoxZoom;
        QCheckBox       *checkBoxGetOcr;
        QLabel          *labelPagesText;
        QLineEdit       *lineEditPages;
        QLabel          *labelPagesExample;
        QLabel          *labelLogoLeeching;
        QLabel          *labelDstFolder;
        QLineEdit       *lineEditDstFolder;
        QToolButton     *toolButtonDstFolder;
        QCheckBox       *checkBoxAutoPdf;
        QCheckBox       *checkBoxResume;
        QCheckBox       *checkBoxAutoProxy;
        QCheckBox       *checkBoxWebProxy;
        QLabel          *labelWebProxyState;
        QProgressBar    *progressBarDownload;
        QLabel          *labelDownloadState;
        QPushButton     *pushButtonStartDownload;
        QMovie          *mpMovieLeech;

        // Pdf merge & conversion settings
        QGroupBox       *groupBoxMergeAndConversion;
        QGridLayout     *gridLayoutMergeAndConversion;
        QLabel          *labelOutputOptions;
        QRadioButton    *radioButtonMergePdftk;
        QRadioButton    *radioButtonMergeImages;
        QLabel          *labelPrinterRes;
        QComboBox       *comboBoxPrinterRes;
        QRadioButton    *radioButtonConvertImages;
        QLabel          *labelLogoMerging;
        QCheckBox       *checkBoxConvertOcrOnly;
        QLabel          *labelSrcFolder;
        QLineEdit       *lineEditSrcFolder;
        QToolButton     *toolButtonSrcFolder;
        QProgressBar    *progressBarCreatePdf;
        QLabel          *labelCreatePdfState;
        QPushButton     *pushButtonCreatePdf;
        QMovie          *mpMovieMerge;

        // Table widgets
        QTableWidget *tableWidgetLinkList;
        QTableWidget *tableWidgetHistory;
        // Exit button
        QPushButton *pushButtonExit;
    } Gui;

    struct Gui *ui;

    struct tUserSettingsDialog {

        // Auto update
        QGroupBox       *groupBoxAutoUpdate;
        QGridLayout     *gridLayoutAutoUpdate;
        QCheckBox       *checkBoxAutoUpdate;

        // Book info groupbox elements
        QGroupBox       *groupBoxBookInfo;
        QGridLayout     *gridLayoutBookInfo;
        QCheckBox       *checkBoxEnableProxy;
        QCheckBox       *checkBoxEnableAutoProxyActivation;
        QPushButton     *pushButtonProxySetup;

        // Download settings groupbox elements
        QGroupBox       *groupBoxDownloadSettings;
        QGridLayout     *gridLayoutDownloadSettings;
        QLabel          *labelFormat;
        QRadioButton    *radioButtonPdf;
        QRadioButton    *radioButtonImages;
        QComboBox       *comboBoxZoom;
        QCheckBox       *checkBoxGetOcr;
        QLabel          *labelDstFolder;
        QCheckBox       *checkBoxAutoProxy;
        QCheckBox       *checkBoxWebProxy;
        QCheckBox       *checkDownloadWholeBook;
        QLineEdit       *lineEditDstFolder;
        QToolButton     *toolButtonDstFolder;
        QCheckBox       *checkBoxAutoFolder;
        QRadioButton    *radioButtonAutoFolderByTitle;
        QRadioButton    *radioButtonAutoFolderByBookId;
        QLabel          *lableSpinBox;
        QSpinBox        *spinBoxTitleChars;
        QCheckBox       *checkBoxAutoPdf;
        QCheckBox       *checkBoxDeleteFiles;
        QCheckBox       *checkBoxResume;

        // Pdf merge & conversion settings
        QGroupBox    *groupBoxMergeAndConversion;
        QGridLayout  *gridLayoutMergeAndConversion;
        QLabel       *labelOutputOptions;
        QRadioButton *radioButtonMergePdftk;
        QRadioButton *radioButtonMergeImages;
        QLabel       *labelPrinterRes;
        QComboBox    *comboBoxPrinterRes;
        QRadioButton *radioButtonConvertImages;
        QCheckBox    *checkBoxConvertOcrOnly;
        QCheckBox    *checkBoxKeepExistingFile;

    } tUserSettingsDialog;

    struct tUserSettingsDialog* mpUserSettingsDialog;

    // GUI menu elements
    QMenu                   *mpFileMenu;
    QMenu                   *mpOptionsMenu;
    QMenu                   *mpStyleSubmenu;
    QMenu                   *mpToolMenu;
    QMenu                   *mpHelpMenu;
    QAction                 *mpExitAct;
    QAction                 *mpProxyAct;
    QAction                 *mpPageAct;
    QAction                 *mpStyleAct;
    QAction                 *mpFontAct;
    QAction                 *mpResetGuiAct;
    QAction                 *mpBatchAct;
    QAction                 *mpExportAct;
    QAction                 *mpMergePdfs;
    QAction                 *mpUserSettingsAct;
    QAction                 *mpHelpAct;
    QAction                 *mpUpdateAct;
    QAction                 *mpWhatsNewAct;
    QAction                 *mpAutoUpdateAct;
    QAction                 *mpAboutAct;
    QAction                 *mpContactAct;
//    QAction                 *mpAboutQtAct;

    QString                 msDefaultStyle;
    QFont                   mDefaultFont;

    QDialog*                mpWebProxyNotification;
    QDialog*                mpUserSettings;

    // Network elements
    QWebPage                *mpBookPage;
    QNetworkAccessManager   *mpNetwUpdateManager;
    QNetworkAccessManager   *mpNetwBookInfoManager;
    FileDownloader          *mpFileDownloader;
    QNetworkReply           *mpBookInfoReply;
    QNetworkReply           *mpUpdateReply;
    QNetworkProxy           *mpProxy;
    QNetworkProxy           *mpUserProxy;
    ProxyDialog             *mpProxyDialog;
    QString                 msTargetUrl;
    QList<QUrl>             mUrlProxyList;
    AutoProxy               *mpAutoProxy;

    // PDF printer
    QPrinter                *mpPrinter;
    QPrinter                *mpSetupPrinter;
    QPageSetupDialog        *mpPageSetup;
    QThread                 *mpThread;

    // Miscellaneous: Document info, setttings, control falgs, timer, handler
    QClipboard              *mpClipboard;
    QFile                   *mpFile;
    QFile                   *mpUpdateFile;
    QTimer                  *mpRetryTimer;
    QTimer                  *mpRetryTimerFeedback;
    QTimer                  *mpClipboardTimer;
    QSqlDatabase             mHistoryDatabase;
    QString                 mHistoryDbName;
    QStringList             *mpsListColumnHeaders;
    QStringList             msPrevHistoryItem;
    QStringList             msLatestHistoryItem;

    QVector<int>            *mpviZoomlevel;
    QVector<int>            *mpviDpi;

    int                     miMaxPageNum;
    int                     miTotalPageNum;
    int                     miRetry;
    int                     mFontsize;
    QString                 msDocumentID;
    QString                 msFontFamily;
    bool                    mbEmptyDocument;
    bool                    mbhttpRequestAborted;
    bool                    mbStopPdfMergingProcessFlag;
    bool                    mBatchJobActivated;
    bool                    mLinkExportActivated;
    bool                    mbTimerActive;
    tUserSettings           mtUserSettings;
    tUserSettings           mtDefaultSettings;
    tGuiSettings            mtUserGuiSettings;
    tGuiSettings            mtDefaultGuiSettings;
    QStringList             msRightsCodeDscr;
    QStringList             msRightsCode;
    QStringList             msRightsPublicDomainUs;
    bool                    mbDocumentAvailable;
    bool                    mbWholeBookAvailable;
    int                     miCurPdfRetry;
    int                     miPdfRetryThreshold;

};


#endif // HATHIDOWNLOADHELPER_H
