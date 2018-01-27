#include "hathidownloadhelper.h"
#include "createpdfworker.h"

#include <QStyleFactory>
#include <QUrl>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QFileDialog>
#include <QNetworkProxy>
#include <QWebView>
#include <QInputDialog>
#include <QFontDialog>
#include <QApplication>
#include <QDateTime>
#include <QSpacerItem>
#include <QButtonGroup>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextEdit>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlQueryModel>

#define connect(src, sig, trgt, slt) connect(src, sig, trgt, slt, Qt::UniqueConnection)

/*-----------------------------------------------------------------------------------------------*
 * GUI: Download directory selection dialog user settings
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_toolButtonDstFolder_clicked()
{
    QString sDirectory = QFileDialog::getExistingDirectory(this, tr("Choose Or Create Directory"),
                                                           mpUserSettingsDialog->lineEditDstFolder->text(),
                                                           QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog );
    if (!sDirectory.isEmpty())
    {
        mpUserSettingsDialog->lineEditDstFolder->setText(sDirectory);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: create book info group box
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::createBookInfoGroupBox()
{
    ui = &Gui;
    ui->groupBoxBookInfo    = new QGroupBox(tr("Book information"));
    ui->gridLayoutBookInfo  = new QGridLayout;

    // Row 0 -------------------------------------------------------------------------------------
    ui->labelBookURL        = new QLabel(tr("Book URL: "));
    ui->lineEditBookURL     = new QLineEdit();
    connect(ui->lineEditBookURL, SIGNAL(editingFinished()), this, SLOT(on_lineEditBookURL_editingFinished()));
    QSpacerItem *pSpacerItem = new QSpacerItem(0, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

    ui->gridLayoutBookInfo->addWidget(ui->labelBookURL, 0, 0, 1 ,1);
    ui->gridLayoutBookInfo->addWidget(ui->lineEditBookURL, 0, 1, 1, 4);
    ui->gridLayoutBookInfo->addItem(pSpacerItem, 0, 5, 1, 1);

    // Row 1 -------------------------------------------------------------------------------------
    ui->labelLogoReading    = new QLabel(tr("LOGO"));
    ui->labelLogoReading->setAlignment(Qt::AlignCenter);
    ui->labelBookTitle      = new QLabel(tr("Book title: "));
    ui->labelBookTitleText  = new QLabel(tr("-"));
    ui->labelBookTitleText->setWordWrap(true);

    ui->gridLayoutBookInfo->addWidget(ui->labelBookTitle, 1, 0, 1, 1);
    ui->gridLayoutBookInfo->addWidget(ui->labelLogoReading, 1, 5, 3, 1);
    ui->gridLayoutBookInfo->addWidget(ui->labelBookTitleText, 1, 1, 1, 4);

    // Row 2 -------------------------------------------------------------------------------------
    ui->labelNumPages       = new QLabel(tr("Number of Pages: "));
    ui->labelNumPagesText   = new QLabel(tr("-"));
    ui->labelAuthor         = new QLabel(tr("Author: "));
    ui->labelAuthor->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->labelAuthorText     = new QLabel(tr("-"));
    ui->labelAuthorText->setWordWrap(true);

    ui->gridLayoutBookInfo->addWidget(ui->labelNumPages, 2, 0, 1, 1);
    ui->gridLayoutBookInfo->addWidget(ui->labelNumPagesText, 2, 1, 1, 1);
    ui->gridLayoutBookInfo->addWidget(ui->labelAuthor, 2, 2, 1, 1);
    ui->gridLayoutBookInfo->addWidget(ui->labelAuthorText, 2, 3, 1, 2);

    // Row 3 -------------------------------------------------------------------------------------
    ui->labelBookID         = new QLabel(tr("Book ID: "));
    ui->labelBookIDText     = new QLabel(tr("-"));
    ui->labelPublished      = new QLabel(tr("Published: "));
    ui->labelPublished->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->labelPublishedText  = new QLabel(tr("-"));
    ui->labelPublishedText->setWordWrap(true);

    ui->gridLayoutBookInfo->addWidget(ui->labelBookID, 3, 0, 1, 1);
    ui->gridLayoutBookInfo->addWidget(ui->labelBookIDText, 3, 1, 1, 1);
    ui->gridLayoutBookInfo->addWidget(ui->labelPublished, 3, 2, 1, 1);
    ui->gridLayoutBookInfo->addWidget(ui->labelPublishedText, 3, 3, 1, 2);

    // Row 4 -------------------------------------------------------------------------------------
    ui->labelCopyright      = new QLabel(tr("Copyright: "));
    ui->labelCopyright->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->labelCopyrightText  = new QLabel(tr("-"));
    ui->labelCopyrightText->setWordWrap(true);

    ui->gridLayoutBookInfo->addWidget(ui->labelCopyright, 4, 0, 1, 1);
    ui->gridLayoutBookInfo->addWidget(ui->labelCopyrightText, 4, 1, 1, 5);

    // Row 5 -------------------------------------------------------------------------------------
    ui->progressBarBookPage = new QProgressBar();
    ui->progressBarBookPage->setAlignment(Qt::AlignHCenter);
    ui->progressBarBookPage->setMinimum(0);
    ui->progressBarBookPage->setMaximum(1);
    ui->progressBarBookPage->setValue(0);
    ui->labelBookInfoState  = new QLabel(tr(""));
    ui->checkBoxUseProxy    = new QCheckBox(tr("use proxy server"), this);
    ui->checkBoxUseProxy->setToolTip(QString("Enable download via proxy server"));
    ui->checkBoxUseProxy->setChecked(mtUserSettings.bUseProxy);
    ui->pushButtonGetInfo   = new QPushButton(tr("Get book info"));
    ui->pushButtonGetInfo->setObjectName("pushButtonGetInfo");
    connect(ui->checkBoxUseProxy, SIGNAL(clicked(bool)), this, SLOT(on_checkBoxUseProxy_clicked(bool)));
    connect(ui->pushButtonGetInfo, SIGNAL(clicked()), this, SLOT(on_pushButtonGetInfo_clicked()));

    ui->gridLayoutBookInfo->addWidget(ui->progressBarBookPage, 5, 0, 1, 2);
    ui->gridLayoutBookInfo->addWidget(ui->labelBookInfoState, 5, 2, 1, 2);
    ui->gridLayoutBookInfo->addWidget(ui->checkBoxUseProxy, 5, 4, 1, 1);
    ui->gridLayoutBookInfo->addWidget(ui->pushButtonGetInfo, 5, 5, 1, 1);


    ui->gridLayoutBookInfo->setColumnStretch(0, 1);
    ui->gridLayoutBookInfo->setColumnStretch(1, 5);
    ui->gridLayoutBookInfo->setColumnStretch(2, 10);
    ui->gridLayoutBookInfo->setColumnStretch(3, 5);
    ui->gridLayoutBookInfo->setColumnStretch(4, 5);
    ui->gridLayoutBookInfo->setColumnStretch(5, 1);

    ui->groupBoxBookInfo->setLayout(ui->gridLayoutBookInfo);

}

/*-----------------------------------------------------------------------------------------------*
 * GUI: create download settings group box
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::createDownloadSettingsGroupBox()
{
    ui->groupBoxDownloadSettings      = new QGroupBox(tr("Download settings"));
    ui->gridLayoutDownloadSettings    = new QGridLayout;

    // Row 0 -------------------------------------------------------------------------------------
    ui->labelFormat     = new QLabel(tr("Download pages as "));
    ui->radioButtonPdf  = new QRadioButton(tr("pdfs (1 pdf per page, searchable text,  watermark on left border,  max. 30 pages / 5 min)"));
    ui->radioButtonPdf->setChecked(mtUserSettings.bDownloadPdfs);
    connect(ui->radioButtonPdf, SIGNAL(clicked()), this, SLOT(on_radioButtonPdf_clicked()));

    ui->gridLayoutDownloadSettings->addWidget(ui->labelFormat, 0, 0, 1, 1);
    ui->gridLayoutDownloadSettings->addWidget(ui->radioButtonPdf, 0, 1, 1, 5);

    // Row 1 -------------------------------------------------------------------------------------
    ui->radioButtonImages   = new QRadioButton(tr("images (1 file per page, pdf generation possible, max. 22 MB / 1 min), image zoom:"));
    ui->radioButtonImages->setChecked(mtUserSettings.bDownloadImages);
    ui->comboBoxZoom        = new QComboBox(0);
    QSpacerItem* pSpacerItem1 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QSpacerItem* pSpacerItem2 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
    connect(ui->radioButtonImages, SIGNAL(clicked()), this, SLOT(on_radioButtonImages_clicked()));
    connect(ui->radioButtonImages, SIGNAL(toggled(bool)), ui->comboBoxZoom, SLOT(setEnabled(bool)));
    ui->comboBoxZoom->setEnabled(mtUserSettings.bDownloadImages);

    ui->gridLayoutDownloadSettings->addItem(pSpacerItem1, 1, 0, 1, 1);
    ui->gridLayoutDownloadSettings->addWidget(ui->radioButtonImages, 1, 1, 1, 3);
    ui->gridLayoutDownloadSettings->addWidget(ui->comboBoxZoom, 1, 4, 1, 1);
    ui->gridLayoutDownloadSettings->addItem(pSpacerItem2, 1, 5, 1, 1);

    // Row 2 -------------------------------------------------------------------------------------
    ui->checkBoxGetOcr  = new QCheckBox(tr("download OCR text (required for creation of searchable pdfs from images)"));
    ui->checkBoxGetOcr->setChecked(mtUserSettings.bDownloadOcr);
    QSpacerItem* pSpacerItem3 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

    ui->gridLayoutDownloadSettings->addItem(pSpacerItem3, 2, 0, 1, 1);
    ui->gridLayoutDownloadSettings->addWidget(ui->checkBoxGetOcr, 2, 1, 1, 5);

    // Row 3 -------------------------------------------------------------------------------------
    ui->labelPagesText      = new QLabel(tr("Pages: "));
    ui->lineEditPages       = new QLineEdit(0);
    ui->labelPagesExample   = new QLabel(tr("(e.g. 1, 3, 5 - 10)"));
    QSpacerItem* pSpacerItem4 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
    connect(ui->lineEditPages, SIGNAL(editingFinished()), this, SLOT(on_lineEditPages_editingFinished()));

    ui->gridLayoutDownloadSettings->addWidget(ui->labelPagesText, 3, 0, 1, 1);
    ui->gridLayoutDownloadSettings->addWidget(ui->lineEditPages, 3, 1, 1, 3);
    ui->gridLayoutDownloadSettings->addWidget(ui->labelPagesExample, 3, 4, 1, 1);
    ui->gridLayoutDownloadSettings->addItem(pSpacerItem4, 3, 5, 1, 1);

    // Row 4 -------------------------------------------------------------------------------------
    ui->labelDstFolder      = new QLabel(tr("Destination folder: "));
    ui->lineEditDstFolder   = new QLineEdit(0);
    ui->toolButtonDstFolder = new QToolButton(0);
    ui->toolButtonDstFolder->setText(tr("..."));
    ui->labelLogoLeeching   = new QLabel(0);
    ui->labelLogoLeeching->setAlignment(Qt::AlignCenter);
    connect(ui->lineEditDstFolder, SIGNAL(editingFinished()), this, SLOT(on_lineEditDstFolder_editingFinished()));
    connect(ui->toolButtonDstFolder, SIGNAL(clicked()), this, SLOT(on_toolButtonDstFolder_clicked()));

    ui->gridLayoutDownloadSettings->addWidget(ui->labelDstFolder, 4, 0, 1, 1);
    ui->gridLayoutDownloadSettings->addWidget(ui->lineEditDstFolder, 4, 1, 1, 3);
    ui->gridLayoutDownloadSettings->addWidget(ui->toolButtonDstFolder, 4, 4, 1, 1);
    ui->gridLayoutDownloadSettings->addWidget(ui->labelLogoLeeching, 4, 5, 2, 1);

    // Row 5 -------------------------------------------------------------------------------------
    ui->checkBoxAutoPdf = new QCheckBox(tr("create pdf book after download"));
    ui->checkBoxAutoPdf->setToolTip(QString("The PDF merge & conversion process will start automatically when the download is complete"));
    ui->checkBoxAutoPdf->setChecked(mtUserSettings.bAutoPdfBook);
    ui->checkBoxResume = new QCheckBox(tr("resume book download"));
    ui->checkBoxResume->setToolTip(QString("Continue book download for the specified input data including checks for corrupted files."));
    ui->checkBoxResume->setChecked(mtUserSettings.bResume);
    ui->checkBoxAutoProxy = new QCheckBox(tr("enable AutoProxy"));;
    ui->checkBoxAutoProxy->setToolTip(QString("Enable automatic download via proxy server"));
    QSpacerItem* pSpacerItem5 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
    connect(ui->checkBoxResume, SIGNAL(toggled(bool)), this, SLOT(slot_checkBoxResume(bool)));

    ui->gridLayoutDownloadSettings->addItem(pSpacerItem5, 5, 0, 2, 1);
    ui->gridLayoutDownloadSettings->addWidget(ui->checkBoxAutoPdf, 5, 1, 1, 1);
    ui->gridLayoutDownloadSettings->addWidget(ui->checkBoxResume, 5, 2, 1, 1, Qt::AlignCenter);
    ui->gridLayoutDownloadSettings->addWidget(ui->checkBoxAutoProxy, 5, 3, 1, 2, Qt::AlignLeft);

    // Row 6 -------------------------------------------------------------------------------------
    ui->checkBoxWebProxy = new QCheckBox(tr("enable WebProxies"));
    ui->checkBoxWebProxy->setChecked(mtUserSettings.bEnableWebProxy);
    ui->checkBoxWebProxy->setToolTip(QString("Enable automatic download via webproxy server"));
    ui->labelWebProxyState      = new QLabel(0);
    connect(ui->checkBoxWebProxy, SIGNAL(toggled(bool)), this, SLOT(slot_enableWebProxy(bool)));
    connect(ui->checkBoxWebProxy, SIGNAL(toggled(bool)), this, SLOT(slot_disableAutoProxy(bool)));
    connect(ui->checkBoxAutoProxy, SIGNAL(toggled(bool)), this, SLOT(slot_disableWebProxy(bool)));
    connect(ui->checkBoxAutoProxy, SIGNAL(toggled(bool)), this, SLOT(slot_enableAutoProxy(bool)));

    ui->gridLayoutDownloadSettings->addWidget(ui->checkBoxWebProxy, 6, 1, 1, 1);
    ui->gridLayoutDownloadSettings->addWidget(ui->labelWebProxyState, 6, 2, 1, 3);

    // Row 7 -------------------------------------------------------------------------------------
    ui->progressBarDownload     = new QProgressBar(0);
    ui->progressBarDownload->setAlignment(Qt::AlignHCenter);
    ui->labelDownloadState      = new QLabel(0);
    ui->pushButtonStartDownload = new QPushButton(tr("Start download"),0);
    ui->pushButtonStartDownload->setObjectName("pushButtonStartDownload");
    connect(ui->pushButtonStartDownload, SIGNAL(clicked()), this, SLOT(on_pushButtonStartDownload_clicked()));

    ui->gridLayoutDownloadSettings->addWidget(ui->progressBarDownload, 7, 0, 1, 2);
    ui->gridLayoutDownloadSettings->addWidget(ui->labelDownloadState, 7, 2, 1, 3);
    ui->gridLayoutDownloadSettings->addWidget(ui->pushButtonStartDownload, 7, 5, 1, 1);

    ui->gridLayoutDownloadSettings->setColumnStretch(0,1);
    ui->gridLayoutDownloadSettings->setColumnStretch(1,5);
    ui->gridLayoutDownloadSettings->setColumnStretch(2,10);
    ui->gridLayoutDownloadSettings->setColumnStretch(3,5);
    ui->gridLayoutDownloadSettings->setColumnStretch(4,5);
    ui->gridLayoutDownloadSettings->setColumnStretch(5,1);

    ui->groupBoxDownloadSettings->setLayout(ui->gridLayoutDownloadSettings);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: create merge and conversion group box
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::createMergeAndConversionGroupBox()
{
    ui->groupBoxMergeAndConversion    = new QGroupBox(tr("PDF merge && conversion"));
    ui->gridLayoutMergeAndConversion  = new QGridLayout;

    // Row 0 -------------------------------------------------------------------------------------
    ui->labelOutputOptions      = new QLabel(tr("Output options: "));
    ui->radioButtonMergePdftk   = new QRadioButton(tr("merge pdfs"));
    ui->radioButtonMergePdftk->setChecked(mtUserSettings.bMergePdf);
    connect(ui->radioButtonMergePdftk, SIGNAL(toggled(bool)), this, SLOT(on_radioButtonMergePdftk_toggled(bool)));

    ui->gridLayoutMergeAndConversion->addWidget(ui->labelOutputOptions, 0, 0, 1, 1);
    ui->gridLayoutMergeAndConversion->addWidget(ui->radioButtonMergePdftk, 0, 1, 1, 5);

    // Row 1 -------------------------------------------------------------------------------------
    ui->radioButtonMergeImages  = new QRadioButton(tr("convert && merge images to pdf book"));
    ui->radioButtonMergeImages->setChecked(mtUserSettings.bImage2Book);
    ui->labelPrinterRes         = new QLabel(tr("set pdf resolution: "));
    ui->comboBoxPrinterRes      = new QComboBox(0);
    ui->labelLogoMerging        = new QLabel(0);
    ui->labelLogoMerging->setAlignment(Qt::AlignCenter);
    QSpacerItem* pSpacerItem1 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
    connect(ui->radioButtonMergePdftk, SIGNAL(toggled(bool)), ui->comboBoxPrinterRes, SLOT(setDisabled(bool)));
    ui->comboBoxPrinterRes->setEnabled(!mtUserSettings.bMergePdf);

    ui->gridLayoutDownloadSettings->addItem(pSpacerItem1, 1, 0, 3, 1);
    ui->gridLayoutMergeAndConversion->addWidget(ui->radioButtonMergeImages, 1, 1, 1, 2);
    ui->gridLayoutMergeAndConversion->addWidget(ui->labelPrinterRes, 1, 3, 1 , 1);
    ui->gridLayoutMergeAndConversion->addWidget(ui->comboBoxPrinterRes, 1, 4, 1, 1);
    ui->gridLayoutMergeAndConversion->addWidget(ui->labelLogoMerging, 1, 5, 3, 1);

    // Row 2 -------------------------------------------------------------------------------------
    ui->radioButtonConvertImages    = new QRadioButton(tr("convert images to single pdf files"));
    ui->radioButtonConvertImages->setChecked(mtUserSettings.bImage2Pdf);
    QSpacerItem* pSpacerItem2 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

    ui->gridLayoutMergeAndConversion->addWidget(ui->radioButtonConvertImages, 2, 1, 1, 2);
    ui->gridLayoutDownloadSettings->addItem(pSpacerItem2, 2, 3, 1, 2);

    // Row 3 -------------------------------------------------------------------------------------
    ui->checkBoxConvertOcrOnly = new QCheckBox(tr("use plain text (ocr text) only"));
    ui->checkBoxConvertOcrOnly->setChecked(mtUserSettings.bOcrOnly);

    ui->gridLayoutMergeAndConversion->addWidget(ui->checkBoxConvertOcrOnly, 3, 1, 1, 4);

    // Row 4 -------------------------------------------------------------------------------------
    ui->labelSrcFolder      = new QLabel(tr("Source folder: "));
    ui->lineEditSrcFolder   = new QLineEdit(0);
    ui->toolButtonSrcFolder = new QToolButton(0);
    ui->toolButtonSrcFolder->setText(tr("..."));
    QSpacerItem* pSpacerItem4 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
    connect(ui->lineEditSrcFolder, SIGNAL(editingFinished()), this,SLOT(on_lineEditSrcFolder_editingFinished()));
    connect(ui->toolButtonSrcFolder, SIGNAL(clicked()), this,SLOT(on_toolButtonSrcFolder_clicked()));

    ui->gridLayoutMergeAndConversion->addWidget(ui->labelSrcFolder, 4, 0, 1, 1);
    ui->gridLayoutMergeAndConversion->addWidget(ui->lineEditSrcFolder, 4, 1, 1, 3);
    ui->gridLayoutMergeAndConversion->addWidget(ui->toolButtonSrcFolder, 4, 4, 1, 1);
    ui->gridLayoutDownloadSettings->addItem(pSpacerItem4, 4, 5, 1, 1);

    // Row 5 -------------------------------------------------------------------------------------
    ui->progressBarCreatePdf     = new QProgressBar(0);
    ui->progressBarCreatePdf->setAlignment(Qt::AlignHCenter);
    ui->labelCreatePdfState      = new QLabel(0);
    ui->pushButtonCreatePdf = new QPushButton(tr("Create pdf"),0);
    ui->pushButtonCreatePdf->setObjectName("pushButtonCreatePdf");
    connect(ui->pushButtonCreatePdf, SIGNAL(clicked()), this, SLOT(on_pushButtonCreatePdf_clicked()));

    ui->gridLayoutMergeAndConversion->addWidget(ui->progressBarCreatePdf, 5, 0, 1, 2);
    ui->gridLayoutMergeAndConversion->addWidget(ui->labelCreatePdfState, 5, 2, 1, 3);
    ui->gridLayoutMergeAndConversion->addWidget(ui->pushButtonCreatePdf, 5, 5, 1, 1);

    ui->gridLayoutMergeAndConversion->setColumnStretch(0,1);
    ui->gridLayoutMergeAndConversion->setColumnStretch(1,5);
    ui->gridLayoutMergeAndConversion->setColumnStretch(2,10);
    ui->gridLayoutMergeAndConversion->setColumnStretch(3,5);
    ui->gridLayoutMergeAndConversion->setColumnStretch(4,5);
    ui->gridLayoutMergeAndConversion->setColumnStretch(5,1);

    ui->groupBoxMergeAndConversion->setLayout(ui->gridLayoutMergeAndConversion);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Create Link collector tab content
 *-----------------------------------------------------------------------------------------------*/
QGridLayout* HathiDownloadHelper::createLinkCollectorTab(void)
{
    // Link Collector tab
    QGridLayout *pGridLayoutLinkCollector   = new QGridLayout;
    QVBoxLayout *pVBoxLayoutCntrl           = new QVBoxLayout;

    // Dialog Cntrl ----------------------------------------------------------------------------
    QPushButton *pPushButtonStartDownload      = new QPushButton(QString("Start download"));
    connect(pPushButtonStartDownload, SIGNAL(clicked()), this, SLOT(slot_startLinkCollectorDownload()));
    pVBoxLayoutCntrl->addWidget(pPushButtonStartDownload);

    QPushButton *pPushButtonCreateBatchJob  = new QPushButton(QString("Add to\nbatch job"));
    connect(pPushButtonCreateBatchJob, SIGNAL(clicked()), this, SLOT(slot_addLinkCollectorListToBatchJob()));
    pVBoxLayoutCntrl->addWidget(pPushButtonCreateBatchJob);

    QSpacerItem* spacer0                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pVBoxLayoutCntrl->addItem(spacer0);

    QPushButton *pPushButtonDownloadSettings    = new QPushButton(QString("Download settings"));
    connect(pPushButtonDownloadSettings, SIGNAL(clicked()), this, SLOT(slot_userSettingsDialog()));
    pVBoxLayoutCntrl->addWidget(pPushButtonDownloadSettings);
    pGridLayoutLinkCollector->addLayout(pVBoxLayoutCntrl, 0, 1, 1, 1);

    // Link collector Cntrl -------------------------------------------------------------------------
    QVBoxLayout *pVBoxLayoutCntrlLinks   = new QVBoxLayout;
    QSpacerItem* spacer1                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pVBoxLayoutCntrlLinks->addItem(spacer1);

    QPushButton *pPushButtonRemoveLink    = new QPushButton(QString("Remove"));
    connect(pPushButtonRemoveLink, SIGNAL(clicked()), this, SLOT(on_pushButtonRemoveLink_clicked()));
    pVBoxLayoutCntrlLinks->addWidget(pPushButtonRemoveLink);

    QPushButton *pPushButtonRemoveAll = new QPushButton(QString("Remove all"));
    connect(pPushButtonRemoveAll, SIGNAL(clicked()), this, SLOT(on_pushButtonRemoveAll_clicked()));
    pVBoxLayoutCntrlLinks->addWidget(pPushButtonRemoveAll);

    pGridLayoutLinkCollector->addLayout(pVBoxLayoutCntrlLinks, 1, 1, 1, 1);

    // Table
    ui->tableWidgetLinkList = new QTableWidget(this);
    ui->tableWidgetLinkList->setColumnCount(1);
    QStringList sColumnHeaders;
    sColumnHeaders.append("Book URL");
    ui->tableWidgetLinkList->setHorizontalHeaderLabels(sColumnHeaders);
    ui->tableWidgetLinkList->verticalHeader()->setVisible(false);
    ///???ui->tableWidgetLinkList->setStyleSheet("QTableView {selection-background-color: red;}");
    ui->tableWidgetLinkList->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->tableWidgetLinkList->setShowGrid(true);
    ui->tableWidgetLinkList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidgetLinkList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetLinkList->setAlternatingRowColors(true);
    ui->tableWidgetLinkList->setStyleSheet("alternate-background-color: light;");

#if QT_VERSION >= 0x050000
    ui->tableWidgetLinkList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
    ui->tableWidgetLinkList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif

    pGridLayoutLinkCollector->addWidget(ui->tableWidgetLinkList, 0, 0, 2, 1);

    return pGridLayoutLinkCollector;
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Create History tab content
 *-----------------------------------------------------------------------------------------------*/
QGridLayout* HathiDownloadHelper::createHistoryTab()
{
    // History tab
    QGridLayout *pGridLayoutHistory   = new QGridLayout;
    QVBoxLayout *pVBoxLayoutCntrl     = new QVBoxLayout;

    // Dialog Cntrl ----------------------------------------------------------------------------
    QSpacerItem* spacer0                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pVBoxLayoutCntrl->addItem(spacer0);

    QPushButton *pPushButtonOpenPath    = new QPushButton(QString("Open path"));
    connect(pPushButtonOpenPath, SIGNAL(clicked()), this, SLOT(on_pushButtonOpenPath_clicked()));
    connect(this, SIGNAL(enableOpenPathButton(bool)),pPushButtonOpenPath,SLOT(setEnabled(bool)));
    pVBoxLayoutCntrl->addWidget(pPushButtonOpenPath);
    pPushButtonOpenPath->setEnabled(false);

    QSpacerItem* spacer1                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pVBoxLayoutCntrl->addItem(spacer1);

    QPushButton *pPushButtonRemoveItem    = new QPushButton(QString("Remove"));
    connect(pPushButtonRemoveItem, SIGNAL(clicked()), this, SLOT(on_pushButtonRemoveItem_clicked()));
    pVBoxLayoutCntrl->addWidget(pPushButtonRemoveItem);

    QPushButton *pPushButtonRemoveAll = new QPushButton(QString("Remove all"));
    connect(pPushButtonRemoveAll, SIGNAL(clicked()), this, SLOT(on_pushButtonRemoveAllItems_clicked()));
    pVBoxLayoutCntrl->addWidget(pPushButtonRemoveAll);

    QSpacerItem* spacer2                    = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pVBoxLayoutCntrl->addItem(spacer2);

    pGridLayoutHistory->addLayout(pVBoxLayoutCntrl, 0, 1, 2, 1);

    // Table --------------------------------------------------------------------------------------

    ui->tableWidgetHistory          = new QTableWidget(this);
    ui->tableWidgetHistory->setColumnCount(6);

    mHistoryDbName = QString("history");
    mpsListColumnHeaders = new QStringList;
    mpsListColumnHeaders->append("Date");
    mpsListColumnHeaders->append("Link");
    mpsListColumnHeaders->append("Title");
    mpsListColumnHeaders->append("Author");
    mpsListColumnHeaders->append("Published");
    mpsListColumnHeaders->append("Path");
    ui->tableWidgetHistory->setHorizontalHeaderLabels(*mpsListColumnHeaders);
    addHistoryLineEditFilter();

    ui->tableWidgetHistory->verticalHeader()->setVisible(false);
    ui->tableWidgetHistory->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->tableWidgetHistory->setShowGrid(true);
    ui->tableWidgetHistory->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetHistory->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetHistory->setAlternatingRowColors(true);
    ui->tableWidgetHistory->setStyleSheet("alternate-background-color: light;");

#if QT_VERSION >= 0x050000
    ui->tableWidgetHistory->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableWidgetHistory->horizontalHeader()->setSectionResizeMode(ui->tableWidgetHistory->columnCount()-1, QHeaderView::Stretch);
#else
    ui->tableWidgetHistory->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
    ui->tableWidgetHistory->horizontalHeader()->setResizeMode(ui->tableWidgetHistory->columnCount()-1, QHeaderView::Stretch);
#endif

    pGridLayoutHistory->addWidget(ui->tableWidgetHistory, 0, 0, 2, 1);
    slot_loadTableWidgetHistory();

    connect(ui->tableWidgetHistory->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(slot_sortTableWidgetHistory(int)));
    connect(ui->tableWidgetHistory, SIGNAL(itemSelectionChanged()), this, SLOT(slot_updateSelectionTableWidgetHistory()));
    connect(ui->tableWidgetHistory, SIGNAL(cellChanged(int,int)), this, SLOT(slot_updateTableWidgetHistory(int, int)));
    connect(this, SIGNAL(updateHistoryTableContentPath(QString)), this, SLOT(slot_updateTableWidgetHistoryPath(QString)));

    return pGridLayoutHistory;
}
#if USE_MENUS_IN_TABS
/*-----------------------------------------------------------------------------------------------*
 * GUI: Create Proxy tab
 *-----------------------------------------------------------------------------------------------*/
QGridLayout* HathiDownloadHelper::createProxyTab()
{
    // Proxy tab
    QGridLayout *pGridLayoutProxy   = new QGridLayout;
    QVBoxLayout *pVBoxLayoutCntrl     = new QVBoxLayout;


    return pGridLayoutProxy;
}
#endif

/*-----------------------------------------------------------------------------------------------*
 * GUI: sort history data
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_sortTableWidgetHistory(int iCol)
{
    static int iPrevCol = -1;
    static Qt::SortOrder prevOrder = Qt::AscendingOrder;

    if (ui->tableWidgetHistory->rowCount() > 1)
    {
        QStringList sFilterList;
        for (int iColCnt = 0; iColCnt < ui->tableWidgetHistory->columnCount(); iColCnt++)
        {
            QLineEdit* pLineEdit = (QLineEdit*)ui->tableWidgetHistory->cellWidget(0,iColCnt);
            sFilterList.append(pLineEdit->text());
        }
        ui->tableWidgetHistory->removeRow(0);

        if ((iPrevCol == iCol) && (prevOrder == Qt::AscendingOrder))
        {
            ui->tableWidgetHistory->sortItems(iCol,Qt::DescendingOrder);
            prevOrder = Qt::DescendingOrder;
        }
        else
        {
            ui->tableWidgetHistory->sortItems(iCol,Qt::AscendingOrder);
            prevOrder = Qt::AscendingOrder;
        }

        addHistoryLineEditFilter(&sFilterList);
    }
    iPrevCol = iCol;
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Stores last select row item values
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_updateSelectionTableWidgetHistory()
{
    if (ui->tableWidgetHistory->rowCount() > 0)
    {
        int iRow = ui->tableWidgetHistory->selectionModel()->currentIndex().row();
        msPrevHistoryItem.clear();
        for(int iCol = 0; iCol < mpsListColumnHeaders->length(); iCol++)
            msPrevHistoryItem.append(ui->tableWidgetHistory->item(iRow, iCol)->text());

        QFile myFile(QDir::toNativeSeparators(ui->tableWidgetHistory->item(iRow, mpsListColumnHeaders->length()-1)->text()));
        QFileInfo myFileInfo(myFile);
        emit enableOpenPathButton(myFileInfo.exists());
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Update path for latest data set
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_updateTableWidgetHistoryPath(QString sPath)
{
    if (ui->tableWidgetHistory->rowCount() > 0)
    {
        if (sPath.isEmpty()|| (ui->tableWidgetHistory->item(ui->tableWidgetHistory->rowCount()-1, 5) == 0))
                return;

        // Update book entry in data base
        if (openHistoryDatabase())
        {
            QSqlQuery query = QSqlQuery( mHistoryDatabase );

            QString sDbColTitles;
            foreach(QString sColTitle, *mpsListColumnHeaders)
                sDbColTitles.append(sColTitle + " TEXT,");
            sDbColTitles.chop(1);

            if(!query.exec( "create table if not exists " + mHistoryDbName + "(" + sDbColTitles + ")"))
                qDebug() << "HDH: Unable to create hstr data table in database - " << query.lastError();

            if(!query.exec("BEGIN TRANSACTION"))
                qDebug() << "HDH: Unable to begin transaction of hstr data to database - " << query.lastError();

            // Set Current values
            QString sQuery = "UPDATE " + mHistoryDbName + " SET ";
            foreach(QString sColumnName, *mpsListColumnHeaders)
                sQuery.append(sColumnName + " = :" + sColumnName + ", ");
            // Remove last ', '
            sQuery.chop(2);

            // Select latest values
            sQuery.append(" WHERE ");
            foreach(QString sColumnName, *mpsListColumnHeaders)
                sQuery.append(sColumnName + " = :Prev" + sColumnName + " AND ");
            // Remove last 'comma ' AND '
            sQuery.chop(5);

            query.prepare(sQuery);

            // bin previous data
            for(int iCol = 0; iCol < mpsListColumnHeaders->length(); iCol++)
                query.bindValue(":Prev" + mpsListColumnHeaders->at(iCol), msLatestHistoryItem.at(iCol));

            // bind new data
            msLatestHistoryItem.last() =  sPath;
            for(int iCol = 0; iCol < mpsListColumnHeaders->length(); iCol++)
                query.bindValue(":" + mpsListColumnHeaders->at(iCol), msLatestHistoryItem.at(iCol));
            query.exec();

            if(!query.exec("COMMIT"))
                qDebug() << "HDH: Unable to commit hstr data to database - " << query.lastError();

            on_lineEditFilterHistory_changed();
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Update history data in specified row
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_updateTableWidgetHistory(int iRow, int iColumn)
{
    if ((ui->tableWidgetHistory->rowCount() > 0) && (iRow > 0))
    {
        if (ui->tableWidgetHistory->rowCount() > 0)
        {
            // Check if last row has all 5 items
            if (ui->tableWidgetHistory->item(ui->tableWidgetHistory->rowCount()-1, 5) == 0)
                return;
        }

        if (ui->tableWidgetHistory->item(iRow,iColumn)->text().isEmpty())
            ui->tableWidgetHistory->item(iRow,iColumn)->setText("-");

        // Update book entry in data base
        if (openHistoryDatabase())
        {
            QSqlQuery query = QSqlQuery( mHistoryDatabase );

            QString sDbColTitles;
            foreach(QString sColTitle, *mpsListColumnHeaders)
                sDbColTitles.append(sColTitle + " TEXT,");
            sDbColTitles.chop(1);

            if(!query.exec( "create table if not exists " + mHistoryDbName + "(" + sDbColTitles + ")"))
                qDebug() << "HDH: Unable to create hstr data table in database - " << query.lastError();

            if(!query.exec("BEGIN TRANSACTION"))
                qDebug() << "HDH: Unable to begin transaction of hstr data to database - " << query.lastError();

            // Set Current values
            QString sQuery = "UPDATE " + mHistoryDbName + " SET ";
            foreach(QString sColumnName, *mpsListColumnHeaders)
                sQuery.append(sColumnName + " = :" + sColumnName + ", ");
            // Remove last ', '
            sQuery.chop(2);

            // Select previous values
            sQuery.append(" WHERE ");
            foreach(QString sColumnName, *mpsListColumnHeaders)
                sQuery.append(sColumnName + " = :Prev" + sColumnName + " AND ");
            // Remove last 'comma ' AND '
            sQuery.chop(5);

            query.prepare(sQuery);

            // bind new data
            for(int iCol = 0; iCol < mpsListColumnHeaders->length(); iCol++)
                query.bindValue(":" + mpsListColumnHeaders->at(iCol), ui->tableWidgetHistory->item(iRow,iCol)->text());

            // bin previous data
            for(int iCol = 0; iCol < mpsListColumnHeaders->length(); iCol++)
                query.bindValue(":Prev" + mpsListColumnHeaders->at(iCol), msPrevHistoryItem.at(iCol));

            query.exec();

            if(!query.exec("COMMIT"))
                qDebug() << "HDH: Unable to commit hstr data to database - " << query.lastError();

            on_lineEditFilterHistory_changed();
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Open history database
 *-----------------------------------------------------------------------------------------------*/
bool HathiDownloadHelper::openHistoryDatabase(void)
{
    bool bDbState = true;
    QString sDbName( "hstr.db3" );
    //QFile::remove( sDbName ); // alte sqlite-datei löschen
#if defined(Q_OS_WIN)
    QString sPathDB = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#else
#if QT_VERSION >= 0x050000
    QString sPathDB = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data/organization/application";
#else
    QString sPathDB = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
#endif // Q_OS_WIN

    QFileInfo fi(sDbName);
    if (fi.isAbsolute())
    {
        sPathDB = fi.path();
    }

    QDir filePath;
    filePath.mkpath(sPathDB);

    if (!mHistoryDatabase.isOpen())
    {
        mHistoryDatabase = QSqlDatabase::addDatabase( "QSQLITE" );
        mHistoryDatabase.setDatabaseName(sPathDB + QDir::separator() + fi.fileName());
        if (!mHistoryDatabase.open())
        {
            bDbState = false;
            qDebug() << "HDH: Unable to open hstr database - " << mHistoryDatabase.lastError();
        }
    }
    return bDbState;
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Read history data from file hstr.db3
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::addHistoryLineEditFilter(QStringList* psFilterValueList)
{
    disconnect(ui->tableWidgetHistory, SIGNAL(cellChanged(int,int)), this, SLOT(slot_updateTableWidgetHistory(int, int)));
    ui->tableWidgetHistory->insertRow(0);
    for(int iCol = 0; iCol < ui->tableWidgetHistory->columnCount(); iCol++)
    {
        QLineEdit* pLineEditFilter = new QLineEdit(this);
        pLineEditFilter->setPlaceholderText("Filter");

        // Add latest filter value
        if ((psFilterValueList != 0) && psFilterValueList->count() > iCol)
        {
            pLineEditFilter->setText(psFilterValueList->at(iCol));
        }
        QTableWidgetItem *pItem = new QTableWidgetItem;
        ui->tableWidgetHistory->setItem(0,iCol,pItem);
        ui->tableWidgetHistory->setCellWidget(0,iCol,pLineEditFilter);

        // connect with DB filter
        connect(pLineEditFilter,SIGNAL(textChanged(QString)), this, SLOT(on_lineEditFilterHistory_changed()));
    }
    connect(ui->tableWidgetHistory, SIGNAL(cellChanged(int,int)), this, SLOT(slot_updateTableWidgetHistory(int, int)));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Udpate history table content
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::updateHistoryTableContent(QSqlQuery* pQuery)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(*pQuery);

    QTableWidgetItem *pItem;
    QSqlRecord row;
    QString sTagValue;
    int iRow, iIdx;
    disconnect(ui->tableWidgetHistory, SIGNAL(cellChanged(int,int)), this, SLOT(slot_updateTableWidgetHistory(int, int)));
    for(iIdx = 0, row = model->record(iIdx); !row.isEmpty() && iIdx < model->rowCount(); iIdx++, row = model->record(iIdx))
    {
        // Add new row
        iRow = ui->tableWidgetHistory->rowCount();
        ui->tableWidgetHistory->insertRow(iRow);

        // Add Date & time, URL, Title, Author, Published, Folder
        for(int iCol = 0; iCol < ui->tableWidgetHistory->columnCount(); iCol++)
        {
            // Get column name
            sTagValue = row.field(mpsListColumnHeaders->at(iCol)).value().toString();
            pItem = new QTableWidgetItem;
            pItem->setFlags(pItem->flags()|Qt::ItemIsEditable);
            if (!sTagValue.isEmpty())
                pItem->setText(sTagValue);

            // Set items
            ui->tableWidgetHistory->setItem(iRow, iCol, pItem);
        }
    }
    connect(ui->tableWidgetHistory, SIGNAL(cellChanged(int,int)), this, SLOT(slot_updateTableWidgetHistory(int, int)));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Read history data from file hstr.db3
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_loadTableWidgetHistory()
{
    if (openHistoryDatabase())
    {
        QSqlQuery query = QSqlQuery( mHistoryDatabase );

        QString sTagValue;
        int iIdx = 0;

        if (!mHistoryDatabase.tables().contains(mHistoryDbName))
        {
            QString sDbColTitles;
            foreach(QString sColTitle, *mpsListColumnHeaders)
                sDbColTitles.append(sColTitle + " TEXT,");
            sDbColTitles.chop(1);

            if(!query.exec( "create table if not exists " + mHistoryDbName + "(" + sDbColTitles + ")"))
                qDebug() << "HDH: Unable to create hstr data table in database - " << query.lastError();

            QString sHistory = readFile(QString("hstr.xml"));
            if (!sHistory.isEmpty())
            {
                QString sDate, sLink, sTitle, sAuthor, sPublished, sPath;
                if(!query.exec("BEGIN TRANSACTION"))
                    qDebug() << "HDH: Unable to begin transaction of hstr data to database - " << query.lastError();
                do
                {
                    sTagValue = myXmlReader(&sHistory, "book", &iIdx);
                    if (!sTagValue.isEmpty())
                    {
                        // Get entry data
                        sDate = myXmlReader(&sHistory, "datetime", &iIdx);
                        if (sDate.isEmpty())
                            sDate = "-";
                        sLink = myXmlReader(&sHistory, "link", &iIdx);
                        if (sLink.isEmpty())
                            sLink = "-";
                        sTitle = myXmlReader(&sHistory, "title", &iIdx);
                        if (sTitle.isEmpty())
                            sTitle = "-";
                        sAuthor = myXmlReader(&sHistory, "author", &iIdx);
                        if (sAuthor.isEmpty())
                            sAuthor = "-";
                        sPublished = myXmlReader(&sHistory, "published", &iIdx);
                        if (sPublished.isEmpty())
                            sPublished = "-";
                        sPath = myXmlReader(&sHistory, "folder", &iIdx);
                        if (sPath.isEmpty())
                            sPath = "-";

                        // Try to detemine pdf-file in path for current book title
                        QFile pdfFile(sPath);
                        QFileInfo pdfFileInfo(pdfFile);
                        if (pdfFileInfo.exists())
                        {
                            // Get list of pdf files
                            QDir pdfDir(sPath);

                            QStringList sFormatList;
                            sFormatList.append( "*.pdf");

                            // Create file list
                            QStringList pdfFileList;
                            pdfFileList = pdfDir.entryList( sFormatList, QDir::Files | QDir::NoSymLinks, 0);

                            // If file list is empty retry with empty document ID
                            if (pdfFileList.isEmpty())
                            {
                                // Do nothing
                            }
                            else if (pdfFileList.count() == 1)
                            {
                                // Select file
                                sPath.append(pdfFileList.at(0));
                            }
                            // If more than 1 file is found
                            else
                            {
                                // Identify document ID number
                                QString sBookId = "";
                                // Identify document ID number
                                if ( sLink.contains(QString("?id=")) )
                                {
                                    int iStartIndex  = sLink.indexOf(QString("?id=")) + (int) 4;
                                    int iStopIndex   = sLink.indexOf(QString(";"), iStartIndex);
                                    sBookId = sLink.mid(iStartIndex,iStopIndex-iStartIndex);
                                }



                                QString sBookTitle = sTitle;
                                removeReservedCharFromString(&sBookTitle);
                                QString sFileName;

                                // Try to identify book by title and ID
                                for (int i=0; i < pdfFileList.count(); i++)
                                {
                                    sFileName = pdfFileList.at(i);
                                    sFileName.remove(".pdf");
                                    if (sFileName.length() > QString("book_full_view").length())
                                    {
                                        sFileName.remove("book_full_view");
                                        sFileName.remove("_full_view");
                                    }

                                    if (!sBookId.isEmpty() && sFileName.contains(sBookId))
                                    {
                                        sPath.append(pdfFileList.at(i));
                                        break;
                                    }
                                    else if (!sBookTitle.isEmpty() && sBookTitle.contains(sFileName))
                                    {
                                        sPath.append(pdfFileList.at(i));
                                        break;
                                    }
                                }
                            }
                        }

                        // Add to database
                        QString sQuery = "INSERT INTO " + mHistoryDbName + " (Date, Link, Title, Author, Published, Path) "
                                             + "VALUES (:Date, :Link, :Title, :Author, :Published, :Path)";
                        query.prepare(sQuery);
                        query.bindValue(":Date", sDate);
                        query.bindValue(":Link", sLink);
                        query.bindValue(":Title", sTitle);
                        query.bindValue(":Author", sAuthor);
                        query.bindValue(":Published", sPublished);
                        query.bindValue(":Path", sPath);
                        query.exec();
                    }
                }while ( iIdx > -1 );
                if(!query.exec("COMMIT"))
                    qDebug() << "HDH: Unable to commit hstr data to database - " << query.lastError();
            }
        }

        query.prepare("SELECT * FROM " + mHistoryDbName);
        if(!query.exec())
            qDebug() << "HDH: Unable to load hstr data from database - " << query.lastError();

        updateHistoryTableContent(&query);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: show user settings dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_userSettingsDialog()
{
    if (mpUserSettings == 0)
    {
        mpUserSettings = new QDialog;
        mpUserSettings->setWindowTitle("User settings");

        mpUserSettingsDialog = &tUserSettingsDialog;

        // Auto update
        mpUserSettingsDialog->groupBoxAutoUpdate    = new QGroupBox(tr("Auto update"));
        mpUserSettingsDialog->gridLayoutAutoUpdate  = new QGridLayout;

        mpUserSettingsDialog->checkBoxAutoUpdate    = new QCheckBox(tr("enable automatic update check on start-up"));
        mpUserSettingsDialog->gridLayoutAutoUpdate->addWidget(mpUserSettingsDialog->checkBoxAutoUpdate, 1,0, 1, 4);

        mpUserSettingsDialog->gridLayoutAutoUpdate->setColumnStretch(0, 1);
        mpUserSettingsDialog->gridLayoutAutoUpdate->setColumnStretch(1, 5);

        mpUserSettingsDialog->groupBoxAutoUpdate->setLayout(mpUserSettingsDialog->gridLayoutAutoUpdate);

        // Book information group box
        mpUserSettingsDialog->groupBoxBookInfo      = new QGroupBox(tr("Book information"));
        mpUserSettingsDialog->gridLayoutBookInfo    = new QGridLayout;

        mpUserSettingsDialog->checkBoxEnableProxy   = new QCheckBox(tr("use Proxy Server"));
        mpUserSettingsDialog->checkBoxEnableProxy->setToolTip(QString("enables HDH to download data via proxy server"));
        mpUserSettingsDialog->checkBoxEnableAutoProxyActivation = new QCheckBox(tr("enable automatic Proxy Server activation"));
        mpUserSettingsDialog->checkBoxEnableAutoProxyActivation->setToolTip("enables HDH to automatically activate proxy server connections to obtain book informations if required");
        mpUserSettingsDialog->pushButtonProxySetup  = new QPushButton("Proxy settings");
        mpUserSettingsDialog->gridLayoutBookInfo->addWidget(mpUserSettingsDialog->checkBoxEnableProxy, 1, 0);
        mpUserSettingsDialog->gridLayoutBookInfo->addWidget(mpUserSettingsDialog->checkBoxEnableAutoProxyActivation, 1, 2, 1, 1);
        mpUserSettingsDialog->gridLayoutBookInfo->addWidget(mpUserSettingsDialog->pushButtonProxySetup, 1, 5, 1, 1);

        mpUserSettingsDialog->gridLayoutBookInfo->setColumnStretch(0, 1);
        mpUserSettingsDialog->gridLayoutBookInfo->setColumnStretch(1, 1);
        mpUserSettingsDialog->gridLayoutBookInfo->setColumnStretch(2, 3);
        mpUserSettingsDialog->gridLayoutBookInfo->setColumnStretch(5, 1);

        mpUserSettingsDialog->groupBoxBookInfo->setLayout(mpUserSettingsDialog->gridLayoutBookInfo);

        // Download settings group box
        mpUserSettingsDialog->groupBoxDownloadSettings      = new QGroupBox(tr("Download settings"));
        mpUserSettingsDialog->gridLayoutDownloadSettings    = new QGridLayout;

        mpUserSettingsDialog->labelFormat     = new QLabel(tr("Download pages as "));
        mpUserSettingsDialog->radioButtonPdf  = new QRadioButton(tr("pdfs (1 pdf per page, searchable text,  watermark on left border,  max. 30 pages / 5 min)"));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->labelFormat, 1, 0);
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->radioButtonPdf, 1, 1, 1, 5);

        mpUserSettingsDialog->radioButtonImages   = new QRadioButton(tr("images (1 file per page, pdf generation possible, fast download), image zoom:"));
        mpUserSettingsDialog->comboBoxZoom        = new QComboBox(0);
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->radioButtonImages, 2, 1, 1, 5);
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->comboBoxZoom, 2, 4);
        connect(mpUserSettingsDialog->radioButtonImages , SIGNAL(toggled(bool)),mpUserSettingsDialog->comboBoxZoom , SLOT(setEnabled(bool)));
        mpUserSettingsDialog->comboBoxZoom->setEnabled(mtUserSettings.bDownloadImages);

        mpUserSettingsDialog->checkBoxGetOcr  = new QCheckBox(tr("download OCR text (required for creation of searchable pdfs from images)"));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->checkBoxGetOcr, 3, 1, 1, 5);

        mpUserSettingsDialog->checkDownloadWholeBook = new QCheckBox(tr("'Download whole book' as 1 pdf if available (public domain only)"));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->checkDownloadWholeBook, 4, 1, 1, 5);

        mpUserSettingsDialog->labelDstFolder      = new QLabel(tr("Destination folder: "));
        mpUserSettingsDialog->lineEditDstFolder   = new QLineEdit(0);
        mpUserSettingsDialog->toolButtonDstFolder = new QToolButton(0);
        mpUserSettingsDialog->toolButtonDstFolder->setText(tr("..."));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->labelDstFolder, 5, 0);
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->lineEditDstFolder, 5, 1, 1, 3);
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->toolButtonDstFolder, 5, 4, 1, 1);

        QLabel *pFolderNamingLabel = new QLabel(tr("Folder naming: "));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(pFolderNamingLabel, 6, 0);
        mpUserSettingsDialog->checkBoxAutoFolder = new QCheckBox(tr("automatically create download folder by: "));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->checkBoxAutoFolder, 6, 1, 1, 1);
        QButtonGroup *bg = new QButtonGroup();
        mpUserSettingsDialog->radioButtonAutoFolderByBookId = new QRadioButton(tr("book ID"));
        bg->addButton(mpUserSettingsDialog->radioButtonAutoFolderByBookId);
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->radioButtonAutoFolderByBookId, 6, 2, 1, 1);
        mpUserSettingsDialog->radioButtonAutoFolderByTitle = new QRadioButton(tr("book title"));
        bg->addButton(mpUserSettingsDialog->radioButtonAutoFolderByTitle);
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->radioButtonAutoFolderByTitle, 6, 3, 1, 1);
        mpUserSettingsDialog->lableSpinBox = new QLabel();
        mpUserSettingsDialog->lableSpinBox->setText("number of characters");
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->lableSpinBox, 6, 4, 1, 1);
        mpUserSettingsDialog->spinBoxTitleChars = new QSpinBox();
        mpUserSettingsDialog->spinBoxTitleChars->setMinimum(1);
        mpUserSettingsDialog->spinBoxTitleChars->setMaximum(255);
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->spinBoxTitleChars, 6, 5, 1, 1);

        QLabel *pDownloadOptionsLabel = new QLabel(tr("Download options: "));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(pDownloadOptionsLabel, 7, 0);

        mpUserSettingsDialog->checkBoxResume = new QCheckBox(tr("resume book download"));
        mpUserSettingsDialog->checkBoxResume->setToolTip(QString("Continue book download for the specified input data including checks for corrupted files."));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->checkBoxResume, 7, 1, 1, 1);//, Qt::AlignRight);

        mpUserSettingsDialog->checkBoxAutoProxy = new QCheckBox(tr("enable AutoProxy"));
        mpUserSettingsDialog->checkBoxAutoProxy->setToolTip(QString("enables HDH to automatically download data via proxy server"));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->checkBoxAutoProxy, 7, 2, 1, 1);//, Qt::AlignLeft);

        mpUserSettingsDialog->checkBoxWebProxy = new QCheckBox(tr("enable WebProxies"));
        mpUserSettingsDialog->checkBoxWebProxy->setToolTip(QString("enables HDH to automatically download via webproxy server"));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->checkBoxWebProxy, 7, 3, 1, 1);

        QLabel *pPdfActionsLabel = new QLabel(tr("Pdf actions: "));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(pPdfActionsLabel, 8, 0);
        mpUserSettingsDialog->checkBoxAutoPdf = new QCheckBox(tr("create PDF book after download"));
        mpUserSettingsDialog->checkBoxAutoPdf->setToolTip(QString("The PDF merge & conversion process will start automatically when the download is complete"));
        mpUserSettingsDialog->gridLayoutDownloadSettings->addWidget(mpUserSettingsDialog->checkBoxAutoPdf, 8, 1, 1, 1);

        mpUserSettingsDialog->gridLayoutDownloadSettings->setColumnStretch(0,1);
        mpUserSettingsDialog->gridLayoutDownloadSettings->setColumnStretch(1,5);
        mpUserSettingsDialog->gridLayoutDownloadSettings->setColumnStretch(2,10);
        mpUserSettingsDialog->gridLayoutDownloadSettings->setColumnStretch(3,5);
        mpUserSettingsDialog->gridLayoutDownloadSettings->setColumnStretch(4,5);
        mpUserSettingsDialog->gridLayoutDownloadSettings->setColumnStretch(5,1);

        mpUserSettingsDialog->groupBoxDownloadSettings->setLayout(mpUserSettingsDialog->gridLayoutDownloadSettings);

        // Pdf & conversion group box
        mpUserSettingsDialog->groupBoxMergeAndConversion                  = new QGroupBox(tr("PDF merge && conversion"));
        mpUserSettingsDialog->gridLayoutMergeAndConversion  = new QGridLayout;

        mpUserSettingsDialog->labelOutputOptions     = new QLabel(tr("Output options: "));
        mpUserSettingsDialog->radioButtonMergePdftk  = new QRadioButton(tr("merge pdfs"));
        mpUserSettingsDialog->gridLayoutMergeAndConversion->addWidget(mpUserSettingsDialog->labelOutputOptions, 1, 0);
        mpUserSettingsDialog->gridLayoutMergeAndConversion->addWidget(mpUserSettingsDialog->radioButtonMergePdftk, 1, 1,1,5);

        mpUserSettingsDialog->radioButtonMergeImages  = new QRadioButton(tr("convert && merge images to pdf book"));
        mpUserSettingsDialog->gridLayoutMergeAndConversion->addWidget(mpUserSettingsDialog->radioButtonMergeImages, 2, 1,1,2);

        mpUserSettingsDialog->radioButtonConvertImages    = new QRadioButton(tr("convert images to single pdf files"));
        mpUserSettingsDialog->gridLayoutMergeAndConversion->addWidget(mpUserSettingsDialog->radioButtonConvertImages, 3, 1,1,2);

        mpUserSettingsDialog->checkBoxConvertOcrOnly = new QCheckBox(tr("use plain text (ocr text) only"));
        mpUserSettingsDialog->gridLayoutMergeAndConversion->addWidget(mpUserSettingsDialog->checkBoxConvertOcrOnly, 4, 1,1,4);

        mpUserSettingsDialog->checkBoxKeepExistingFile = new QCheckBox(tr("Keep existing pdf book files with identical name (e.g. book, book_1, book_2 etc.)"));
        mpUserSettingsDialog->gridLayoutMergeAndConversion->addWidget(mpUserSettingsDialog->checkBoxKeepExistingFile, 5, 1,1,4);

        mpUserSettingsDialog->checkBoxDeleteFiles = new QCheckBox(tr("Delete downloaded pages after pdf book generation (viz. pdf files, images, ocr data)"));
        mpUserSettingsDialog->checkBoxDeleteFiles->setToolTip(QString("All downloaded files (pdfs, images, ocr data) will be deleted after pdf book generation expect generated pdf book file"));
        mpUserSettingsDialog->gridLayoutMergeAndConversion->addWidget(mpUserSettingsDialog->checkBoxDeleteFiles, 6, 1, 1, 4);

        mpUserSettingsDialog->gridLayoutMergeAndConversion->setColumnStretch(0,1);
        mpUserSettingsDialog->gridLayoutMergeAndConversion->setColumnStretch(1,5);
        mpUserSettingsDialog->gridLayoutMergeAndConversion->setColumnStretch(2,10);
        mpUserSettingsDialog->gridLayoutMergeAndConversion->setColumnStretch(3,5);
        mpUserSettingsDialog->gridLayoutMergeAndConversion->setColumnStretch(4,5);
        mpUserSettingsDialog->gridLayoutMergeAndConversion->setColumnStretch(5,5);
        mpUserSettingsDialog->gridLayoutMergeAndConversion->setColumnStretch(6,5);

        mpUserSettingsDialog->groupBoxMergeAndConversion->setLayout(mpUserSettingsDialog->gridLayoutMergeAndConversion);

        QPushButton* pPushButtonOK = new QPushButton(QString("OK"));
        QPushButton* pPushButtonCancel = new QPushButton(QString("Cancel"));
        QPushButton* pPushButtonDefault = new QPushButton(QString("Default"));
        QHBoxLayout *pHLayout = new QHBoxLayout;
        pHLayout->setAlignment(Qt::AlignRight);
        pHLayout->addWidget(pPushButtonOK);
        pHLayout->addWidget(pPushButtonCancel);
        pHLayout->addWidget(pPushButtonDefault);

        // Book information group box
        QVBoxLayout *pVLayout = new QVBoxLayout;
        pVLayout->addWidget(mpUserSettingsDialog->groupBoxAutoUpdate);
        pVLayout->addWidget(mpUserSettingsDialog->groupBoxBookInfo);
        pVLayout->addWidget(mpUserSettingsDialog->groupBoxDownloadSettings);
        pVLayout->addWidget(mpUserSettingsDialog->groupBoxMergeAndConversion);
        pVLayout->addLayout(pHLayout);

        mpUserSettings->setLayout(pVLayout);
        mpUserSettings->setModal(true);
        connect(pPushButtonOK, SIGNAL(clicked()), mpUserSettings, SLOT(accept()));
        connect(pPushButtonCancel, SIGNAL(clicked()), mpUserSettings, SLOT(reject()));
        connect(pPushButtonDefault, SIGNAL(clicked()), this, SLOT(slot_defaultSettings()));

        // Add items to combo boxes
        QStringList sComboListItems;
        for (int i = 0; i < mpviDpi->count(); i++)
        {
            sComboListItems.append(QString("%1% (%2dpi)").arg(mpviZoomlevel->at(i)).arg(mpviDpi->at(i)));
        }
        mpUserSettingsDialog->comboBoxZoom->addItems(sComboListItems);

        // Create dependencies
        connect(mpUserSettingsDialog->radioButtonImages, SIGNAL(toggled(bool)), mpUserSettingsDialog->checkBoxGetOcr, SLOT(setChecked(bool)));
        connect(mpUserSettingsDialog->radioButtonImages, SIGNAL(toggled(bool)), mpUserSettingsDialog->comboBoxZoom, SLOT(setEnabled(bool)));
        connect(mpUserSettingsDialog->radioButtonImages, SIGNAL(toggled(bool)), mpUserSettingsDialog->comboBoxZoom, SLOT(setEnabled(bool)));
        connect(mpUserSettingsDialog->radioButtonImages, SIGNAL(toggled(bool)), mpUserSettingsDialog->radioButtonMergePdftk, SLOT(setDisabled(bool)));
        connect(mpUserSettingsDialog->radioButtonImages, SIGNAL(toggled(bool)), mpUserSettingsDialog->radioButtonMergeImages, SLOT(setChecked(bool)));
        connect(mpUserSettingsDialog->radioButtonMergePdftk, SIGNAL(toggled(bool)), mpUserSettingsDialog->checkBoxConvertOcrOnly, SLOT(setDisabled(bool)));
        connect(mpUserSettingsDialog->radioButtonPdf, SIGNAL(toggled(bool)), mpUserSettingsDialog->radioButtonConvertImages, SLOT(setDisabled(bool)));
        connect(mpUserSettingsDialog->radioButtonPdf, SIGNAL(toggled(bool)), mpUserSettingsDialog->radioButtonMergeImages, SLOT(setDisabled(bool)));
        connect(mpUserSettingsDialog->radioButtonPdf, SIGNAL(toggled(bool)), mpUserSettingsDialog->radioButtonMergePdftk, SLOT(setChecked(bool)));
        connect(mpUserSettingsDialog->toolButtonDstFolder, SIGNAL(clicked()), this, SLOT(slot_toolButtonDstFolder_clicked()));
        connect(mpUserSettingsDialog->pushButtonProxySetup, SIGNAL(pressed()), this, SLOT(slot_proxySetupDialog()));
        connect(mpUserSettingsDialog->checkBoxAutoProxy, SIGNAL(toggled(bool)), mpUserSettingsDialog->checkBoxWebProxy, SLOT(setDisabled(bool)));
        connect(mpUserSettingsDialog->checkBoxWebProxy, SIGNAL(toggled(bool)), mpUserSettingsDialog->checkBoxAutoProxy, SLOT(setDisabled(bool)));
        connect(mpUserSettingsDialog->checkBoxAutoFolder, SIGNAL(toggled(bool)), mpUserSettingsDialog->radioButtonAutoFolderByTitle, SLOT(setEnabled(bool)));
        connect(mpUserSettingsDialog->checkBoxAutoFolder, SIGNAL(toggled(bool)), mpUserSettingsDialog->radioButtonAutoFolderByBookId, SLOT(setEnabled(bool)));
        connect(mpUserSettingsDialog->checkBoxAutoFolder, SIGNAL(toggled(bool)), mpUserSettingsDialog->spinBoxTitleChars, SLOT(setEnabled(bool)));
        connect(mpUserSettingsDialog->radioButtonAutoFolderByBookId, SIGNAL(toggled(bool)), mpUserSettingsDialog->spinBoxTitleChars, SLOT(setDisabled(bool)));
        connect(mpUserSettingsDialog->radioButtonAutoFolderByTitle, SIGNAL(toggled(bool)), mpUserSettingsDialog->spinBoxTitleChars, SLOT(setEnabled(bool)));
        connect(mpUserSettingsDialog->radioButtonAutoFolderByTitle, SIGNAL(toggled(bool)), mpUserSettingsDialog->spinBoxTitleChars, SLOT(setEnabled(bool)));
        connect(mpUserSettingsDialog->lineEditDstFolder, SIGNAL(editingFinished()), this, SLOT(on_lineEditDstFolderUserSettings_editingFinished()));
    }

    /* Apply user settings */
    mpUserSettingsDialog->checkBoxAutoUpdate->setChecked(mtUserSettings.bAutoUpdate);
    mpUserSettingsDialog->checkBoxEnableProxy->setChecked(mtUserSettings.bUseProxy);
    mpUserSettingsDialog->checkBoxEnableAutoProxyActivation->setChecked(mtUserSettings.bEnableAutoProxyActivation);
    mpUserSettingsDialog->radioButtonPdf->setChecked(mtUserSettings.bDownloadPdfs);
    mpUserSettingsDialog->radioButtonImages->setChecked(mtUserSettings.bDownloadImages);
    mpUserSettingsDialog->checkBoxGetOcr->setChecked(mtUserSettings.bDownloadOcr);
    mpUserSettingsDialog->checkDownloadWholeBook->setChecked(mtUserSettings.bDownloadWholeBook);
    mpUserSettingsDialog->lineEditDstFolder->setText(mtUserSettings.sDestinationFolder);
    mpUserSettingsDialog->checkBoxAutoFolder->setChecked(mtUserSettings.bCreateDownloadFolder);
    mpUserSettingsDialog->radioButtonAutoFolderByTitle->setEnabled(mtUserSettings.bCreateDownloadFolder);
    mpUserSettingsDialog->radioButtonAutoFolderByBookId->setEnabled(mtUserSettings.bCreateDownloadFolder);
    mpUserSettingsDialog->radioButtonAutoFolderByTitle->setChecked(mtUserSettings.bCreateDownloadFolderByTitle);
    mpUserSettingsDialog->radioButtonAutoFolderByBookId->setChecked(!mtUserSettings.bCreateDownloadFolderByTitle);
    mpUserSettingsDialog->spinBoxTitleChars->setEnabled(mtUserSettings.bCreateDownloadFolder && mtUserSettings.bCreateDownloadFolderByTitle);
    mpUserSettingsDialog->spinBoxTitleChars->setValue(mtUserSettings.iTitleChars);
    mpUserSettingsDialog->checkBoxAutoPdf->setChecked(mtUserSettings.bAutoPdfBook);
    mpUserSettingsDialog->checkBoxResume->setChecked(mtUserSettings.bResume);
    mpUserSettingsDialog->checkBoxAutoProxy->setChecked(mtUserSettings.bEnableAutoProxy);
    mpUserSettingsDialog->checkBoxWebProxy->setChecked(mtUserSettings.bEnableWebProxy);
    mpUserSettingsDialog->radioButtonMergePdftk->setChecked(mtUserSettings.bMergePdf);
    mpUserSettingsDialog->radioButtonMergeImages->setChecked(mtUserSettings.bImage2Book);
    mpUserSettingsDialog->radioButtonConvertImages->setChecked(mtUserSettings.bImage2Pdf);
    mpUserSettingsDialog->checkBoxConvertOcrOnly->setChecked(mtUserSettings.bOcrOnly);
    mpUserSettingsDialog->checkBoxKeepExistingFile->setChecked(mtUserSettings.bKeepExistingFile);
    mpUserSettingsDialog->checkBoxDeleteFiles->setChecked(mtUserSettings.bDeleteDownloadedPageData);
    mpUserSettingsDialog->comboBoxZoom->setCurrentIndex(mtUserSettings.iZoom);

    if(mpUserSettings->exec() == QDialog::Accepted)
    {
        // Auto update
        mtUserSettings.bAutoUpdate          = mpUserSettingsDialog->checkBoxAutoUpdate->isChecked();
        mpAutoUpdateAct->setChecked(mtUserSettings.bAutoUpdate);

        // Book info
        mtUserSettings.bUseProxy                    = mpUserSettingsDialog->checkBoxEnableProxy->isChecked();
        mtUserSettings.bEnableAutoProxyActivation   = mpUserSettingsDialog->checkBoxEnableAutoProxyActivation->isChecked();

        // Download settings
        mtUserSettings.bDownloadPdfs        = mpUserSettingsDialog->radioButtonPdf->isChecked();
        mtUserSettings.bDownloadImages      = mpUserSettingsDialog->radioButtonImages->isChecked();
        mtUserSettings.bDownloadOcr         = mpUserSettingsDialog->checkBoxGetOcr->isChecked();
        mtUserSettings.bDownloadWholeBook   = mpUserSettingsDialog->checkDownloadWholeBook->isChecked();
        mtUserSettings.iZoom                = mpUserSettingsDialog->comboBoxZoom->currentIndex();
        mtUserSettings.sDestinationFolder   = mpUserSettingsDialog->lineEditDstFolder->text();
        mtUserSettings.bCreateDownloadFolder = mpUserSettingsDialog->checkBoxAutoFolder->isChecked();
        mtUserSettings.bCreateDownloadFolderByTitle = mpUserSettingsDialog->radioButtonAutoFolderByTitle->isChecked();
        mtUserSettings.iTitleChars          = mpUserSettingsDialog->spinBoxTitleChars->value();
        mtUserSettings.bResume              = mpUserSettingsDialog->checkBoxResume->isChecked();
        mtUserSettings.bEnableWebProxy      = mpUserSettingsDialog->checkBoxWebProxy->isChecked();
        mtUserSettings.bEnableAutoProxy     = mpUserSettingsDialog->checkBoxAutoProxy->isChecked();
        mtUserSettings.bAutoPdfBook         = mpUserSettingsDialog->checkBoxAutoPdf->isChecked();

        // Pdf conversion
        mtUserSettings.bMergePdf            = mpUserSettingsDialog->radioButtonMergePdftk->isChecked();
        mtUserSettings.bImage2Book          = mpUserSettingsDialog->radioButtonMergeImages->isChecked();
        mtUserSettings.bImage2Pdf           = mpUserSettingsDialog->radioButtonConvertImages->isChecked();
        mtUserSettings.bOcrOnly             = mpUserSettingsDialog->checkBoxConvertOcrOnly->isChecked();
        mtUserSettings.bKeepExistingFile    = mpUserSettingsDialog->checkBoxKeepExistingFile->isChecked();
        mtUserSettings.bDeleteDownloadedPageData = mpUserSettingsDialog->checkBoxDeleteFiles->isChecked();



        slot_saveGuiSettings();
        setUserSettings();
    }
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: apply default settings
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_defaultSettings()
{
    /* Apply default settings */
    mpUserSettingsDialog->checkBoxAutoUpdate->setChecked(mtDefaultSettings.bAutoUpdate);
    mpUserSettingsDialog->checkBoxEnableProxy->setChecked(mtDefaultSettings.bUseProxy);
    mpUserSettingsDialog->checkBoxEnableAutoProxyActivation->setChecked(mtDefaultSettings.bEnableAutoProxyActivation);
    mpUserSettingsDialog->radioButtonPdf->setChecked(mtDefaultSettings.bDownloadPdfs);
    mpUserSettingsDialog->radioButtonImages->setChecked(mtDefaultSettings.bDownloadImages);
    mpUserSettingsDialog->checkBoxGetOcr->setChecked(mtDefaultSettings.bDownloadOcr);
    mpUserSettingsDialog->checkDownloadWholeBook->setChecked(mtDefaultSettings.bDownloadWholeBook);
    mpUserSettingsDialog->lineEditDstFolder->setText(mtDefaultSettings.sDestinationFolder);
    mpUserSettingsDialog->checkBoxAutoFolder->setChecked(mtDefaultSettings.bCreateDownloadFolder);
    mpUserSettingsDialog->radioButtonAutoFolderByTitle->setChecked(mtDefaultSettings.bCreateDownloadFolderByTitle);
    mpUserSettingsDialog->spinBoxTitleChars->setValue(mtDefaultSettings.iTitleChars);
    mpUserSettingsDialog->radioButtonAutoFolderByBookId->setChecked(!mtDefaultSettings.bCreateDownloadFolderByTitle);
    mpUserSettingsDialog->checkBoxAutoPdf->setChecked(mtDefaultSettings.bAutoPdfBook);
    mpUserSettingsDialog->checkBoxResume->setChecked(mtDefaultSettings.bResume);
    mpUserSettingsDialog->checkBoxWebProxy->setChecked(mtDefaultSettings.bEnableWebProxy);
    mpUserSettingsDialog->checkBoxAutoProxy->setChecked(mtDefaultSettings.bEnableAutoProxy);
    mpUserSettingsDialog->radioButtonMergePdftk->setChecked(mtDefaultSettings.bMergePdf);
    mpUserSettingsDialog->radioButtonMergeImages->setChecked(mtDefaultSettings.bImage2Book);
    mpUserSettingsDialog->radioButtonConvertImages->setChecked(mtDefaultSettings.bImage2Pdf);
    mpUserSettingsDialog->checkBoxConvertOcrOnly->setChecked(mtDefaultSettings.bOcrOnly);
    mpUserSettingsDialog->checkBoxKeepExistingFile->setChecked(mtDefaultSettings.bKeepExistingFile);
    mpUserSettingsDialog->checkBoxDeleteFiles->setChecked(mtDefaultSettings.bDeleteDownloadedPageData);
    mpUserSettingsDialog->comboBoxZoom->setCurrentIndex(mtDefaultSettings.iZoom);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: init GUI elements
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::initGuiComponents()
{
    loadDefaultSettings();

    getXmlGuiSetup();

    mpWebProxyNotification = 0;
    if (!mtUserSettings.bWebProxyNoteAccepted)
        createNotificationDialog();

    createActions();
    createMenus();

    createBookInfoGroupBox();
    createDownloadSettingsGroupBox();
    createMergeAndConversionGroupBox();

    QHBoxLayout* pExitBox   = new QHBoxLayout;
    ui->pushButtonExit      = new QPushButton(tr("Exit"));
    pExitBox->addWidget(ui->pushButtonExit);
    pExitBox->setAlignment(Qt::AlignRight);
    connect(ui->pushButtonExit, SIGNAL(clicked()),this,SLOT(on_pushButtonExit_clicked()));

    // Main tab
    QVBoxLayout* pVLayoutMainTab   = new QVBoxLayout;
    pVLayoutMainTab->addWidget(ui->groupBoxBookInfo);
    pVLayoutMainTab->addWidget(ui->groupBoxDownloadSettings);
    pVLayoutMainTab->addWidget(ui->groupBoxMergeAndConversion);
    ui->mainTabWidget = new QWidget;
    ui->mainTabWidget->setLayout(pVLayoutMainTab);

    // Batch job tab
    ui->batchJobTabWidget = new CreateBatchJob(this, &mtUserSettings);
    connectSignalsToCreateBatchJob();

    // Link collector
    ui->linkCollectorTabWidget = new QWidget;
    ui->linkCollectorTabWidget->setLayout(createLinkCollectorTab());

    // History tab
    ui->historyTabWidget = new QWidget;
    ui->historyTabWidget->setLayout(createHistoryTab());
#if USE_MENUS_IN_TABS
    // Settings tab
    ui->settingsTabWidget = new QWidget;

    // Settings tab
    ui->toolsTabWidget = new QWidget;

    // Settings tab
    ui->aboutTabWidget = new QWidget;

    QTabWidget *pInnerTab = new QTabWidget();
    QGridLayout *pInnerLayout = new QGridLayout();
    pInnerLayout->addWidget(pInnerTab);
    ui->settingsTabWidget->setLayout(pInnerLayout);
    QGridLayout *pPageSetupLayout = new QGridLayout();
#endif
    // Tab widget
    ui->tabWidget = new QTabWidget;
    ui->tabWidget->addTab(ui->mainTabWidget,tr("Main"));
    ui->tabWidget->addTab(ui->batchJobTabWidget,tr("Batch job"));//Status
    ui->tabWidget->addTab(ui->linkCollectorTabWidget,tr("Link Collector"));
    ui->tabWidget->addTab(ui->historyTabWidget,tr("History DB"));
#if USE_MENUS_IN_TABS
    ui->tabWidget->addTab(ui->settingsTabWidget,tr("Options"));
    pInnerTab->addTab(ui->toolsTabWidget,tr("Page setup"));
    pInnerTab->addTab(ui->aboutTabWidget,tr("Gui setup"));
#endif

    connect(ui->batchJobTabWidget, SIGNAL(numberOpenJobs(int)), this, SLOT(slot_updateBatchJobTabTitle(int)));
    ui->batchJobTabWidget->numberOpenJobs();

    // Scroll area
    ui->scrollArea = new QScrollArea;
    ui->scrollArea->setWidget(ui->tabWidget);
    ui->scrollArea->setWidgetResizable(true);

    // Central Widget layout
    QVBoxLayout* pVWidgetLayout   = new QVBoxLayout;
    pVWidgetLayout->addWidget(ui->scrollArea);
    pVWidgetLayout->addLayout(pExitBox);
    this->setCentralWidget(new QWidget);
    this->centralWidget()->setLayout(pVWidgetLayout);
    this->setAttribute(Qt::WA_WindowPropagation,true);
    centralWidget()->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    // Set default style
    qApp->setStyle(QStyleFactory::create(mtDefaultGuiSettings.sStyle));
    qApp->setFont(QFont(mtDefaultGuiSettings.sFontFamily,mtDefaultGuiSettings.iFontPointSize));
    this->setFont(QFont(mtDefaultGuiSettings.sFontFamily,mtDefaultGuiSettings.iFontPointSize));

    // Check default style
    QRegExp regExp(".(.*)\\+?Style");
    msDefaultStyle = qApp->style()->metaObject()->className();
    if (regExp.exactMatch(msDefaultStyle))
        msDefaultStyle = regExp.cap(1);
    mDefaultFont = this->font();
    mtDefaultGuiSettings.sFontFamily    = mDefaultFont.family();
    mtDefaultGuiSettings.iFontPointSize = mDefaultFont.pointSize();

    ui->labelBookTitleText->setWordWrap(true);
    ui->lineEditBookURL->setText("");
    ui->lineEditBookURL->setPlaceholderText(QString("Enter Hathitrust.org book URL or book ID here"));
    ui->radioButtonImages->setFocus();

    // set logo
    setGuiLogo();

    // Setup folders
    ui->lineEditDstFolder->setText(mtUserSettings.sDestinationFolder);
    ui->pushButtonStartDownload->setEnabled(false);

    ui->lineEditSrcFolder->setText(mtUserSettings.sDestinationFolder);

    // Setup progress bars
    ui->progressBarBookPage->setValue(0);
    ui->labelBookInfoState->setText(QString(""));

    ui->progressBarDownload->setValue(0);
    ui->labelDownloadState->setText(QString(""));

    ui->progressBarCreatePdf->setValue(0);
    ui->labelCreatePdfState->setText(QString(""));

    // Create comboBox entries for zoom level
    mpviZoomlevel = new QVector <int>;
    mpviZoomlevel->append(50);
    mpviZoomlevel->append(75);
    mpviZoomlevel->append(100);
    mpviZoomlevel->append(125);
    mpviZoomlevel->append(150);
    mpviZoomlevel->append(175);
    mpviZoomlevel->append(200);
    mpviZoomlevel->append(300);
    mpviZoomlevel->append(400);

    // Create vector with parameter values for zoom level
    mpviDpi = new QVector <int>;
    mpviDpi->append(48);
    mpviDpi->append(72);
    mpviDpi->append(96);
    mpviDpi->append(120);
    mpviDpi->append(144);
    mpviDpi->append(168);
    mpviDpi->append(192);
    mpviDpi->append(288);
    mpviDpi->append(384);

    // Add items to combo boxes
    QStringList sComboListItems;
    for (int i = 0; i < mpviDpi->count(); i++)
    {
        sComboListItems.append(QString("%1% (%2dpi)").arg(mpviZoomlevel->at(i)).arg(mpviDpi->at(i)));
    }
    ui->comboBoxZoom->addItems(sComboListItems);
    ui->comboBoxZoom->setCurrentIndex(mtUserSettings.iZoom);
    ui->comboBoxPrinterRes->addItems(sComboListItems);
    ui->comboBoxPrinterRes->setCurrentIndex(mtUserSettings.iZoom);

    connect(ui->comboBoxZoom,       SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBoxZoom_currentIndexChanged(int)));
    connect(ui->comboBoxPrinterRes, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBoxPrinterRes_currentIndexChanged(int)));

    adjustSize();
    slot_adjustGuiGrid();

    mpUserSettings = 0;
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: load default GUI elements and user settings
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::loadDefaultSettings()
{
    // GUI
    mtDefaultGuiSettings.bFontBold          = false;
    mtDefaultGuiSettings.bFontItalic        = false;
    mtDefaultGuiSettings.bFontStrikeout     = false;
    mtDefaultGuiSettings.bFontUnderline     = false;
    mtDefaultGuiSettings.iFontPointSize     = (int) 8;
    mtDefaultGuiSettings.sFontFamily        = QString("Arial");
    mtDefaultGuiSettings.sStyle             = QString("Windows Vista");

    // auto update
    mtDefaultSettings.bAutoUpdate           = true;
    // Book info
    mtDefaultSettings.bUseProxy             = false;
#if DISABLE_AUTOPROXY
    mtDefaultSettings.bEnableAutoProxyActivation = false;
#else
    mtDefaultSettings.bEnableAutoProxyActivation = true;
#endif
    // Download settings
    mtDefaultSettings.bDownloadPdfs        = true;
    mtDefaultSettings.bDownloadImages      = false;
    mtDefaultSettings.bDownloadOcr         = false;
    mtDefaultSettings.bDownloadWholeBook   = true;
    mtDefaultSettings.iZoom                = 6;
    mtDefaultSettings.sDestinationFolder   = QDir::homePath() + QString("/Downloads/");
    mtDefaultSettings.bCreateDownloadFolder = false;
    mtDefaultSettings.bCreateDownloadFolderByTitle = false;
    mtDefaultSettings.iTitleChars          = 20;
    mtDefaultSettings.bResume              = false;
    mtDefaultSettings.bEnableWebProxy      = false;
    mtDefaultSettings.bEnableAutoProxy     = false;
    mtDefaultSettings.bAutoPdfBook         = true;

    // Pdf conversion
    mtDefaultSettings.bMergePdf            = true;
    mtDefaultSettings.bImage2Book          = false;
    mtDefaultSettings.bImage2Pdf           = false;
    mtDefaultSettings.bOcrOnly             = false;
    mtDefaultSettings.bKeepExistingFile    = false;
    mtDefaultSettings.bDeleteDownloadedPageData = false;

    // WebProxy note accepted
    mtDefaultSettings.bWebProxyNoteAccepted = false;

    // Fill user settings
    mtUserGuiSettings   = mtDefaultGuiSettings;
    mtUserSettings      = mtDefaultSettings;
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: set GUI logos
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::setGuiLogo()
{
    QDate date = QDate::currentDate();

    // Use Halloween images from YYYY:10:31 to YYYY:11:01
    if ( ((date.month() == (int) 10) && (date.day() == (int) 31)) || ((date.month() == (int) 11) && (date.day() == (int) 1)) )
    {
        // Setup halloween elephant images
        ui->mpMovieRead = new QMovie(":/images/elephant_reading_hlwn.gif");
        ui->labelLogoReading->setMovie(ui->mpMovieRead);
        ui->mpMovieRead->jumpToFrame(0);
        ui->labelLogoReading->setToolTip(QString("Happy Halloween!"));

        ui->mpMovieLeech = new QMovie(":/images/elephant_leeching_hlwn.gif");
        ui->labelLogoLeeching->setMovie(ui->mpMovieLeech);
        ui->mpMovieLeech->jumpToFrame(0);
        ui->labelLogoLeeching->setToolTip(QString("Happy Halloween!"));

        ui->mpMovieMerge = new QMovie(":/images/elephant_merging_hlwn.gif");
        ui->labelLogoMerging->setMovie(ui->mpMovieMerge);
        ui->mpMovieMerge->jumpToFrame(0);
        ui->labelLogoMerging->setToolTip(QString("Happy Halloween!"));
    }
    // Use Christmas images from YYYY:12:24 to YYYY:12:31
    else if ( ((date.month() == (int) 12) && ( date.day() >= (int) 24)) )
    {
        // Setup elephant images
        ui->mpMovieRead = new QMovie(":/images/elephant_reading_xms.gif");
        ui->labelLogoReading->setMovie(ui->mpMovieRead);
        ui->mpMovieRead->jumpToFrame(0);
        ui->labelLogoReading->setToolTip(QString("We wish you..."));

        ui->mpMovieLeech = new QMovie(":/images/elephant_leeching_xms.gif");
        ui->labelLogoLeeching->setMovie(ui->mpMovieLeech);
        ui->mpMovieLeech->jumpToFrame(0);
        ui->labelLogoLeeching->setToolTip(QString("...a merry Christmas..."));

        ui->mpMovieMerge = new QMovie(":/images/elephant_merging_xms.gif");
        ui->labelLogoMerging->setMovie(ui->mpMovieMerge);
        ui->mpMovieMerge->jumpToFrame(0);
        ui->labelLogoMerging->setToolTip(QString("...and a Happy New Year!"));
    }
    else
    {
        // Setup elephant images
        ui->mpMovieRead = new QMovie(":/images/elephant_reading.gif");
        ui->labelLogoReading->setMovie(ui->mpMovieRead);
        ui->mpMovieRead->jumpToFrame(0);
        ui->labelLogoReading->setToolTip(QString("Reading book information..."));

        ui->mpMovieLeech = new QMovie(":/images/elephant_leeching.gif");
        ui->labelLogoLeeching->setMovie(ui->mpMovieLeech);
        ui->mpMovieLeech->jumpToFrame(0);
        ui->labelLogoLeeching->setToolTip(QString("Leeching book pages..."));

        ui->mpMovieMerge = new QMovie(":/images/elephant_merging.gif");
        ui->labelLogoMerging->setMovie(ui->mpMovieMerge);
        ui->mpMovieMerge->jumpToFrame(0);
        ui->labelLogoMerging->setToolTip(QString("Merging pdf files..."));
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: create notification dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::createNotificationDialog()
{
    if (mpWebProxyNotification == 0)
    {
        mpWebProxyNotification = new QDialog ();
        QVBoxLayout *pVBoxLayoutGroupBox    = new QVBoxLayout;
        QVBoxLayout *pVBoxLayout            = new QVBoxLayout;
        QHBoxLayout *pHBoxLayout            = new QHBoxLayout;

        QGroupBox *pGroupBoxNotification    = new QGroupBox(tr("Notification"));
        QLabel *pLabelPurpose = new QLabel();
        pLabelPurpose->setText(QString("<b>Purpose:</b><br>&bull; Enables the user to bypass the queuing time for pdf and image downloads."));
        pVBoxLayoutGroupBox->addWidget(pLabelPurpose);

        QLabel *pLabelRestrictions = new QLabel();
        pLabelRestrictions->setText(QString("<b>Restrictions:</b><br>&bull; Works only for non-restricted books which are also public domain when viewed outside the US.<br>&bull; Strongly varying download speed."));
        pVBoxLayoutGroupBox->addWidget(pLabelRestrictions);

        QLabel *pLabelHint = new QLabel();
        pLabelHint->setText(QString("<b>Important advice:</b><br>&bull; Since this feature utilizes a large number of random webpages<br> &nbsp;&nbsp;an updated virus scanner is recommented.<br>&bull; There is no guarantee for proper functioning.<br>For more information please see help file."));
        pVBoxLayoutGroupBox->addWidget(pLabelHint);

        pGroupBoxNotification->setLayout(pVBoxLayoutGroupBox);

        pVBoxLayout->addWidget(pGroupBoxNotification);

        QCheckBox *pAcceptBox = new QCheckBox();
        pAcceptBox->setText(QString("I have read this notification and like to activate the webproxy feature."));
        pVBoxLayout->addWidget(pAcceptBox);

        QSpacerItem* spacer1                = new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding );
        pHBoxLayout->addItem(spacer1);

        QPushButton *pPushButtonOk          = new QPushButton(QString("Ok"));
        pPushButtonOk->setDisabled(true);
        connect(pAcceptBox, SIGNAL(clicked(bool)), pPushButtonOk, SLOT(setEnabled(bool)));
        connect(pPushButtonOk, SIGNAL(clicked()), mpWebProxyNotification, SLOT(accept()));
        pHBoxLayout->addWidget(pPushButtonOk);

        QPushButton *pPushButtonCancel      = new QPushButton(QString("Cancel"));
        connect(pPushButtonCancel, SIGNAL(clicked()), mpWebProxyNotification, SLOT(reject()));
        pHBoxLayout->addWidget(pPushButtonCancel);

        pVBoxLayout->addLayout(pHBoxLayout);

        mpWebProxyNotification->setLayout(pVBoxLayout);
        mpWebProxyNotification->setFixedSize(mpWebProxyNotification->minimumSize());
        mpWebProxyNotification->setWindowFlags( windowFlags() | Qt::WindowMinimizeButtonHint);
    }
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: Set new fixed size of GUI on resize event
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::resizeEvent ( QResizeEvent * /*event - suppress unused warning*/ )
{
    //this->setFixedSize( this->minimumSize());  // comment this line to achieve a free resizable GUI
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: On Change event - WORKARROUND FOR QAPPLICATION::ALERT() BUG
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::changeEvent  ( QEvent  *event )
{
    // There seems to be abug in QApplication::alert().
    // It should ["...Cause(s) an alert to be shown for widget if the window is not the active window."]
    // But it just works the other way arround.
    // Further using the minize icon in the title bar seems to be different to minimizing the application
    // by clicking the icon on the task bar: the window is dreactivated.
    // Therefore the windows in activated manually here.

    if( event->type() == QEvent::WindowStateChange )
    {
        if (QMainWindow::isMinimized() && !QMainWindow::isActiveWindow())
        {
            // WORKARROUND: setting the window activated when the application is minimized enables
            // QApplication::alert() (which does not comply with Qt documentation)
            this->activateWindow();
        }
    }

#if defined(Q_OS_MAC)
    if (mpClipboardTimer != 0)
    {
        // WORKARROUND: for clipboard events when application has no focus:
        // On Mac clipboard changes will not be noticed when application is not active window.
        // Thus link collector would not work properly
        if (this->isActiveWindow()){
            // Application has focus
            mpClipboardTimer->stop();
        }
        else
        {
            // Application don't has focus
            mpClipboardTimer->start();
        }
    }
#endif
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: adjust column grid size of GUI elements
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_adjustGuiGrid()
{
    int iMaxWidth = -1;
    QList<int> iColWidth;

    for( int i = 0; i < 6; i++ )
    {
        iMaxWidth = ui->gridLayoutBookInfo->cellRect(0,i).width() ;

        if (iMaxWidth < ui->gridLayoutDownloadSettings->cellRect(0,i).width())
            iMaxWidth =  ui->gridLayoutDownloadSettings->cellRect(0,i).width();
        if (iMaxWidth < ui->gridLayoutMergeAndConversion->cellRect(0,i).width())
            iMaxWidth =  ui->gridLayoutMergeAndConversion->cellRect(0,i).width();

        iColWidth.append(iMaxWidth);
    }

    for( int i = 5; i >=0; i-- )
    {
        ui->gridLayoutMergeAndConversion->setColumnMinimumWidth(i,iColWidth.at(i));
        ui->gridLayoutBookInfo->setColumnMinimumWidth(i,iColWidth.at(i));
        ui->gridLayoutDownloadSettings->setColumnMinimumWidth(i,iColWidth.at(i));
    }
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: Update progress bar
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_updateDownloadProgressBar(int iPage, int iMaxPage)
{
    ui->progressBarDownload->setValue(iPage);
    ui->progressBarDownload->setMaximum(iMaxPage);

    if (mBatchJobActivated)
    {
        if ((iPage >= 0) && (iMaxPage != 0))
        {
            emit updateProgressbar((int) ( iPage * 100 )/iMaxPage);
        }
        else
        {
            emit updateProgressbar(iPage);
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Update download state
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_updateDownloadState(QString sState)
{
    ui->labelDownloadState->setText(sState);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Update webproxy state
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_updateWebProxyState(QString sState)
{
    ui->labelWebProxyState->setText(sState);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Connect radiobutton 'pdf' with merge pdf option
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_radioButtonPdf_clicked()
{
    // Deselect checkbox for ocr download
    ui->checkBoxGetOcr->setChecked(false);
    // Set 'merge pdfs' option
    ui->radioButtonMergePdftk->setChecked(true);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Connect radiobutton 'images' with ocr and merge images gui elements
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_radioButtonImages_clicked()
{
    // Select checkbox for ocr download
    ui->checkBoxGetOcr->setChecked(true);
    // Set 'convert images'
    ui->radioButtonMergeImages->setChecked(true);
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: On checkbox resume download toggled
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_checkBoxResume(bool bState)
{
    mpFileDownloader->setResumeDownloadEnabled(bState);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Activate /  Deactivate proxy server
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_checkBoxUseProxy_clicked(bool)
{
    if (ui->pushButtonGetInfo->text().contains("cancel", Qt::CaseInsensitive))
        slot_pushButtonGetInfoCancel_clicked();

    if (ui->checkBoxUseProxy->isChecked())
    {
        // Open proxy dialog, if none proxy is given and manual proxy is set
        if ((!mpProxyDialog->getAutoProxy()) && (mpProxyDialog->getProxyIp().isEmpty()) )
        {
            if( mpProxyDialog->exec() )
            {
                // Check if none proxy is given and manual proxy is set
                if ( (!mpProxyDialog->getAutoProxy()) && (mpProxyDialog->getProxyIp().isEmpty()) )
                {
                    ui->checkBoxUseProxy->setChecked(false);
                    ui->labelBookInfoState->setText(QString(""));

                    return;
                }
            }
            else
            {
                // Canceled by user
                ui->checkBoxUseProxy->setChecked(false);
                ui->labelBookInfoState->setText(QString(""));

                return;
            }
        }

        // Change button
        slot_enablePushButtonGetBookInfo(false);

        mpAutoProxy->enableLocalHost(false);

        if (mpProxyDialog->getAutoProxy())
            mpAutoProxy->applyProxyServer();
        else
        {
            // Apply user proxy settings
            mpAutoProxy->applyProxyServer(mpUserProxy);
        }
    }
    else
    {
        // Deactivate proxy
        mpAutoProxy->enableLocalHost(true);
        mpProxy->setType(QNetworkProxy::NoProxy);
        QNetworkProxy::setApplicationProxy(*mpProxy);
        ui->labelBookInfoState->setText(QString("Proxy disabled!"));

        qDebug() << "HDH: SETUP NOPROXY";

        if (mpAutoProxy->getProxyBusyFlagState())
            slot_pushButtonGetInfoCancel_clicked();

        mpAutoProxy->enableLocalHost(true);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Open proxy setup dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_proxySetupDialog()
{
    QNetworkProxy tmpProxyCopy = *mpProxy;
    bool bAutoProxyState = mpAutoProxy->getAutoProxyState();
    bool bUsProxyOnlyState = mpAutoProxy->getUsProxyOnlyState();
    mpProxyDialog->setAutomaticProxyActivation(mtUserSettings.bEnableAutoProxyActivation);

    if (mpProxyDialog->exec())
    {
        // Apply settings
        mpProxy->setHostName(mpProxyDialog->getProxyIp());
        mpProxy->setPort(mpProxyDialog->getProxyPort());
        mpProxy->setUser(mpProxyDialog->getProxyUsername());
        mpProxy->setPassword(mpProxyDialog->getProxyPassword());
        mpProxy->setType(mpProxyDialog->getProxyType());
        mtUserSettings.bEnableAutoProxyActivation = mpProxyDialog->getAutoProxyActivation();

        *mpUserProxy = *mpProxy;

        mpAutoProxy->setAutoProxyState(mpProxyDialog->getAutoProxy());
        mpAutoProxy->setUsProxyOnlyState(mpProxyDialog->getUsProxyOnlyState());
    }

    /* Check for changes */
    if ((tmpProxyCopy != *mpProxy) ||
            (bAutoProxyState != mpAutoProxy->getAutoProxyState()) ||
            (bUsProxyOnlyState != mpAutoProxy->getUsProxyOnlyState()) )
    {
        mpAutoProxy->clearProxyList();
        mpAutoProxy->clearFailures();
        if (ui->checkBoxUseProxy->isChecked() )
            on_checkBoxUseProxy_clicked(true);

    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Set GUI style
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_styleSetupDialog()
{
    this->hide();
    QStringList sStyleList;
    sStyleList = QStyleFactory::keys();

    QRegExp regExp(".(.*)\\+?Style");
    QString sCurrentStyle = QApplication::style()->metaObject()->className();

    if (regExp.exactMatch(sCurrentStyle))
        sCurrentStyle = regExp.cap(1);

    QString sNewStyle = QInputDialog::getItem(this, "Select Style", "Styles:", sStyleList, sStyleList.indexOf(sCurrentStyle));
    qApp->setStyle(sNewStyle);

    adjustSize();
    slot_adjustGuiGrid();

    for (int i = 0;  i < ui->gridLayoutBookInfo->columnCount(); i++)
    {
        ui->gridLayoutBookInfo->setColumnMinimumWidth(i,-1);
        ui->gridLayoutDownloadSettings->setColumnMinimumWidth(i,-1);
        ui->gridLayoutMergeAndConversion->setColumnMinimumWidth(i,-1);
    }

    adjustSize();
    slot_adjustGuiGrid();

    setAppFont();

    slot_saveGuiSettings();

    this->show();
    this->resize(this->size() + QSize(ui->scrollArea->horizontalScrollBar()->maximum(),ui->scrollArea->verticalScrollBar()->maximum()));
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: Font setup dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_fontSetupDialog()
{
    this->hide();

    // Show font setup dialog
    QFont prevGuiFont = this->font();
    QFont guiFont = QFontDialog::getFont(0, this->font());

    // Reset font size to default min value (6pt) and adjust GUI
    QFont guiFontSmall = guiFont;
    guiFontSmall.setPointSize(6);
    qApp->setFont(guiFontSmall);
    this->setFont(guiFontSmall);

    ui->batchJobTabWidget->setGuiFont(guiFontSmall);
    ui->tableWidgetHistory->setFont(guiFontSmall);
    ui->tableWidgetLinkList->setFont(guiFontSmall);


    for (int i = 0;  i < ui->gridLayoutBookInfo->columnCount(); i++)
    {
        ui->gridLayoutBookInfo->setColumnMinimumWidth(i,-1);
        ui->gridLayoutDownloadSettings->setColumnMinimumWidth(i,-1);
        ui->gridLayoutMergeAndConversion->setColumnMinimumWidth(i,-1);
    }

    adjustSize();
    slot_adjustGuiGrid();

    // Apply new font size and adjust GUI
    qApp->setFont(guiFont);
    this->setFont(guiFont);

    for (int i = 0;  i < ui->gridLayoutBookInfo->columnCount(); i++)
    {
        ui->gridLayoutBookInfo->setColumnMinimumWidth(i,-1);
        ui->gridLayoutDownloadSettings->setColumnMinimumWidth(i,-1);
        ui->gridLayoutMergeAndConversion->setColumnMinimumWidth(i,-1);
    }

    ui->batchJobTabWidget->setGuiFont(guiFont);
    ui->tableWidgetHistory->setFont(guiFont);
    ui->tableWidgetLinkList->setFont(guiFont);

    adjustSize();
    slot_adjustGuiGrid();

    slot_saveGuiSettings();

    // Show hint when font size has been reduced
    if (guiFont.pointSize() < prevGuiFont.pointSize())
        QMessageBox::information(this, tr("Restart HDD"),
                             tr("For best GUI performance, please restart Hathi Download Helper."));

    this->show();
    this->resize(this->size() + QSize(ui->scrollArea->horizontalScrollBar()->maximum(),ui->scrollArea->verticalScrollBar()->maximum()));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Set GUI font
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::setAppFont()
{
    QFont guiFont = qApp->font();
    QFont guiFontSmall = guiFont;
    guiFontSmall.setPointSize(6);
    qApp->setFont(guiFontSmall);
    this->setFont(guiFontSmall);

    for (int i = 0;  i < ui->gridLayoutBookInfo->columnCount(); i++)
    {
        ui->gridLayoutBookInfo->setColumnMinimumWidth(i,-1);
        ui->gridLayoutDownloadSettings->setColumnMinimumWidth(i,-1);
        ui->gridLayoutMergeAndConversion->setColumnMinimumWidth(i,-1);
    }

    adjustSize();
    slot_adjustGuiGrid();

    qApp->setFont(guiFont);
    this->setFont(guiFont);

    for (int i = 0;  i < ui->gridLayoutBookInfo->columnCount(); i++)
    {
        ui->gridLayoutBookInfo->setColumnMinimumWidth(i,-1);
        ui->gridLayoutDownloadSettings->setColumnMinimumWidth(i,-1);
        ui->gridLayoutMergeAndConversion->setColumnMinimumWidth(i,-1);
    }

    ui->batchJobTabWidget->setGuiFont(guiFont);
    ui->tableWidgetHistory->setFont(guiFont);
    ui->tableWidgetLinkList->setFont(guiFont);

    adjustSize();
    slot_adjustGuiGrid();
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: Get guisetup.xml from configuration file
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::getXmlGuiSetup(void)
{
    QString sGuiSetup = readFile(QString("guisetup.xml"));
    QFont guiFont = this->font();

    if (sGuiSetup.isEmpty())
        return;

    QString sTagValue ;
    bool ok = 0;
    int iTagValue;

    // Appearance
    sTagValue  = myXmlReader(&sGuiSetup, "style");
    if (!sTagValue.isEmpty())
    {
        mtUserGuiSettings.sStyle = sTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "fontfamily");
    if (!sTagValue.isEmpty())
    {
        mtUserGuiSettings.sFontFamily = sTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "fontpointsize");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserGuiSettings.iFontPointSize = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "fontitalic");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserGuiSettings.bFontItalic = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "fontbold");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserGuiSettings.bFontBold = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "fontstrikeout");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserGuiSettings.bFontStrikeout = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "fontunderline");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserGuiSettings.bFontUnderline = iTagValue;
    }

    qApp->setFont(guiFont);
    this->setFont(guiFont);

    /* User settings -----------------------------------------------*/
    sTagValue  = myXmlReader(&sGuiSetup, "autoupdate");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bAutoUpdate = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "useproxy");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bUseProxy = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "autoproxyactivation");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bEnableAutoProxyActivation = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "getpdfs");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bDownloadPdfs = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "getimages");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bDownloadImages = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "getocr");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bDownloadOcr = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "downloadwholebook");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bDownloadWholeBook = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "zoom");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.iZoom = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "dstfld");
    if (!sTagValue.isEmpty())
    {
        mtUserSettings.sDestinationFolder = sTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "autofld");
    if (sTagValue.isEmpty())
        sTagValue = "0";
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bCreateDownloadFolder = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "autofldtitle");
    if (sTagValue.isEmpty())
        sTagValue = "0";
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bCreateDownloadFolderByTitle = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "numtitlechars");
    if (sTagValue.isEmpty())
        sTagValue = "20";
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.iTitleChars = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "resume");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bResume = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "autoproxy");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bEnableAutoProxy = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "webproxy");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bEnableWebProxy = iTagValue;
        // Only one is allowed
        if (mtUserSettings.bEnableWebProxy && mtUserSettings.bEnableAutoProxy)
            mtUserSettings.bEnableWebProxy = false;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "autopdf");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bAutoPdfBook = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "mergepdf");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bMergePdf = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "img2book");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bImage2Book = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "img2pdf");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bImage2Pdf = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "ocronly");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bOcrOnly = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "keepexistingfile");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bKeepExistingFile = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "deletedownloadeddata");
    iTagValue  = sTagValue.toInt(&ok,10);
    if (ok)
    {
        mtUserSettings.bDeleteDownloadedPageData = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "webproxyaccept");
    iTagValue  = sTagValue.toInt(&ok,10);

    if (ok)
    {
        mtUserSettings.bWebProxyNoteAccepted = iTagValue;
    }

    sTagValue  = myXmlReader(&sGuiSetup, "whatsnewversion");
    mtUserSettings.sWhatsNewVersion = sTagValue;
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Apply guisetup.xml configuration file
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::setXmlGuiSetup(void)
{
    // Style settings
    QFont guiFont = this->font();

    qApp->setStyle(mtUserGuiSettings.sStyle);
    guiFont.setFamily(mtUserGuiSettings.sFontFamily);
    guiFont.setPointSize(mtUserGuiSettings.iFontPointSize);
    guiFont.setItalic(mtUserGuiSettings.bFontItalic);
    guiFont.setBold(mtUserGuiSettings.bFontBold);
    guiFont.setStrikeOut(mtUserGuiSettings.bFontStrikeout);
    guiFont.setUnderline(mtUserGuiSettings.bFontUnderline);
    qApp->setFont(guiFont);
    this->setFont(guiFont);

    // User settings
    setUserSettings();
    setAppFont();
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Apply guisetup.xml setUserSettings configuration
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::setUserSettings(void)
{
    // User settings
    mpAutoUpdateAct->setChecked(mtUserSettings.bAutoUpdate);
    if (ui->checkBoxUseProxy->isChecked() != mtUserSettings.bUseProxy)
    {
        ui->checkBoxUseProxy->setChecked(mtUserSettings.bUseProxy);
        this->on_checkBoxUseProxy_clicked(true);
    }
    else
        ui->checkBoxUseProxy->setChecked(mtUserSettings.bUseProxy);
    ui->radioButtonPdf->setChecked(mtUserSettings.bDownloadPdfs);
    ui->radioButtonImages->setChecked(mtUserSettings.bDownloadImages);
    ui->comboBoxZoom->setCurrentIndex(mtUserSettings.iZoom);
    ui->checkBoxGetOcr->setChecked(mtUserSettings.bDownloadOcr);
    if (ui->lineEditDstFolder->isEnabled())
        ui->lineEditDstFolder->setText(mtUserSettings.sDestinationFolder);
    ui->checkBoxWebProxy->setChecked(mtUserSettings.bEnableWebProxy);
    ui->checkBoxAutoProxy->setChecked(mtUserSettings.bEnableAutoProxy);
    ui->checkBoxResume->setChecked(mtUserSettings.bResume);
    ui->checkBoxAutoPdf->setChecked(mtUserSettings.bAutoPdfBook);
    ui->radioButtonMergePdftk->setChecked(mtUserSettings.bMergePdf);
    ui->radioButtonMergeImages->setChecked(mtUserSettings.bImage2Book);
    ui->radioButtonConvertImages->setChecked(mtUserSettings.bImage2Pdf);
    ui->checkBoxConvertOcrOnly->setChecked(mtUserSettings.bOcrOnly);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Write GUI settings to file
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_saveGuiSettings(void)
{
    QFont guiFont = qApp->font();

    QRegExp regExp(".(.*)\\+?Style");
    QString sCurrentStyle = QApplication::style()->metaObject()->className();
    if (regExp.exactMatch(sCurrentStyle))
        sCurrentStyle = regExp.cap(1);

    // Store Settings in guisetup.xml file
    QString sGuiSetup;
    sGuiSetup.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    sGuiSetup.append("<stylesetup>");
    sGuiSetup.append("<style>" +  sCurrentStyle + "</style>\n");
    sGuiSetup.append("</stylesetup>\n");
    sGuiSetup.append("<fontsetup>\n");
    sGuiSetup.append("<fontfamily>" + guiFont.family() + "</fontfamily>\n");
    sGuiSetup.append("<fontpointsize>" + QString::number(guiFont.pointSize()) + "</fontpointsize>\n");
    sGuiSetup.append("<fontitalic>" + QString::number(guiFont.italic()) + "</fontitalic>\n");
    sGuiSetup.append("<fontbold>" + QString::number(guiFont.bold()) + "</fontbold>\n");
    sGuiSetup.append("<fontstrikeout>" + QString::number(guiFont.strikeOut()) + "</fontstrikeout>\n");
    sGuiSetup.append("<fontunderline>" + QString::number(guiFont.underline()) + "</fontunderline>\n");
    sGuiSetup.append("</fontsetup>\n");
    sGuiSetup.append("<usersettings>\n");
    mtUserSettings.bAutoUpdate = mpAutoUpdateAct->isChecked();
    sGuiSetup.append("<autoupdate>" + QString::number(mtUserSettings.bAutoUpdate) + "</autoupdate>\n" );
    sGuiSetup.append("<useproxy>" + QString::number(mtUserSettings.bUseProxy) + "</useproxy>\n" );
    sGuiSetup.append("<autoproxyactivation>" + QString::number(mtUserSettings.bEnableAutoProxyActivation) + "</autoproxyactivation>\n" );
    sGuiSetup.append("<getpdfs>" + QString::number(mtUserSettings.bDownloadPdfs) + "</getpdfs>" );
    sGuiSetup.append("<getimages>" + QString::number(mtUserSettings.bDownloadImages) + "</getimages>\n" );
    sGuiSetup.append("<getocr>" + QString::number(mtUserSettings.bDownloadOcr) + "</getocr>\n" );
    sGuiSetup.append("<downloadwholebook>" + QString::number(mtUserSettings.bDownloadWholeBook) + "</downloadwholebook>\n");
    sGuiSetup.append("<zoom>" + QString::number(mtUserSettings.iZoom) + "</zoom>\n" );
    sGuiSetup.append("<dstfld>" + mtUserSettings.sDestinationFolder + "</dstfld>\n" );
    sGuiSetup.append("<autofld>" + QString::number(mtUserSettings.bCreateDownloadFolder) + "</autofld>\n" );
    sGuiSetup.append("<autofldtitle>" + QString::number(mtUserSettings.bCreateDownloadFolderByTitle) + "</autofldtitle>\n" );
    sGuiSetup.append("<numtitlechars>" + QString::number(mtUserSettings.iTitleChars) + "</numtitlechars>\n" );
    sGuiSetup.append("<resume>" + QString::number(mtUserSettings.bResume) + "</resume>\n" );
    sGuiSetup.append("<webproxy>" + QString::number(mtUserSettings.bEnableWebProxy) + "</webproxy>\n" );
    sGuiSetup.append("<autoproxy>" + QString::number(mtUserSettings.bEnableAutoProxy) + "</autoproxy>\n" );
    sGuiSetup.append("<autopdf>" + QString::number(mtUserSettings.bAutoPdfBook) + "</autopdf>\n" );
    sGuiSetup.append("<mergepdf>" + QString::number(mtUserSettings.bMergePdf) + "</mergepdf>\n" );
    sGuiSetup.append("<img2book>" + QString::number(mtUserSettings.bImage2Book) + "</img2book>\n" );
    sGuiSetup.append("<img2pdf>" + QString::number(mtUserSettings.bImage2Pdf) + "</img2pdf>\n" );
    sGuiSetup.append("<ocronly>" + QString::number(mtUserSettings.bOcrOnly) + "</ocronly>\n" );
    sGuiSetup.append("<keepexistingfile>" + QString::number(mtUserSettings.bKeepExistingFile) + "</keepexistingfile>\n");
    sGuiSetup.append("<deletedownloadeddata>" + QString::number(mtUserSettings.bDeleteDownloadedPageData) + "</deletedownloadeddata>\n");
    sGuiSetup.append("<webproxyaccept>" + QString::number(mtUserSettings.bWebProxyNoteAccepted) + "</webproxyaccept>\n" );
    sGuiSetup.append("<whatsnewversion>" + qApp->applicationVersion() + "</whatsnewversion>\n" );
    sGuiSetup.append("</usersettings>\n");
    writeFile(this, QString("guisetup.xml"), &sGuiSetup);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Reset GUI style and font to default
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_resetGuiSetup()
{
    this->hide();
    qApp->setFont(mDefaultFont);
    this->setFont(mDefaultFont);
    qApp->setStyle(msDefaultStyle);

    for (int i = 0;  i < ui->gridLayoutBookInfo->columnCount(); i++)
    {
        ui->gridLayoutBookInfo->setColumnMinimumWidth(i,-1);
        ui->gridLayoutDownloadSettings->setColumnMinimumWidth(i,-1);
        ui->gridLayoutMergeAndConversion->setColumnMinimumWidth(i,-1);
    }

    adjustSize();
    slot_adjustGuiGrid();

    setAppFont();

    slot_saveGuiSettings();

    this->show();
    this->resize(this->size() + QSize(ui->scrollArea->horizontalScrollBar()->maximum(),ui->scrollArea->verticalScrollBar()->maximum()));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Create menu bar actions
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::createActions()
{
    mpExitAct = new QAction(tr("E&xit"), this);
    mpExitAct->setShortcuts(QKeySequence::Quit);
    connect(mpExitAct, SIGNAL(triggered()), this, SLOT(on_pushButtonExit_clicked()));

    mpProxyAct = new QAction(tr("Proxy"), this);
    connect(mpProxyAct, SIGNAL(triggered()), this, SLOT(slot_proxySetupDialog()));

    mpPageAct = new QAction(tr("Page setup"), this);
    connect(mpPageAct, SIGNAL(triggered()), this, SLOT(slot_pageSetupDialog()));

    mpStyleAct = new QAction(tr("Style setup"), this);
    connect(mpStyleAct, SIGNAL(triggered()), this, SLOT(slot_styleSetupDialog()));

    mpFontAct = new QAction(tr("Font setup"), this);
    connect(mpFontAct, SIGNAL(triggered()), this, SLOT(slot_fontSetupDialog()));

    mpResetGuiAct = new QAction(tr("Default"), this);
    connect(mpResetGuiAct, SIGNAL(triggered()), this, SLOT(slot_resetGuiSetup()));

    mpUserSettingsAct = new QAction(tr("&User settings"), this);
    connect(mpUserSettingsAct, SIGNAL(triggered()), this, SLOT(slot_userSettingsDialog()));

    mpBatchAct = new QAction(tr("&Batch job"), this);
    connect(mpBatchAct, SIGNAL(triggered()), this, SLOT(slot_selectBatchJobTab()));

    mpExportAct = new QAction(tr("&Export links"), this);
    connect(mpExportAct, SIGNAL(triggered()), this, SLOT(slot_exportLinksDialog()));

    mpMergePdfs = new QAction(tr("&Merge PDFs"), this);
    connect(mpMergePdfs, SIGNAL(triggered()), this, SLOT(slot_selectPdfFilesToBeMergerd()));

    mpHelpAct = new QAction(tr("&Help"), this);
    connect(mpHelpAct, SIGNAL(triggered()), this, SLOT(slot_help()));

    mpAboutAct = new QAction(tr("&About"), this);
    connect(mpAboutAct, SIGNAL(triggered()), this, SLOT(slot_about()));

    mpContactAct = new QAction(tr("Contact && Bug report"), this);
    connect(mpContactAct, SIGNAL(triggered()), this, SLOT(slot_sendFeeback()));

#if defined(Q_OS_MAC)
    mpContactAct->setDisabled(true);        // Disabled due to crash on exit: SEE QT-BUG 37699
#endif

//    mpAboutQtAct = new QAction(tr("About &Qt"), this);
//    connect(mpAboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    mpWhatsNewAct = new QAction(tr("&What's New"), this);
    connect(mpWhatsNewAct, SIGNAL(triggered()), this, SLOT(slot_showWhatsNew()));

    mpUpdateAct = new QAction(tr("&Check for update"), this);
    connect(mpUpdateAct, SIGNAL(triggered()), this, SLOT(slot_loadVersionFile()));

    mpAutoUpdateAct = new QAction(tr("A&utomatically &check for update"), this);
    mpAutoUpdateAct->setCheckable(true);
    connect(mpAutoUpdateAct, SIGNAL(triggered()), this, SLOT(slot_saveGuiSettings()));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Create menu bar
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::createMenus()
{
    mpFileMenu = menuBar()->addMenu(tr("&File"));
    mpFileMenu->addAction(mpExitAct);

    mpOptionsMenu = menuBar()->addMenu(tr("&Options"));
    mpOptionsMenu->addAction(mpPageAct);
    mpStyleSubmenu = mpOptionsMenu->addMenu(tr("&GUI setup"));
    mpStyleSubmenu->addAction(mpStyleAct);
    mpStyleSubmenu->addAction(mpFontAct);
    mpStyleSubmenu->addAction(mpResetGuiAct);
    mpOptionsMenu->addAction(mpUserSettingsAct);

    mpToolMenu = menuBar()->addMenu(tr("&Tools"));
    mpToolMenu->addAction(mpProxyAct);
    mpToolMenu->addAction(mpBatchAct);
    mpToolMenu->addAction(mpExportAct);
    mpToolMenu->addAction(mpMergePdfs);

    mpHelpMenu = menuBar()->addMenu(tr("&Help"));
    mpHelpMenu->addAction(mpHelpAct);
    mpHelpMenu->addAction(mpAboutAct);
    mpHelpMenu->addAction(mpContactAct);
 //   mpHelpMenu->addAction(mpAboutQtAct);
    mpHelpMenu->addAction(mpWhatsNewAct);
    mpHelpMenu->addAction(mpUpdateAct);
    mpHelpMenu->addAction(mpAutoUpdateAct);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Help dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_help()
{
    QWebView* helpDialog= new QWebView(this);

    helpDialog->setWindowFlags(Qt::Window);
    helpDialog->setUrl(QUrl("qrc:/html/help.html"));
    helpDialog->show();
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: Download directory selection dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_toolButtonDstFolder_clicked()
{
    QString sDirectory = QFileDialog::getExistingDirectory(this, tr("Choose Or Create Directory"),
                                                           ui->lineEditDstFolder->text(),
                                                           QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog );
    if (!sDirectory.isEmpty())
    {
        ui->lineEditDstFolder->setText(sDirectory);
        on_lineEditDstFolder_editingFinished();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Source directory selection dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_toolButtonSrcFolder_clicked()
{
    QString sDirectory = QFileDialog::getExistingDirectory(this, tr("Choose Or Create Directory"),
                                                           ui->lineEditSrcFolder->text(),
                                                           QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog );
    if (!sDirectory.isEmpty())
    {
        ui->lineEditSrcFolder->setText(sDirectory);
        on_lineEditSrcFolder_editingFinished();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: update history when download finished
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::addBookToHistory()
{
    msLatestHistoryItem.clear();

    // Date
    QDateTime date = QDateTime::currentDateTime();
    QTableWidgetItem *pDateItem = new QTableWidgetItem;
    pDateItem->setFlags(pDateItem->flags()|Qt::ItemIsEditable);
    pDateItem->setText(date.toString("yyyy-MM-dd hh:mm:ss"));
    msLatestHistoryItem.append(pDateItem->text());

    // URL
    QTableWidgetItem *pUrlItem = new QTableWidgetItem;
    pUrlItem->setFlags(pUrlItem->flags()|Qt::ItemIsEditable);
    pUrlItem->setText(ui->lineEditBookURL->text());
    msLatestHistoryItem.append(pUrlItem->text());

    // Title
    QTableWidgetItem *pTitleItem = new QTableWidgetItem;
    pTitleItem->setFlags(pTitleItem->flags()|Qt::ItemIsEditable);
    pTitleItem->setText(ui->labelBookTitleText->text());
    msLatestHistoryItem.append(pTitleItem->text());

    // Author
    QTableWidgetItem *pAuthorItem = new QTableWidgetItem;
    pAuthorItem->setFlags(pAuthorItem->flags()|Qt::ItemIsEditable);
    pAuthorItem->setText(ui->labelAuthorText->text());
    msLatestHistoryItem.append(pAuthorItem->text());

    // Published
    QTableWidgetItem *pPublishedItem = new QTableWidgetItem;
    pPublishedItem->setFlags(pPublishedItem->flags()|Qt::ItemIsEditable);
    pPublishedItem->setText(ui->labelPublishedText->text());
    msLatestHistoryItem.append(pPublishedItem->text());

    // Folder
    QTableWidgetItem *pFolderItem = new QTableWidgetItem;
    pFolderItem->setFlags(pFolderItem->flags()|Qt::ItemIsEditable);
    pFolderItem->setText(ui->lineEditDstFolder->text());
    msLatestHistoryItem.append(pFolderItem->text());

    // Add book to data base
    if (openHistoryDatabase())
    {
        QSqlQuery query = QSqlQuery( mHistoryDatabase );

        QString sDbColTitles;
        foreach(QString sColTitle, *mpsListColumnHeaders)
            sDbColTitles.append(sColTitle + " TEXT,");
        sDbColTitles.chop(1);

        if(!query.exec( "create table if not exists " + mHistoryDbName + "(" + sDbColTitles + ")"))
            qDebug() << "HDH: Unable to create hstr data table in database - " << query.lastError();

        if(!query.exec("BEGIN TRANSACTION"))
            qDebug() << "HDH: Unable to begin transaction of hstr data to database - " << query.lastError();

        QString sQuery = "INSERT INTO " + mHistoryDbName + " (Date, Link, Title, Author, Published, Path) "
                + "VALUES (:Date, :Link, :Title, :Author, :Published, :Path)";
        query.prepare(sQuery);

        // bind new data
        query.bindValue(":Date", pDateItem->text());
        query.bindValue(":Link", pUrlItem->text());
        query.bindValue(":Title", pTitleItem->text());
        query.bindValue(":Author", pAuthorItem->text());
        query.bindValue(":Published", pPublishedItem->text());
        query.bindValue(":Path", pFolderItem->text());
        query.exec();

        if(!query.exec("COMMIT"))
            qDebug() << "HDH: Unable to commit hstr data to database - " << query.lastError();
    }

    // Add new row and set items
    disconnect(ui->tableWidgetHistory, SIGNAL(cellChanged(int,int)), this, SLOT(slot_updateTableWidgetHistory(int, int)));
    int iRow = 0;
    iRow = ui->tableWidgetHistory->rowCount();
    ui->tableWidgetHistory->insertRow(iRow);

    ui->tableWidgetHistory->setItem(iRow, 0, pDateItem);
    ui->tableWidgetHistory->setItem(iRow, 1, pUrlItem);
    ui->tableWidgetHistory->setItem(iRow, 2, pTitleItem);
    ui->tableWidgetHistory->setItem(iRow, 3, pAuthorItem);
    ui->tableWidgetHistory->setItem(iRow, 4, pPublishedItem);
    ui->tableWidgetHistory->setItem(iRow, 5, pFolderItem);
    connect(ui->tableWidgetHistory, SIGNAL(cellChanged(int,int)), this, SLOT(slot_updateTableWidgetHistory(int, int)));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Open page setup dialog and create pagsetup.xml file
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_pageSetupDialog()
{
#if defined(Q_OS_WIN)
    QString sInstalledPrinter(mpPrinter->printerName());
    if (sInstalledPrinter.isEmpty())
    {
        QMessageBox::warning(this, tr("Missing printer"),
                             tr("Before you can perform page setup you need to install a printer."));
    }
    else
#endif
    {
        if ( mpPageSetup->exec() )
        {
            // Apply settings to pdf printer
            mpPrinter->setPaperSize(mpSetupPrinter->paperSize());
            mpPrinter->setOrientation(mpSetupPrinter->orientation());
            qreal left, top, right, bottom;
            mpSetupPrinter->getPageMargins(&left, &top, &right, &bottom, QPrinter::Point);
            mpPrinter->setPageMargins(left, top, right, bottom, QPrinter::Point);
            mpPrinter->setResolution(mpviDpi->at(ui->comboBoxPrinterRes->currentIndex()));
            mpPrinter->setOutputFormat(QPrinter::PdfFormat);

            // Store Settings in pagesetup.xml file
            QString sPageSetup;
            sPageSetup.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            sPageSetup.append("<pagesetup>\n");
            sPageSetup.append("<paperSize>" +  QString::number(mpPrinter->paperSize()) + "</paperSize>\n");
            sPageSetup.append("<orientation>" + QString::number(mpPrinter->orientation()) + "</orientation>\n");
            sPageSetup.append("<pageMargins>\n");
            sPageSetup.append("<marginLeft>" + QString::number(left) + "</marginLeft>\n");
            sPageSetup.append("<marginTop>" + QString::number(top) + "</marginTop>\n");
            sPageSetup.append("<marginRight>" + QString::number(right) + "</marginRight>\n");
            sPageSetup.append("<marginBottom>" + QString::number(bottom) + "</marginBottom>\n");
            sPageSetup.append("<marginUnit>" +  QString::number(QPrinter::Point) + "</marginUnit>\n");
            sPageSetup.append("</pageMargins>\n");
            sPageSetup.append("</pagesetup>\n");
            writeFile(this, QString("pagesetup.xml"), &sPageSetup);
        }
    }
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: About dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_about()
{
    QString sVersion = this->windowTitle();
    sVersion.mid(QString("Hathi Download Helper ").length());

    QMessageBox::information(this, tr("About Hathi Download Helper"),
                             tr("<table><tr><td><img src=':/images/elephant.png'></td>"
                                "<td><table><tr><td><h1>Hathi Download Helper</h1></td></tr><tr><td> An assistant to download books from Hathitrust.org.</td></tr>"
                                "<tr><td>Version %1</td></tr>"
                                "<tr><td style='white-space:nowrap;'><br>WWW: &nbsp;<a href='http://hathidownloadhelper.sourceforge.net/' >www.hathidownloadhelper.sourceforge.net</a></td></tr>"
                                "<tr><td style='white-space:nowrap;'><br>WWW: &nbsp;<a href='http://www.facebook.com/hathidownloadhelperApp' >www.facebook.com/hathidownloadhelperApp</a></td></tr>"
                                "<tr><td style='white-space:nowrap;'><br>WWW: &nbsp;<a href='http://www.softpedia.com/get/Internet/Download-Managers/Hathi-Download-Helper.shtml' >www.softpedia.com</a></td></tr>"
                                "<tr><td style='white-space:nowrap;'><br>Email: &nbsp;<a href='mailto:hathidownloadhelper@hotmail.com'>hathidownloadhelper@hotmail.com</a></td></tr>"
                                "</table></td></tr></table>").arg(sVersion));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: What's new dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_showWhatsNew()
{
    QDialog *pWhatsNewDialog        = new QDialog;
    QGridLayout *pGridLayout        = new QGridLayout;

    // Change log text ---------------------------------------------------------------------------

    QGroupBox *pWhatsNewGroupBox = new QGroupBox;
    pWhatsNewGroupBox->setTitle("Change log for Version " + qApp->applicationVersion());
    QVBoxLayout *pVBoxLayout = new QVBoxLayout;
    QLabel *pWhatsNewLabel = new QLabel;

    QString sWhatsNew = "<ul>";
    sWhatsNew.append("<li>Added option to remove downloaded page data after pdf book generation</li>");
    sWhatsNew.append("<ul><li>Use checkbox in [Options]->[User settings]->[PDF merge & conversion] for activation</li></ul><br>");
    sWhatsNew.append("<li>Added SQL data base for downloaded books including filter options</li>");
    sWhatsNew.append("<ul><li>See 'History DB'-Tab for details</li></ul><br>");
    sWhatsNew.append("<li>Added bugfixes / improvements for auto proxy feature</li>");
    sWhatsNew.append("<ul><li>Removed obsolete proxy sources / added new ones etc.</li></ul>");
    sWhatsNew.append("</ul>");

    pWhatsNewLabel->setText(sWhatsNew);
    // pWhatsNewLabel->setStyleSheet("QLabel { background-color : white; color : black;}");
    pVBoxLayout->addWidget(pWhatsNewLabel);
    pWhatsNewGroupBox->setLayout(pVBoxLayout);
    pGridLayout->addWidget(pWhatsNewGroupBox, 0,0,1,2);

    // Dialog Cntrl ----------------------------------------------------------------------------
    QHBoxLayout *pHBoxLayoutCntrl   = new QHBoxLayout;
    QSpacerItem* spacer1                = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    pHBoxLayoutCntrl->addItem(spacer1);
    QPushButton *pPushButtonClose      = new QPushButton(QString("Close"));
    connect(pPushButtonClose, SIGNAL(clicked()), pWhatsNewDialog, SLOT(accept()));
    pHBoxLayoutCntrl->addWidget(pPushButtonClose);

    pGridLayout->addLayout(pHBoxLayoutCntrl,1,0,1,2);

    pWhatsNewDialog->setLayout(pGridLayout);
    pWhatsNewDialog->setWindowTitle(QString("What's New"));
    pWhatsNewDialog->exec();
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Update tab title of batch job
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_updateBatchJobTabTitle(int iJobs)
{
    ui->tabWidget->setTabText(1, QString("Batch Job (%1)").arg(iJobs));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Set pdf resolution to the same resolution as download images have (zoom)
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_comboBoxZoom_currentIndexChanged(int index)
{
    ui->comboBoxPrinterRes->setCurrentIndex(index);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Set pdf printer resolution
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_comboBoxPrinterRes_currentIndexChanged(int index)
{
    // Set pdf resolution
    mpPrinter->setResolution( mpviDpi->at(index) );
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Copy page URL from clipboard to lineEdit
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_copyFromClipboard(void)
{
    static QString sPrevClipboard = "";

    if (sPrevClipboard.compare(mpClipboard->text()))
    {
        sPrevClipboard = mpClipboard->text();
        QStringList sLinkList = mpClipboard->text().split(QRegExp("\n|\r\n|\r"));

        for (int i=0; i < sLinkList.count(); i++)
        {

            if (    (sLinkList.at(i).contains(QString("hathitrust.org")) && sLinkList.at(i).contains(QString("id=")))
                    || sLinkList.at(i).contains(QString("hdl.handle.net")) )
            {
                if (!ui->pushButtonStartDownload->text().contains("Cancel"))
                {
                    // Set book URL
                    ui->lineEditBookURL->setText(sLinkList.at(i));
                    // Reset labels
                    ui->labelBookTitleText->setText(QString("-"));
                    ui->labelNumPagesText->setText(QString("-"));
                    ui->labelBookIDText->setText(QString("-"));
                    ui->labelAuthorText->setText(QString("-"));
                    ui->labelPublishedText->setText(QString("-"));
                    ui->labelCopyrightText->setText(QString("-"));
                    if (!mBatchJobActivated)
                        ui->pushButtonStartDownload->setEnabled(false);
                }

                // Send clipboard content to job dialog
                emit clipboardChanged(sLinkList.at(i));

                // Add link to link collector
                slot_addLinkToCollectorList(sLinkList.at(i));

            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Copy page URL from clipboard to link collector list
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_addLinkToCollectorList(QString sUrl)
{
    sUrl = (QString) QUrl::fromPercentEncoding(sUrl.toLatin1()).toLatin1();

    // Check if link already exist
    if(ui->tableWidgetLinkList->findItems(sUrl,Qt::MatchContains).isEmpty())
    {
        // Add to list
        QTableWidgetItem* pUrlItem = new QTableWidgetItem(sUrl);
        int iRow = ui->tableWidgetLinkList->rowCount();
        ui->tableWidgetLinkList->insertRow(iRow);
        ui->tableWidgetLinkList->setItem(iRow, 0, pUrlItem);
        // Update tab title incl. number of links
        ui->tabWidget->setTabText(2, QString("Link collector (%1)").arg(ui->tableWidgetLinkList->rowCount()));
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Remove page URL from link collector list
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_removeLinkFromCollectorList(QString sUrl)
{
    QString sBookId = sUrl.mid(sUrl.indexOf("id=")+3, sUrl.indexOf(";", sUrl.indexOf("id=")+3)-1);

    for(int iRow = 0; iRow < ui->tableWidgetLinkList->rowCount(); iRow++)
    {
        if( (ui->tableWidgetLinkList->item(iRow, 0)->text() == sUrl) || ui->tableWidgetLinkList->item(iRow, 0)->text().contains(sBookId))
        {
            ui->tableWidgetLinkList->removeRow(iRow);
            // Update tab title incl. number of links
            ui->tabWidget->setTabText(2, QString("Link collector (%1)").arg(ui->tableWidgetLinkList->rowCount()));
            break;
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI:  Remove selected item link from link collector table
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_pushButtonRemoveLink_clicked()
{
    int iRow = ui->tableWidgetLinkList->selectionModel()->currentIndex().row();
    ui->tableWidgetLinkList->removeRow(iRow);

    // Update tab title incl. number of links
    ui->tabWidget->setTabText(2, QString("Link collector (%1)").arg(ui->tableWidgetLinkList->rowCount()));

    if (ui->tableWidgetLinkList->rowCount() > 0)
    {
        if (iRow >= ui->tableWidgetLinkList->rowCount())
            iRow--;
        ui->tableWidgetLinkList->selectRow(iRow);
        ui->tableWidgetLinkList->setFocus();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI:  Remove selected item from history table
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_lineEditFilterHistory_changed()
{
    QSqlQuery query = QSqlQuery( mHistoryDatabase );

    QString sDbColTitles;
    foreach(QString sColTitle, *mpsListColumnHeaders)
        sDbColTitles.append(sColTitle + " TEXT,");
    sDbColTitles.chop(1);

    if(!query.exec( "create table if not exists " + mHistoryDbName + "(" + sDbColTitles + ")"))
    {
        qDebug() << "HDH: Unable to create hstr data table in database - " << query.lastError();
    }
    else
    {
        disconnect(ui->tableWidgetHistory, SIGNAL(cellChanged(int,int)), this, SLOT(slot_updateTableWidgetHistory(int, int)));

        // Build query from column titles and filter contents
        QList<QStringList> sFilterList;
        QString sQuery = "SELECT * FROM " + mHistoryDbName + " WHERE ";
        int iActiveColumn  = -1;
        int iCursorPos  = -1;

        for (int iColCnt = 0; iColCnt < ui->tableWidgetHistory->columnCount(); iColCnt++)
        {
            sFilterList.append(QStringList());
            // Get filter value
            QLineEdit* pLineEdit = (QLineEdit*)ui->tableWidgetHistory->cellWidget(0,iColCnt);
            if (pLineEdit->hasFocus())
            {
                iActiveColumn = iColCnt;
                iCursorPos = pLineEdit->cursorPosition();
            }
            if(pLineEdit->text().isEmpty())
            {
                sFilterList[iColCnt].append("");
            }
            else
            {
                foreach(QString sFilter, pLineEdit->text().split(" ", QString::SkipEmptyParts))
                {
                    // Adding escape character for LIKE operators '%', '_' and '@'
                    sFilter.replace('@', "@@");
                    sFilter.replace('_', "@_");
                    sFilter.replace('%', "@%");

                    // Add column name
                    sQuery.append(ui->tableWidgetHistory->horizontalHeaderItem(iColCnt)->text());
                    // Add like command with condition
                    sQuery.append(" LIKE :");
                    sQuery.append(ui->tableWidgetHistory->horizontalHeaderItem(iColCnt)->text());
                    sQuery.append(QString::number(sFilterList[iColCnt].length()));
                    sQuery.append(" ESCAPE '@'");
                    sFilterList[iColCnt].append(sFilter);
                    sQuery.append(" AND ");
                }
            }
        }
        // remove last ' AND ' from query
        if (sQuery.contains(" AND "))
            sQuery.chop(5);
        else
            // remove ' where ' since filters are emtpy
            sQuery.chop(7);

        if (query.prepare(sQuery))
        {
            // Bind filter values
            for (int iColCnt = 0; iColCnt < ui->tableWidgetHistory->columnCount(); iColCnt++)
            {
                //if (!sFilterList.at(iColCnt).at(0).isEmpty())
                for (int iBindingCnt = 0; iBindingCnt < sFilterList[iColCnt].length(); iBindingCnt++)
                {
                    if (!sFilterList[iColCnt].at(iBindingCnt).isEmpty())
                    {
                        query.bindValue( ":" + ui->tableWidgetHistory->horizontalHeaderItem(iColCnt)->text() + QString::number(iBindingCnt), QString("%1%2%1").arg(QString("%")).arg(sFilterList[iColCnt].at(iBindingCnt)));
                    }
                }
            }

            if(!query.exec())
            {
                qDebug() << "HDH: Unable to load hstr data from database - " << query.lastError();
            }
            else
            {
                // delete rows and build up new table
                disconnect(ui->tableWidgetHistory, SIGNAL(itemSelectionChanged()), this, SLOT(slot_updateSelectionTableWidgetHistory()));
                ui->tableWidgetHistory->setRowCount(1);

                updateHistoryTableContent(&query);
                connect(ui->tableWidgetHistory, SIGNAL(itemSelectionChanged()), this, SLOT(slot_updateSelectionTableWidgetHistory()));
            }
        }
        connect(ui->tableWidgetHistory, SIGNAL(cellChanged(int,int)), this, SLOT(slot_updateTableWidgetHistory(int, int)));
    }
}


/*-----------------------------------------------------------------------------------------------*
 * GUI:  Remove all links from link collector table
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_pushButtonRemoveAll_clicked()
{
    ui->tableWidgetLinkList->setRowCount(0);
    // Update tab title incl. number of links
    ui->tabWidget->setTabText(2, QString("Link collector (%1)").arg(ui->tableWidgetLinkList->rowCount()));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI:  Remove selected item from history table
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_pushButtonRemoveItem_clicked()
{
    int iRow = ui->tableWidgetHistory->selectionModel()->currentIndex().row();
    if ((ui->tableWidgetHistory->rowCount() > 1) && (iRow > 0))
    {
        // Remove book from data base
        if (openHistoryDatabase())
        {
            QSqlQuery query = QSqlQuery( mHistoryDatabase );

            QString sDbColTitles;
            foreach(QString sColTitle, *mpsListColumnHeaders)
                sDbColTitles.append(sColTitle + " TEXT,");
            sDbColTitles.chop(1);

            if(!query.exec( "create table if not exists " + mHistoryDbName + "(" + sDbColTitles + ")"))
                qDebug() << "HDH: Unable to create hstr data table in database - " << query.lastError();

            if(!query.exec("BEGIN TRANSACTION"))
                qDebug() << "HDH: Unable to begin transaction of hstr data to database - " << query.lastError();

            QString sQuery = "DELETE FROM " + mHistoryDbName + " WHERE ";
            foreach(QString sColTitle, *mpsListColumnHeaders)
                sQuery.append(sColTitle + " = :" + sColTitle + " AND ");
            sQuery.chop(5);

            query.prepare(sQuery);
            for(int iCol = 0; iCol < ui->tableWidgetHistory->columnCount(); iCol++)
                query.bindValue(":" + mpsListColumnHeaders->at(iCol), ui->tableWidgetHistory->item(iRow,iCol)->text());

            if(query.exec())
                qDebug() << "HDH: Removed " << ui->tableWidgetHistory->item(iRow,1)->text() << " from history DB";

            if(!query.exec("COMMIT"))
                qDebug() << "HDH: Unable to commit hstr data to database - " << query.lastError();

        }

        disconnect(ui->tableWidgetHistory, SIGNAL(itemSelectionChanged()), this, SLOT(slot_updateSelectionTableWidgetHistory()));
        ui->tableWidgetHistory->removeRow(iRow);
        connect(ui->tableWidgetHistory, SIGNAL(itemSelectionChanged()), this, SLOT(slot_updateSelectionTableWidgetHistory()));

        // Set selection to new row
        if (iRow >= ui->tableWidgetHistory->rowCount())
            iRow--;
        ui->tableWidgetHistory->selectRow(iRow);
        ui->tableWidgetHistory->setFocus();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI:  Open path to selected item in history table
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_pushButtonOpenPath_clicked()
{
    int iRow = ui->tableWidgetHistory->selectionModel()->currentIndex().row();
    int iCol = ui->tableWidgetHistory->columnCount(); // Path is in the latest column of the table
    if ((ui->tableWidgetHistory->rowCount() > 1) && (iRow > 0))
    {
        showFileInFilemanager(ui->tableWidgetHistory->item(iRow, iCol-1)->text());
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI:  Remove all items from history table
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_pushButtonRemoveAllItems_clicked()
{
    disconnect(ui->tableWidgetHistory, SIGNAL(itemSelectionChanged()), this, SLOT(slot_updateSelectionTableWidgetHistory()));
    ui->tableWidgetHistory->setRowCount(1);
    connect(ui->tableWidgetHistory, SIGNAL(itemSelectionChanged()), this, SLOT(slot_updateSelectionTableWidgetHistory()));

    // Remove book from data base
    if (openHistoryDatabase())
    {
        QSqlQuery query = QSqlQuery( mHistoryDatabase );

        QString sDbColTitles;
        foreach(QString sColTitle, *mpsListColumnHeaders)
            sDbColTitles.append(sColTitle + " TEXT,");
        sDbColTitles.chop(1);

        if(!query.exec( "create table if not exists " + mHistoryDbName + "(" + sDbColTitles + ")"))
            qDebug() << "HDH: Unable to create hstr data table in database - " << query.lastError();

        if(!query.exec("BEGIN TRANSACTION"))
            qDebug() << "HDH: Unable to begin transaction of hstr data to database - " << query.lastError();

        QString sQuery = "DELETE FROM " + mHistoryDbName;
        query.prepare(sQuery);

        if(query.exec())
            qDebug() << "HDH: Removed all entries from history DB";

        if(!query.exec("COMMIT"))
            qDebug() << "HDH: Unable to commit hstr data to database - " << query.lastError();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Add link collector list to batch job list and start download
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_startLinkCollectorDownload()
{
    slot_addLinkCollectorListToBatchJob();
    ui->tabWidget->setCurrentIndex(1);
    ui->batchJobTabWidget->startBatchJob();
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Add link collector list to batch job list
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_addLinkCollectorListToBatchJob()
{
    for(int i=0; i < ui->tableWidgetLinkList->rowCount(); i++)
        ui->batchJobTabWidget->addBookJob(ui->tableWidgetLinkList->item(i, 0)->text());
    ui->tableWidgetLinkList->setRowCount(0);
    ui->tabWidget->setTabText(2, QString("Link collector (%1)").arg(ui->tableWidgetLinkList->rowCount()));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Change push button getBookInfo
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_enablePushButtonGetBookInfo(bool bEnabled)
{
    if (bEnabled)
    {
        // Stop animation
        ui->mpMovieRead->stop();
        ui->mpMovieRead->jumpToFrame(0);

        ui->pushButtonGetInfo->setText("Get book info");
        disconnect(ui->pushButtonGetInfo,SIGNAL(clicked()), this, SLOT(slot_pushButtonGetInfoCancel_clicked()));
        connect(ui->pushButtonGetInfo, SIGNAL(clicked()), this, SLOT(on_pushButtonGetInfo_clicked()));

    }
    else
    {
        // Start animation
        ui->mpMovieRead->start();

        ui->pushButtonGetInfo->setText("Cancel request");
        disconnect(ui->pushButtonGetInfo,SIGNAL(clicked()), this, SLOT(on_pushButtonGetInfo_clicked()));
        connect(ui->pushButtonGetInfo, SIGNAL(clicked()), this, SLOT(slot_pushButtonGetInfoCancel_clicked()));
    }

    if(mBatchJobActivated)
        ui->pushButtonGetInfo->setDisabled(bEnabled);

    ui->checkBoxUseProxy->setEnabled(bEnabled);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: connect signal and slots to batchjob widget
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::connectSignalsToCreateBatchJob()
{
    connect(ui->batchJobTabWidget, SIGNAL(batchJobActivated(bool)),     this, SLOT(slot_activateBatchJob(bool)));
    connect(ui->batchJobTabWidget, SIGNAL(setBookUrl(QString)),         this, SLOT(slot_setBookUrl(QString)));
    connect(ui->batchJobTabWidget, SIGNAL(setDownloadOptions(QString)), this, SLOT(slot_applyDownloadOptions(QString)));
    connect(ui->batchJobTabWidget, SIGNAL(setDstFolder(QString)),       this, SLOT(slot_setDstFolder(QString)));
    connect(ui->batchJobTabWidget, SIGNAL(setMergeOptions(QString)),    this, SLOT(slot_applyMergeOptions(QString)));
    connect(ui->batchJobTabWidget, SIGNAL(setProxyState(bool)),         this, SLOT(slot_setProxyState(bool)));
    connect(ui->batchJobTabWidget, SIGNAL(getBookInfo()),               this, SLOT(on_pushButtonGetInfo_clicked()));
    connect(ui->batchJobTabWidget, SIGNAL(stopPageLoad()),              this, SLOT(slot_pushButtonGetInfoCancel_clicked()));
    connect(ui->batchJobTabWidget, SIGNAL(startDownload()),             this, SLOT(on_pushButtonStartDownload_clicked()));
    connect(ui->batchJobTabWidget, SIGNAL(stopDownload()),              this, SLOT(slot_pushButtonCancelDownload_clicked()));
    connect(ui->batchJobTabWidget, SIGNAL(stopPdfMerge()),              this, SLOT(slot_stopPdfMergingProcess()));
    connect(this, SIGNAL(clipboardChanged(QString)),   ui->batchJobTabWidget, SLOT(slot_clipboardChanged(QString)));
    connect(this, SIGNAL(updateProgressbar(int)),      ui->batchJobTabWidget, SLOT(slot_updateProgressbar(int)));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: activate batch job
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_activateBatchJob(bool bState)
{
    if (bState)
    {
        connect(this, SIGNAL(proxyConnectionEstablished(bool)), ui->batchJobTabWidget, SLOT(on_proxyConnectionEstablished(bool)));
        connect(this, SIGNAL(bookInfoLoaded(bool)),             ui->batchJobTabWidget, SLOT(slot_bookInfoState(bool)));
        connect(this, SIGNAL(bookInfoCopyright(QString)),       ui->batchJobTabWidget, SLOT(slot_bookInfoCopyright(QString)));
        connect(this, SIGNAL(cancelBatchJob(void)),             ui->batchJobTabWidget, SLOT(slot_pushButtonCancel_clicked()));
        connect(this, SIGNAL(downloadFinished(bool)),           ui->batchJobTabWidget, SLOT(slot_downloadState(bool)));
        connect(this, SIGNAL(pdfMergeFinished(bool)),           ui->batchJobTabWidget, SLOT(slot_pdfMergeState(bool)));

        ui->pushButtonGetInfo->setDisabled(bState);
        ui->pushButtonStartDownload->setDisabled(bState);
        ui->pushButtonCreatePdf->setDisabled(bState);
    }
    else
    {
        disconnect(this, SIGNAL(proxyConnectionEstablished(bool)),  ui->batchJobTabWidget, SLOT(on_proxyConnectionEstablished(bool)));
        disconnect(this, SIGNAL(bookInfoLoaded(bool)),              ui->batchJobTabWidget, SLOT(slot_bookInfoState(bool)));
        disconnect(this, SIGNAL(downloadFinished(bool)),            ui->batchJobTabWidget, SLOT(slot_downloadState(bool)));
        disconnect(this, SIGNAL(pdfMergeFinished(bool)),            ui->batchJobTabWidget, SLOT(slot_pdfMergeState(bool)));

        if (mBatchJobActivated)
            slot_selectBatchJobTab();

        ui->pushButtonGetInfo->setDisabled(false);
        ui->pushButtonStartDownload->setDisabled(true);
        ui->pushButtonCreatePdf->setDisabled(false);
    }

    mBatchJobActivated = bState;
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: activate batch job tab
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_selectBatchJobTab()
{
    ui->tabWidget->setCurrentIndex(1); // 1 is index of batchjob dialog
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: export links dialog
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_exportLinksDialog()
{
    if (ui->lineEditBookURL->text().isEmpty())
    {
        QMessageBox::information(this, tr("Missing data"),tr("Missing data. Please enter hathitrust.org book URL in main window first."));
    }
    else
    {
        if ( !ui->lineEditBookURL->text().contains(msDocumentID) || msDocumentID.isEmpty() )
        {
            mLinkExportActivated = true;
            connect(this, SIGNAL(bookInfoLoaded(bool)), this, SLOT(slot_createLinksFile(bool)));
            on_pushButtonGetInfo_clicked();

        }
        else
        {
            slot_createLinksFile(true);
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: create link list file
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_createLinksFile(bool bCreateFile)
{
    mLinkExportActivated = false;
    disconnect(this, SIGNAL(bookInfoLoaded(bool)), this, SLOT(slot_createLinksFile(bool)));

    if (bCreateFile)
    {
        // Link list dialog
        QDialog *selectFormatDialog=new QDialog(this);
        selectFormatDialog->setModal(true);
        // OK, Cancel button
        QPushButton *pOk        =   new QPushButton("Ok",selectFormatDialog);
        connect(pOk, SIGNAL(clicked()), selectFormatDialog, SLOT(accept()));
        QPushButton *pCancel    =   new QPushButton("Cancel",selectFormatDialog);
        connect(pCancel, SIGNAL(clicked()), selectFormatDialog, SLOT(reject()));

        // Dynamic link list
        QLabel *pLabeldynHtml    =   new QLabel("Create adjustable link list:    \n",selectFormatDialog);
        QRadioButton *pRbDynamic =   new QRadioButton("pdfs, images and ocr text files",selectFormatDialog);
        pRbDynamic->setChecked(true);
        // Static link lists
        QLabel *pLabelSelect        =   new QLabel("Create static link list for:    \n",selectFormatDialog);
        QRadioButton *pRbPdfStatic  =   new QRadioButton("pdfs",selectFormatDialog);
        QRadioButton *pRbImgStatic  =   new QRadioButton("images",selectFormatDialog);
        // Zoom box
        QComboBox *pCbZoom          =   new QComboBox(selectFormatDialog);
        for (int i=0; i < ui->comboBoxZoom->count(); i++)
        {
            pCbZoom->addItem(ui->comboBoxZoom->itemText(i));
        }
        pCbZoom->setCurrentIndex(6);
        QRadioButton *pRbOcrStatic      =   new QRadioButton("ocr text files",selectFormatDialog);
        QVBoxLayout *pvLayoutDialog     = new QVBoxLayout;
        QHBoxLayout *phLayoutDialog     = new QHBoxLayout;
        QHBoxLayout *phButtonLayout     = new QHBoxLayout;

        pvLayoutDialog->setMargin(20);
        pvLayoutDialog->addWidget(pLabeldynHtml);
        pvLayoutDialog->addWidget(pRbDynamic);
        pvLayoutDialog->addSpacing(30);
        pvLayoutDialog->addWidget(pLabelSelect);
        pvLayoutDialog->addWidget(pRbPdfStatic);
        phLayoutDialog->addWidget(pRbImgStatic);
        phLayoutDialog->addWidget(pCbZoom);
        pvLayoutDialog->addLayout(phLayoutDialog);
        pvLayoutDialog->addWidget(pRbOcrStatic);
        pvLayoutDialog->addSpacing(30);
        phButtonLayout->addWidget(pOk);
        phButtonLayout->addWidget(pCancel);
        pvLayoutDialog->addLayout(phButtonLayout);
        selectFormatDialog->setLayout(pvLayoutDialog);

        // Show dialog
        if (selectFormatDialog->exec())
        {
            // Create html page
            QString sLinks;
            sLinks.append(QString("<html>\n<head><title>extracted links</title>\n"));
            if (pRbDynamic->isChecked())
            {
                // Create javascript functions
                sLinks.append(QString("<script type='text/javascript'>\n"));
                sLinks.append(QString(" var curZoom = \"size=200\"; var newZoom = \"\";\n"));
                sLinks.append(QString("	var curFormat = \"image?id\"; var newFormt = \"\";\n"));
                //  sLinks.append(QString("	var curExtension = \".png\"; var newExtension = \"\";\n"));
                sLinks.append(QString("function setZoom(zoomSelection) {  newZoom =  \"size=\" + zoomSelection; for ( var i = 0; i < document.links.length; i++){ document.links[i].href = document.links[i].href.replace(curZoom, newZoom);  document.links[i].text = document.links[i].text.replace(curZoom, newZoom); } curZoom = newZoom; }\n"));
                sLinks.append(QString("function disableRadioBtn(enableFalg) { document.forms[\"theForm\"].zoom50.disabled  = enableFalg; document.forms[\"theForm\"].zoom75.disabled  = enableFalg; document.forms[\"theForm\"].zoom100.disabled  = enableFalg; document.forms[\"theForm\"].zoom125.disabled  = enableFalg; document.forms[\"theForm\"].zoom150.disabled  = enableFalg; document.forms[\"theForm\"].zoom175.disabled  = enableFalg; document.forms[\"theForm\"].zoom200.disabled  = enableFalg; document.forms[\"theForm\"].zoom300.disabled  = enableFalg;document.forms[\"theForm\"].zoom400.disabled  = enableFalg;}\n"));
                sLinks.append(QString("function setFormat(formatSelection) {  newFormat =  formatSelection + \"?id\"; for ( var i = 0; i < document.links.length; i++){ document.links[i].href = document.links[i].href.replace(curFormat, newFormat);  document.links[i].text = document.links[i].text.replace(curFormat, newFormat); } curFormat = newFormat; if (curFormat == \"pdf?id\") disableRadioBtn(true); else disableRadioBtn(false);}\n"));
                //                sLinks.append(QString("function addExtension() { \n"));
                //                sLinks.append(QString("   if ( document.forms[\"theForm\"].checkbox1.checked == true ) {\n"));
                //                sLinks.append(QString("	      if (curFormat == \"pdf?id\") newExtension = \".pdf\";\n"));
                //                sLinks.append(QString("	      if (curFormat == \"image?id\") newExtension = \".png\";\n"));
                //                sLinks.append(QString("	      if (curFormat == \"ocr?id\") newExtension = \".html\";}\n"));
                //                sLinks.append(QString("   else { newExtension= \"\"; } "));
                //                sLinks.append(QString("   for ( var i = 0; i < document.links.length; i++){"));
                //                sLinks.append(QString("      if (document.forms[\"theForm\"].checkbox1.checked  == true && curExtension == \"\") { \n"));
                //                sLinks.append(QString("         document.links[i].href = document.links[i].href + newExtension;\n"));
                //                sLinks.append(QString("         document.links[i].text = document.links[i].text + newExtension; }\n"));
                //                sLinks.append(QString("      else { document.links[i].href = document.links[i].href.replace( curExtension, newExtension);\n"));
                //                sLinks.append(QString("             document.links[i].text = document.links[i].text.replace(curExtension, newExtension);}}\n"));
                //                sLinks.append(QString("     curExtension = newExtension; }"));
                sLinks.append(QString("</script></head>\n"));
            }
            sLinks.append(QString("<body>\n"));
            sLinks.append(QString("<table><tr><td><h1>Hathi Download Helper Link Export<h1></td><td><img src='file:///%1/elephant.png'/></td></tr></table>\n").arg(QDir::currentPath()));
            sLinks.append(QString("<div style='border-width:medium;border-color:#FF9999; border-style:double;padding:5px;'>\n"));
            sLinks.append(QString("With the extracted links you can download the files with any corresponding external download program.<br>"));
            sLinks.append(QString("Afterwards you can merge and convert all files into a single pdf file ( e.g. with PDFill Tools)"));
            sLinks.append(QString("<ul>\n"));
            sLinks.append(QString("<li>Download remark: Since the download capability of hathitrust.org is limited a 'temporary not available' message might occur during download.<br>"));
            sLinks.append(QString("The used downloader should be capable to recognize this. After a few minutes the downloads will be available again. Hence the downloader should provide an automatic retry function.</li>\n"));
            sLinks.append(QString("<li>The download tool should also be able to manage links without file extensions.</li><ul>\n</div>"));
            sLinks.append(QString("<hr>\n"));
            if (pRbDynamic->isChecked())
            {
                sLinks.append(QString("<noscript><b>Scripting of your browser is disabled! - Please enable java script for dynamic link adjustment:</b><br></noscript>\n"));
                sLinks.append(QString("<form action='' name='theForm'>\nFile format:<br>\n"));
                sLinks.append(QString("<input type='radio' value='download/pdf' name='format' onclick='setFormat(value)'> pdf "));
                sLinks.append(QString("<input type='radio' value='image' name='format' onclick='setFormat(value)' checked> image "));
                sLinks.append(QString("<input type='radio' value='ocr' name='format' onclick='setFormat(value)'> ocr text "));
                // sLinks.append(QString("<input type='checkbox' value='checked' name='checkbox1' onclick='addExtension()' checked>  Add file extension (*.png, *.pdf, *.html)<br>\n"));
                sLinks.append(QString("<br>Zoom level: <br>\n"));
                sLinks.append(QString("<input type='radio' value='50' id='zoom50' name='zoom' onclick='setZoom(value)'> 50% (48 dpi) \n"));
                sLinks.append(QString("<input type='radio' value='75' id='zoom75' name='zoom' onclick='setZoom(value)'> 75% (72 dpi) \n"));
                sLinks.append(QString("<input type='radio' value='100' id='zoom100' name='zoom' onclick='setZoom(value)'> 100% (96 dpi) \n"));
                sLinks.append(QString("<input type='radio' value='125' id='zoom125' name='zoom' onclick='setZoom(value)'> 125% (120 dpi) \n"));
                sLinks.append(QString("<input type='radio' value='150' id='zoom150' name='zoom' onclick='setZoom(value)'> 150% (144 dpi) \n"));
                sLinks.append(QString("<input type='radio' value='175' id='zoom175' name='zoom' onclick='setZoom(value)'> 175% (168 dpi) \n"));
                sLinks.append(QString("<input type='radio' value='200' id='zoom200' name='zoom' onclick='setZoom(value)' checked> 200% (192 dpi) \n"));
                sLinks.append(QString("<input type='radio' value='300' id='zoom300' name='zoom' onclick='setZoom(value)'> 300% (288 dpi) \n"));
                sLinks.append(QString("<input type='radio' value='400' id='zoom400' name='zoom' onclick='setZoom(value)'> 400% (384 dpi) \n"));
                sLinks.append(QString("</form>\n"));
                sLinks.append(QString("<hr>\n"));
            }

            QString sTitleClean = msDocumentID;
            removeReservedCharFromString(&sTitleClean);
            // QString sFileExt;
            QString sFileFormat;
            QString sZoom = QString("200");

            sLinks.append(QString("<h2>%1</h2><br>\n").arg(ui->labelBookTitleText->text()));
            sLinks.append(QString("Hathitrust URL:: <a href='https://babel.hathitrust.org/cgi/pt?id=%1' title='book_infopage_%1'>https://babel.hathitrust.org/cgi/pt?id=%1</a><br>\n").arg(msDocumentID));
            sLinks.append(QString("<br>Number of Pages: %1<br><br>\n").arg(miMaxPageNum));
            sLinks.append(QString("<br>\n"));

            if (pRbDynamic->isChecked())
            {
                // sFileExt = QString(".png");
                sFileFormat = QString("image");
                sLinks.append(QString("<h3>File link list:</h3><br>\n"));

            }
            else if (pRbImgStatic->isChecked())
            {
                // sFileExt = QString(".png");
                sFileFormat = QString("image");
                sZoom = QString::number(mpviZoomlevel->at(pCbZoom->currentIndex()));
                sLinks.append(QString("<h3>Image link list:</h3><br>\n"));

            }
            else if (pRbPdfStatic->isChecked())
            {
                // sFileExt = QString(".pdf");
                sFileFormat = QString("download/pdf");
                sLinks.append(QString("<h3>PDF link list:</h3><br>\n"));
            }
            else
            {
                //  sFileExt = QString(".html");
                sFileFormat = QString("ocr");
                sLinks.append(QString("<h3>OCR text link list:</h3><br>\n"));
            }

            for ( int i=0; i < miMaxPageNum; i++)
            {
                // Use plain text links  + filename after fileformat tag
                sLinks.append(QString("<a href='https://babel.hathitrust.org/cgi/imgsrv/%1?id=%2;size=%3;seq=%4' title='%5_page_%4'>https://babel.hathitrust.org/cgi/imgsrv/%1?id=%2;size=%3;seq=%4</a><br>\n").arg(sFileFormat).arg(msDocumentID).arg(sZoom).arg(i,0,10).arg(sTitleClean));
            }
            sLinks.append(QString("</body></html>\n"));

            // Ask user for filename and destination folder
            QString sFilename = ui->lineEditDstFolder->text();
            sFilename.append(QString("extractedLinks.html"));
#if defined(Q_OS_MAC)
            /* Workarround for issue: QTBUG-27664 */
            sFilename = QFileDialog::getSaveFileName( this,
                                                      "Save File",
                                                      sFilename,
                                                      "(*.html)", 0, QFileDialog::DontUseNativeDialog ).toUtf8();
#else

            sFilename = QFileDialog::getSaveFileName( this,
                                                      "Save File",
                                                      sFilename,
                                                      "(*.html)" );
#endif

            if (!sFilename.isEmpty())
            {
                writeFile(this, sFilename, &sLinks);
                showFileInFilemanager(sFilename);
            }
        }
    }
    else
    {
        // Do nothing
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: set book url
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_setBookUrl(QString sUrl)
{
    ui->lineEditBookURL->setText(sUrl);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: apply download settings for current batchjob
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_applyDownloadOptions(QString sSettings)
{
    // Check for key words
    if ( sSettings.contains("pdfs"))
        ui->radioButtonPdf->setChecked(true);
    else
        ui->radioButtonImages->setChecked(true);
    // OCR
    ui->checkBoxGetOcr->setChecked(sSettings.contains("OCR"));
    // Zoom
    for (int i=0;i < ui->comboBoxZoom->count();i++)
    {
        if ( sSettings.contains(ui->comboBoxZoom->itemText(i)))
        {
            ui->comboBoxZoom->setCurrentIndex(i);
            break;
        }
    }
    // Webproxy
    ui->checkBoxWebProxy->setChecked(sSettings.contains("webproxy"));

    // Autoproxy
    if (sSettings.contains("autoproxy"))
    {
        // Enable auto proxy
        mpAutoProxy->setAutoProxyState(true);
        mpProxyDialog->setAutoProxy(true);

        // Set country selection
        mpAutoProxy->setUsProxyOnlyState(sSettings.contains("US only"));
    }
    else
    {
        slot_setProxyState(false);
    }
    ui->checkBoxAutoProxy->setChecked(sSettings.contains("autoproxy"));

    // Set resume option
    ui->checkBoxResume->setChecked(sSettings.contains("resume"));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: set dst folder
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_setDstFolder(QString sFolder)
{
    ui->lineEditDstFolder->setText(sFolder);
    on_lineEditDstFolder_editingFinished();
}

/*-----------------------------------------------------------------------------------------------*
* GUI: apply pdf merge settings
*-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_applyMergeOptions(QString sSettings)
{
    if (sSettings.contains("do nothing"))
        ui->checkBoxAutoPdf->setChecked(false);
    else
    {
        ui->checkBoxAutoPdf->setChecked(true);
        if ( sSettings.contains("merge pdfs"))
            ui->radioButtonMergePdftk->setChecked(true);
        else if ( sSettings.contains("create pdf book"))
            ui->radioButtonMergeImages->setChecked(true);
        else
            ui->radioButtonConvertImages->setChecked(true);

        ui->checkBoxConvertOcrOnly->setChecked(sSettings.contains("ocr only"));
    }
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: Check path syntax for Dst folder
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_lineEditDstFolder_editingFinished()
{
    if (ui->lineEditDstFolder->text().isEmpty())
        ui->lineEditDstFolder->setText(mtUserSettings.sDestinationFolder);

    if (ui->lineEditDstFolder->text().at(ui->lineEditDstFolder->text().length()-1) == QChar('/'))
        ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().remove(ui->lineEditDstFolder->text().length()-1,1));

    while( ui->lineEditDstFolder->text().contains("/."))
        ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().replace("/.","/"));

    while( ui->lineEditDstFolder->text().contains("//"))
        ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().replace("//","/"));

    while( ui->lineEditDstFolder->text().right(1).contains(QString(" ")) || ui->lineEditDstFolder->text().right(1).contains(QString(".")))
    {
        ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().remove(ui->lineEditDstFolder->text().length()-1,1));
    }

    while( ui->lineEditDstFolder->text().left(1).contains(QString(" ")) || ui->lineEditDstFolder->text().left(1).contains(QString(".")))
    {
        ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().remove(0,1));
    }

    ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().mid(0, MAX_LENGTH_PATH-1));

    if ( !(ui->lineEditDstFolder->text().right(1).contains(QString("/"))))
    {
        ui->lineEditDstFolder->setText(ui->lineEditDstFolder->text().append(QString("/")));
    }

    ui->lineEditSrcFolder->setText(ui->lineEditDstFolder->text());
    ui->labelDownloadState->setText(QString(""));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Check path syntax for Dst folder User settings
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_lineEditDstFolderUserSettings_editingFinished()
{
    if (mpUserSettingsDialog->lineEditDstFolder->text().isEmpty())
        mpUserSettingsDialog->lineEditDstFolder->setText(mtUserSettings.sDestinationFolder);

    if (mpUserSettingsDialog->lineEditDstFolder->text().at(mpUserSettingsDialog->lineEditDstFolder->text().length()-1) == QChar('/'))
        mpUserSettingsDialog->lineEditDstFolder->setText(mpUserSettingsDialog->lineEditDstFolder->text().remove(mpUserSettingsDialog->lineEditDstFolder->text().length()-1,1));

    while( mpUserSettingsDialog->lineEditDstFolder->text().contains("/."))
        mpUserSettingsDialog->lineEditDstFolder->setText(mpUserSettingsDialog->lineEditDstFolder->text().replace("/.","/"));

    while( mpUserSettingsDialog->lineEditDstFolder->text().contains("//"))
        mpUserSettingsDialog->lineEditDstFolder->setText(mpUserSettingsDialog->lineEditDstFolder->text().replace("//","/"));

    while( mpUserSettingsDialog->lineEditDstFolder->text().right(1).contains(QString(" ")) || mpUserSettingsDialog->lineEditDstFolder->text().right(1).contains(QString(".")))
    {
        mpUserSettingsDialog->lineEditDstFolder->setText(mpUserSettingsDialog->lineEditDstFolder->text().remove(mpUserSettingsDialog->lineEditDstFolder->text().length()-1,1));
    }

    while( mpUserSettingsDialog->lineEditDstFolder->text().left(1).contains(QString(" ")) || mpUserSettingsDialog->lineEditDstFolder->text().left(1).contains(QString(".")))
    {
        mpUserSettingsDialog->lineEditDstFolder->setText(mpUserSettingsDialog->lineEditDstFolder->text().remove(0,1));
    }

    mpUserSettingsDialog->lineEditDstFolder->setText(mpUserSettingsDialog->lineEditDstFolder->text().mid(0, MAX_LENGTH_PATH-1));

    if ( !(mpUserSettingsDialog->lineEditDstFolder->text().right(1).contains(QString("/"))))
    {
        mpUserSettingsDialog->lineEditDstFolder->setText(mpUserSettingsDialog->lineEditDstFolder->text().append(QString("/")));
    }

    mpUserSettingsDialog->lineEditDstFolder->setText(mpUserSettingsDialog->lineEditDstFolder->text());
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: Check path syntax for Src folder
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_lineEditSrcFolder_editingFinished()
{
    if (ui->lineEditSrcFolder->text().isEmpty())
        ui->lineEditSrcFolder->setText(ui->lineEditDstFolder->text());

    while( ui->lineEditSrcFolder->text().right(1).contains(QString(" ")))
    {
        ui->lineEditSrcFolder->setText(ui->lineEditSrcFolder->text().remove(ui->lineEditSrcFolder->text().length()-1,1));
    }

    if ( !(ui->lineEditSrcFolder->text().right(1).contains(QString("/"))))
    {
        ui->lineEditSrcFolder->setText(ui->lineEditSrcFolder->text().append(QString("/")));
    }

    ui->progressBarCreatePdf->setValue(0);
    ui->labelCreatePdfState->setText(QString(""));
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Enable/Disable pdf merge and conversion settings
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::enablePdfMergeSettings(bool bState)
{
    mpStyleSubmenu->setEnabled(bState);
    ui->radioButtonMergeImages->setEnabled(bState);
    ui->radioButtonMergePdftk->setEnabled(bState);
    ui->radioButtonConvertImages->setEnabled(bState);
    ui->checkBoxConvertOcrOnly->setEnabled(bState);
    ui->comboBoxPrinterRes->setEnabled(bState);
    ui->comboBoxZoom->setEnabled(bState);
    ui->lineEditSrcFolder->setEnabled(bState);
    ui->toolButtonSrcFolder->setEnabled(bState);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Enable/Disable Download Settings while download
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_enableDownloadSettings(bool bState)
{
    ui->radioButtonImages->setEnabled(bState);
    ui->comboBoxZoom->setEnabled(bState);
    ui->radioButtonPdf->setEnabled(bState);
    ui->checkBoxGetOcr->setEnabled(bState);
    ui->lineEditPages->setEnabled(bState);
    ui->lineEditDstFolder->setEnabled(bState);
    ui->toolButtonDstFolder->setEnabled(bState);

    mpAutoProxy->setTargetUrl(QString(""));
    mpAutoProxy->enableLocalHost(!ui->checkBoxUseProxy->isChecked());

    if (bState)
    {
        // Reset bookinfo progress bar
        if (ui->checkBoxUseProxy->isChecked())
        {
            slot_infiniteProgressBarsBookPage(false);
            // ui->labelBookInfoState->setText("");
        }

        // Change button
        ui->pushButtonStartDownload->setText("Start download");
        disconnect(ui->pushButtonStartDownload,SIGNAL(clicked()),this, SLOT(slot_pushButtonCancelDownload_clicked()));
        connect(ui->pushButtonStartDownload, SIGNAL(clicked()), this, SLOT(on_pushButtonStartDownload_clicked()));

        connect(mpAutoProxy, SIGNAL(updateAutoProxyState(QString)), ui->labelBookInfoState, SLOT(setText(QString)));
        disconnect(mpAutoProxy, SIGNAL(updateAutoProxyState(QString)), ui->labelWebProxyState, SLOT(setText(QString)));
        ui->labelWebProxyState->setText("");
        connect(mpAutoProxy, SIGNAL(updateAutoProxyProgressBar(bool)), this, SLOT(slot_infiniteProgressBarsBookPage(bool)));

        ui->checkBoxUseProxy->setDisabled(false);
        // TODO: added due to possible application crash on WebProxy<->AutoProxy toggle
#if DISABLE_AUTOPROXY
        // DISALBLED IN HDH 1.1.1
        ui->checkBoxAutoProxy->setDisabled(false);
#endif
#if DISABLE_AUTOPROXY
        // DISALBLED IN HDH 1.1.1
        ui->checkBoxWebProxy->setDisabled(false);
#endif
        ui->mpMovieLeech->stop();
        ui->mpMovieLeech->jumpToFrame(0);
    }
    else
    {
        // Change button
        ui->pushButtonStartDownload->setText("Cancel download");
        disconnect(ui->pushButtonStartDownload,SIGNAL(clicked()), this, SLOT(on_pushButtonStartDownload_clicked()));
        connect(ui->pushButtonStartDownload, SIGNAL(clicked()), this, SLOT(slot_pushButtonCancelDownload_clicked()));

        mpAutoProxy->setTargetUrl(ui->lineEditBookURL->text());

        disconnect(mpAutoProxy, SIGNAL(updateAutoProxyState(QString)), ui->labelBookInfoState, SLOT(setText(QString)));
        connect(mpAutoProxy, SIGNAL(updateAutoProxyState(QString)), ui->labelWebProxyState, SLOT(setText(QString)));
        disconnect(mpAutoProxy, SIGNAL(updateAutoProxyProgressBar(bool)), this, SLOT(slot_infiniteProgressBarsBookPage(bool)));

        ui->checkBoxUseProxy->setDisabled(true);
        if (!ui->checkBoxUseProxy->isChecked())
            ui->labelBookInfoState->setText("Done.");

        if ( ui->checkBoxUseProxy->isChecked() && ui->checkBoxAutoProxy->isChecked())
            ui->labelWebProxyState->setText(QString("AutoProxy: <font color='green'>Connection established.</font> Proxy IP: <font color='blue'> " +  QNetworkProxy::applicationProxy().hostName() + "</font>"));

        // TODO: added due to possible application crash on WebProxy<->AutoProxy toggle
#if !ENABLE_PROXY_CHANGEOVER
        if (ui->checkBoxWebProxy->isChecked())
            ui->checkBoxAutoProxy->setDisabled(true);
        if (ui->checkBoxAutoProxy->isChecked())
            ui->checkBoxWebProxy->setDisabled(true);
#endif

        ui->mpMovieLeech->start();
    }

    if(mBatchJobActivated)
        ui->pushButtonStartDownload->setDisabled(bState);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Check lineedit for illegal characters
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_lineEditBookURL_editingFinished()
{
    if (ui->lineEditBookURL->text().contains("hdl.handle.net"))
        ui->lineEditBookURL->setText(ui->lineEditBookURL->text().mid(ui->lineEditBookURL->text().lastIndexOf("/")+1));

    ui->lineEditBookURL->setText(QUrl::fromPercentEncoding(ui->lineEditBookURL->text().toLatin1()).toLatin1());
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Check lineedit for illegal characters
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_lineEditPages_editingFinished()
{
    QString sPages = ui->lineEditPages->text();

    removeIllegalChar(&sPages);

    // Write back cleaned page selection
    ui->lineEditPages->setText(sPages);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: set progress bar
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_infiniteProgressBarsBookPage(bool bEnabled)
{
    if ((bEnabled))
    {
        if ( ui->progressBarBookPage->value() > (-1))
        {
            ui->progressBarBookPage->setMaximum(0);
            ui->progressBarBookPage->setMinimum(0);
            ui->progressBarBookPage->setValue(-1);
        }
    }
    else
    {
        ui->progressBarBookPage->setMaximum(1);
        ui->progressBarBookPage->setMinimum(0);
        ui->progressBarBookPage->setValue(0);
    }
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: Update GUI progress state for pdf conversion & merging
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_updatePdfMergeProgressBar(int i)
{
    ui->progressBarCreatePdf->setValue(i);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Update GUI progress state for pdf conversion & merging
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_updateMergeState(QString sState)
{
    ui->labelCreatePdfState->setText(sState);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Create message box requested by worker thread
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_createMessageBox(QString sMessage)
{
    if( QMessageBox::Yes == QMessageBox::question( this, "Unknown files!",
                                                   sMessage,
                                                   QMessageBox::Yes , QMessageBox::No ) )
    {
        // Signals for batch job handling
        if (mBatchJobActivated)
        {
            emit  abortThread(false);
        }
    }
    else
    {
        // Signals for batch job handling
        if (mBatchJobActivated)
        {
            emit abortThread(true);
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Open filemanager and show folder with selected file
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::showFileInFilemanager(QString sFile)
{
    sFile = QDir::toNativeSeparators(sFile);

#if defined(Q_OS_LINUX)
    sFile.prepend("\"");
    sFile.append("\"");
    QProcess::startDetached("nautilus " +  sFile  );  // e.g.:  nautilus "anyfolder\example.pdf"
#elif defined(Q_OS_WIN)
    QFile myFile(sFile);
    QFileInfo myFileInfo(myFile);
    QStringList params;
    if (myFileInfo.isFile())
        params.append( QLatin1String("/select,"));
    params.append(sFile);
    QProcess::startDetached("explorer.exe ", QStringList(params));  // e.g.:  explorer.exe anyfolder/select, example.pdf
#elif defined(Q_OS_MAC)
    // TODO: not tested!!
    QStringList params;
    //  int iFilenamePos = sFile.lastIndexOf("/");
    //  if (iFilenamePos < 1)
    //      iFilenamePos = 0;
    //  sFile.truncate(iFilenamePos);
    params.append("-R");
    params.append(sFile);
    QProcess::startDetached("open", QStringList(params)); // e.g.: /usr/bin/open  anyfolder/example.pdf
#elif  defined(Q_OS_FREEBSD)
    // TODO: not tested!!
    sFile.prepend("\"");
    sFile.append("\"");
    QProcess::startDetached("nautilus " +  sFile  );  // e.g.:  nautilus "anyfolder\example.pdf"
#endif
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: On toggle enable/disable checkboxfor ocr only
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::on_radioButtonMergePdftk_toggled(bool checked)
{
    ui->checkBoxConvertOcrOnly->setEnabled(!checked);
}


/*-----------------------------------------------------------------------------------------------*
 * Send User feedback via dialog form
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_sendFeeback()
{
    // Stop update request if still active
    if( (mpUpdateReply) && (mpUpdateReply->isOpen()) ) {
           mpUpdateReply->close();
    }

    // Check if download is active
    if (mpFileDownloader->getBusyState())
    {
        QMessageBox::warning(this, tr("Send feeback"),
                             tr("The Feedback form is not available during download."),
                             QMessageBox::Ok);
    }
    else
    {
        // feedback dialog
        QDialog * pSendFeedbackDialog   =   new QDialog(this);
        pSendFeedbackDialog->setModal(true);

        // Name input
        QLabel *pLabelName          =   new QLabel("Your Name:",pSendFeedbackDialog);
        QLineEdit *pLabelNameLine   =   new QLineEdit(pSendFeedbackDialog);
        pLabelNameLine->setPlaceholderText(QString("optional"));

        // Email input
        QLabel *pLabelEmail         =   new QLabel("Your Email:",pSendFeedbackDialog);
        QLineEdit *pLabelEmailLine  =   new QLineEdit(pSendFeedbackDialog);
        pLabelEmailLine->setPlaceholderText(QString("optional - required for replies"));

        // Radio buttons for feedback type: message or bug report
        QLabel *pLabelMsgType       =   new QLabel("Message type:",pSendFeedbackDialog);
        QRadioButton *pRbMsg        =   new QRadioButton("message",pSendFeedbackDialog);
        QRadioButton *pRbBugrpt     =   new QRadioButton("bug report",pSendFeedbackDialog);

        // MESSAGE ELEMENTS ----------------------------------------------------
        QGroupBox *pGbMessage       = new QGroupBox(pSendFeedbackDialog);
        pGbMessage->setTitle("Your message");
        QTextEdit *pMessageEdit     =   new QTextEdit(pGbMessage);
        QGridLayout *pvGridLayoutGpBxMsg    = new QGridLayout(pGbMessage);
        pvGridLayoutGpBxMsg->addWidget(pMessageEdit);
        pMessageEdit->setFocus();

        // BUGREPORT ELEMENTS ---------------------------------------------------
        // Groupbox for bug report
        QGroupBox *pGbBugReport             = new QGroupBox(pSendFeedbackDialog);
        pGbBugReport->setTitle("Bug report");
        QGridLayout *pvGridLayoutGpBxBug    = new QGridLayout(pGbBugReport);

        // Check for update hint
        QLabel *pLabelUpdateChk     =   new QLabel("Hint: Before reporting please make sure that you are running the latest version of HDH.",pGbBugReport);
        QPushButton *pPbChkUpdate   =   new QPushButton("check for update",pGbBugReport);
        connect(pPbChkUpdate,SIGNAL(clicked()), this, SLOT(slot_loadVersionFile()));

        // Start of bug report
        QLabel *pLabelBgrptTtl      =   new QLabel("Please try to answer the following questions:",pGbBugReport);

        // Question #1 - crash? : add checkboxes
        QLabel *pLabelBgrptQ1       =   new QLabel("1.) Did Hathi Download Helper (HDH) crash? If so, when?:",pLabelBgrptTtl);
        QCheckBox *pCbCrshStartup   =   new QCheckBox("a) At startup",  pLabelBgrptTtl);
        QCheckBox *pCbCrshGetInfo   =   new QCheckBox("b) On requesting book info",pLabelBgrptTtl);
        QCheckBox *pCbCrshOnDwnld   =   new QCheckBox("c) On download", pLabelBgrptTtl);
        QCheckBox *pCbCrshPdf       =   new QCheckBox("d) On pdf merging process",pLabelBgrptTtl);
        QCheckBox *pCbCrshOther     =   new QCheckBox("e) Other",       pLabelBgrptTtl);

        // Question #1a - URL? : add line edit
        QLabel *pLabelBgrptQ1a      =   new QLabel("1.a) If you have answered with b) or c) to the above question, please provide the hathitrust URL you have used:", pGbBugReport);
        pLabelBgrptQ1a->setWordWrap(true);
        QLineEdit *pLineUrl         =   new QLineEdit(pGbBugReport);
        pLineUrl->setPlaceholderText(QString("Enter Hathitrust.org book URL or book ID here"));

        // Question #2 - system info : add checkboxes
        QLabel *pLabelBgrptQ2       =   new QLabel("2.) Please allow HDH to add informations about your settings to this bug report. This will help to find the bug.");
        pLabelBgrptQ2->setWordWrap(true);
        QLabel *pLabelAddInfo       =   new QLabel("Add information about:", pGbBugReport);
        QCheckBox *pCbSystemInfo    =   new QCheckBox("Your system", pGbBugReport);
        pCbSystemInfo->setToolTip(QString("operating system, screen resultion, pdftk version"));
        QCheckBox *pCbUsrSttngs     =   new QCheckBox("User setting", pGbBugReport);         // guisetup.xml + pagesetup.xml + prylst.xml
        pCbUsrSttngs->setToolTip(QString("GUI and download setting: selected checkboxes and radiobuttons, etc."));
        QCheckBox *pCbBtchJb        =   new QCheckBox("Batch job content", pGbBugReport);    // tmpbtchjb.xml
        pCbBtchJb->setToolTip(QString("Content of batch job tab"));

        // Question #3 - describe: text field
        QLabel *pLabelBgrptQ3       =   new QLabel("3.) Please describe the bug in detail in the text field below. Is the bug reproducible? If so, how can I provoke it?", pGbBugReport);
        pLabelBgrptQ2->setWordWrap(true);
        QTextEdit *pBugReportEdit   =   new QTextEdit(pGbBugReport);
        //pBugReportEdit->setFocus();

        // Add elements to grid layout
        int iRow = 1;
        // Row : Add update check -------------------------------------------------
        pvGridLayoutGpBxBug->addWidget(pLabelUpdateChk,    iRow, 1, 1, 4);
        pvGridLayoutGpBxBug->addWidget(pPbChkUpdate,       iRow++, 5, 1, 1);
        // Row : Bug report title -------------------------------------------------
        pvGridLayoutGpBxBug->addWidget(pLabelBgrptTtl,     iRow++, 1, 1, 5);
        // Row : Bug report question #1 -------------------------------------------
        pvGridLayoutGpBxBug->addWidget(pLabelBgrptQ1,      iRow++, 1, 1, 5);
        pvGridLayoutGpBxBug->addWidget(pCbCrshStartup,     iRow,   1, 1, 1);
        pvGridLayoutGpBxBug->addWidget(pCbCrshGetInfo,    iRow,   2, 1, 1);
        pvGridLayoutGpBxBug->addWidget(pCbCrshOnDwnld,     iRow,   3, 1, 1);
        pvGridLayoutGpBxBug->addWidget(pCbCrshPdf,         iRow,   4, 1, 1);
        pvGridLayoutGpBxBug->addWidget(pCbCrshOther,       iRow++, 5, 1, 5);
        // Row : Bug report question #1a ------------------------------------------
        pvGridLayoutGpBxBug->addWidget(pLabelBgrptQ1a,     iRow++, 1, 1, 5);
        pvGridLayoutGpBxBug->addWidget(pLineUrl,          iRow++, 1, 1, 5);
        // Row : Bug report question #3 -------------------------------------------
        pvGridLayoutGpBxBug->addWidget(pLabelBgrptQ2,      iRow++, 1, 1, 5);
        // Row : Add checkboxes----------------------------------------------------
        pvGridLayoutGpBxBug->addWidget(pLabelAddInfo,      iRow,   1, 1, 1);
        pvGridLayoutGpBxBug->addWidget(pCbSystemInfo,      iRow,   2, 1, 1);
        pvGridLayoutGpBxBug->addWidget(pCbUsrSttngs,       iRow,   3, 1, 1);
        pvGridLayoutGpBxBug->addWidget(pCbBtchJb,          iRow++, 4, 1, 1);
        // Row : Add Text field ---------------------------------------------------
        pvGridLayoutGpBxBug->addWidget(pLabelBgrptQ3,      iRow++, 1, 1, 5);
        pvGridLayoutGpBxBug->addWidget(pBugReportEdit,     iRow++, 1, 1, 5);

        // Send, Close button
        QPushButton *pSend          =   new QPushButton("Send",pSendFeedbackDialog);
        connect(pSend, SIGNAL(clicked()), pSendFeedbackDialog, SLOT(accept()));
        QPushButton *pClose         =   new QPushButton("Close",pSendFeedbackDialog);
        connect(pClose, SIGNAL(clicked()), pSendFeedbackDialog, SLOT(reject()));

        // Setup layout of dialog ----------------------------------------------------------------------------------------------------
        QGridLayout *pvGridLayout       = new QGridLayout(pSendFeedbackDialog);
        QHBoxLayout *phButtonLayout     = new QHBoxLayout();
        QSpacerItem *pSpacing           = new QSpacerItem(2, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        iRow = 1;
        // Align form elements
        pvGridLayout->setMargin(20);
        // Row : Name -------------------------------------------------------------
        pvGridLayout->addWidget(pLabelName,         iRow,   1, 1, 1);
        pvGridLayout->addWidget(pLabelNameLine,     iRow++, 2, 1, 2);
        // Row - spacer -----------------------------------------------------------
        pvGridLayout->addItem(pSpacing,             iRow++, 1, 1, 1);
        // Row : Email  -----------------------------------------------------------
        pvGridLayout->addWidget(pLabelEmail,        iRow,   1, 1, 1);
        pvGridLayout->addWidget(pLabelEmailLine,    iRow++, 2, 1, 2);
        // Row - spacer -----------------------------------------------------------
        pvGridLayout->addItem(pSpacing,             iRow++, 1, 1, 1);
        // Row : Add message type selection ---------------------------------------
        pvGridLayout->addWidget(pLabelMsgType,      iRow,   1, 1, 1);
        pvGridLayout->addWidget(pRbMsg,             iRow,   2, 1, 1);
        pvGridLayout->addWidget(pRbBugrpt,          iRow++, 3, 1, 1);
        // Row - spacer -----------------------------------------------------------
        pvGridLayout->addItem(pSpacing,             iRow++, 1, 1, 1);
        // Row : Add group box for bug report -------------------------------------
        pvGridLayout->addWidget(pGbBugReport,       iRow++, 1, 1, 3);
        // Row : Add groupbox for messages ----------------------------------------
        pvGridLayout->addWidget(pGbMessage,         iRow++, 1, 1, 3);
        // Add buttons ------------------------------------------------------------
        phButtonLayout->addWidget(pSend);
        phButtonLayout->addWidget(pClose);
        pvGridLayout->addLayout(phButtonLayout,     iRow++, 3, 1, 1);
        // Set layout -------------------------------------------------------------
        pvGridLayout->setSizeConstraint(QLayout::SetFixedSize);
        // Set stretch
        pvGridLayout->setColumnStretch(1,0);
        pvGridLayout->setColumnStretch(2,1);
        pvGridLayout->setColumnStretch(3,1);
        // Add layout ---------------------------------------------------------------
        pSendFeedbackDialog->setLayout(pvGridLayout);

        // Add relationships for hiding bug report elements
        connect(pRbMsg,     SIGNAL(toggled(bool)), pGbBugReport,    SLOT(setHidden(bool)));
        connect(pRbBugrpt,  SIGNAL(toggled(bool)), pGbMessage,      SLOT(setHidden(bool)));
        connect(pRbBugrpt,  SIGNAL(clicked()),     pBugReportEdit,  SLOT(setFocus()));
        connect(pRbMsg,     SIGNAL(clicked()),     pMessageEdit,    SLOT(setFocus()));

        // Preselect message type
        pRbMsg->setChecked(true);

        // Show dialog
        if(pSendFeedbackDialog->exec() == QDialog::Accepted)
        {
            // Collect data:
            // Name and mail : will be added to message field
            QString sUserData;
            if (!pLabelNameLine->text().isEmpty())
                sUserData.append(pLabelNameLine->text());
            else
                sUserData.append("none");
            sUserData.append("\n");
            if (!pLabelEmailLine->text().isEmpty())
                sUserData.append(pLabelEmailLine->text().replace("@","(at)").replace(".","(dot)"));
            else
                sUserData.append("none(at)nowhere(dot)com");
            sUserData.append("\n");

            // Check which message type is selected
            QString sMessageData, sDebugData;
            sMessageData.append(sUserData);
            sMessageData.append("\n<---------------------->\n");
            if (pRbMsg->isChecked())
            {
                sMessageData.append(pMessageEdit->toPlainText());
                sDebugData.append("none");
            }
            else
            {
                sMessageData.append(pBugReportEdit->toPlainText());
                // Bug report -----------------------------------------------------------------------------
                // Check Bug report
                sDebugData.append(qApp->applicationName() + " " + qApp->applicationVersion() + "\n\n");
                sDebugData.append("Q1: crash?\n");
                if (pCbCrshStartup->isChecked())
                    sDebugData.append(pCbCrshStartup->text() + ", ");
                if (pCbCrshGetInfo->isChecked())
                    sDebugData.append(pCbCrshGetInfo->text() + ", ");
                if (pCbCrshOnDwnld->isChecked())
                    sDebugData.append(pCbCrshOnDwnld->text() + ", ");
                if (pCbCrshPdf->isChecked())
                    sDebugData.append(pCbCrshPdf->text() + ", ");
                if (pCbCrshOther->isChecked())
                    sDebugData.append(pCbCrshOther->text() + ", ");
                sDebugData.append("\n\nURL: ");
                if (!pLineUrl->text().isEmpty())
                    sDebugData.append(pLineUrl->text());
                else
                    sDebugData.append("no url");
                sDebugData.append("\n\nAdditional info:\n");
                // System info -----------------------------------------------------------------------------
                sDebugData.append("\n\nSystem info:\n");
                if (pCbSystemInfo->isChecked())
                {
                    sDebugData.append(osName());
                    sDebugData.append("\n");
#if defined(Q_OS_LINUX)
                    // Get 'lsb_release' output.
                    QProcess lsbProcess;
                    lsbProcess.start("sh", QStringList() << "-c" << "lsb_release -s -i -d -r", QIODevice::ReadOnly);
                    lsbProcess.waitForFinished();
                    QByteArray lsbOutput = lsbProcess.readAllStandardOutput();
                    sDebugData.append(QString(lsbOutput));
#elif defined(Q_OS_WIN)
                    sDebugData.append(QString(QSysInfo::WindowsVersion));
                    // Get 'ver' output.
                    QProcess verProcess;
                    verProcess.start("cmd", QStringList()  << "/c" << "ver", QIODevice::ReadOnly);
                    verProcess.waitForFinished();
                    QByteArray verOutput = verProcess.readAllStandardOutput();
                    sDebugData.append(QString(verOutput));
#elif defined(Q_OS_MAC)
                    sDebugData.append(QString(QSysInfo::MacintoshVersion));
                    // Get 'sw_vers' output.
                    QProcess sw_versProcess;
                    sw_versProcess.start("sh", QStringList() << "-c" << "sw_vers", QIODevice::ReadOnly);
                    sw_versProcess.waitForFinished();
                    QByteArray sw_versOutput = sw_versProcess.readAllStandardOutput();
                    sDebugData.append(QString(sw_versOutput));
#elif  defined(Q_OS_FREEBSD)
                    // Get 'freebsd-version' output.
                    QProcess freebsdProcess;
                    freebsdProcess.start("sh", QStringList() << "-c" << "freebsd-version", QIODevice::ReadOnly);
                    freebsdProcess.waitForFinished();
                    QByteArray freebsdOutput = freebsdProcess.readAllStandardOutput();
                    sDebugData.append(QString(freebsdOutput));
#endif

                    sDebugData.append("\n\npdftk --version:\n");
#if defined(Q_OS_LINUX)
                    // Merge Pdf files using pdftk application on LINUX
                    QString sPdftkVersionCmd = "/usr/bin/pdftk --version";
                    QProcess pdftk_vProcess;
                    pdftk_vProcess.start("sh", QStringList()  << "-c" << sPdftkVersionCmd, QIODevice::ReadOnly);
                    pdftk_vProcess.waitForFinished();
                    QByteArray pdftk_vOutput = pdftk_vProcess.readAllStandardOutput();
                    sDebugData.append(QString(pdftk_vOutput));
#elif defined(Q_OS_WIN)
                    QString sPdftkVersionCmd = qApp->applicationDirPath() + "\\pdftk\\pdftk --version";
                    QProcess pdftk_vProcess;
                    pdftk_vProcess.start("cmd", QStringList()  << "/c" << sPdftkVersionCmd, QIODevice::ReadOnly);
                    pdftk_vProcess.waitForFinished();
                    QByteArray pdftk_vOutput = pdftk_vProcess.readAllStandardOutput();
                    sDebugData.append(QString(pdftk_vOutput));
#elif defined(Q_OS_MAC)
                    QString sPdftkVersionCmd = "/usr/local/bin/pdftk --version";
                    QProcess pdftk_vProcess;
                    pdftk_vProcess.start("sh", QStringList()  << "-c" << sPdftkVersionCmd, QIODevice::ReadOnly);
                    pdftk_vProcess.waitForFinished();
                    QByteArray pdftk_vOutput = pdftk_vProcess.readAllStandardOutput();
                    sDebugData.append(QString(pdftk_vOutput));
#elif  defined(Q_OS_FREEBSD)
                    //TODO: NOT TESTED
                    QString sPdftkVersionCmd = "/usr/local/bin/pdftk --version";
                    QProcess pdftk_vProcess;
                    pdftk_vProcess.start("sh", QStringList()  << "-c" << sPdftkVersionCmd, QIODevice::ReadOnly);
                    pdftk_vProcess.waitForFinished();
                    QByteArray pdftk_vOutput = pdftk_vProcess.readAllStandardOutput();
                    sDebugData.append(QString(pdftk_vOutput));
#endif
                }
                else
                    sDebugData.append(" no system info -------------------------------------");

                // User settings -----------------------------------------------------------------------------
                sDebugData.append("\n\nUser settings:\n");
                if (pCbUsrSttngs->isChecked())
                {
                    // Append xml files: guisetup.xml + pagesetup.xml + prylst.xml
                    sDebugData.append("\n\n File: guisetup.xml ---------------------------------------\n");
                    sDebugData.append(readFile(QString("guisetup.xml")));
                    sDebugData.append("\n\n File: pagesetup.xml --------------------------------------\n");
                    sDebugData.append(readFile(QString("pagesetup.xml")));
                    sDebugData.append("\n\n File: prylst.xml -----------------------------------------\n");
                    sDebugData.append(readFile(QString("prylst.xml")));
                }
                else
                    sDebugData.append(" no user settings ------------------------------------");

                // Batch job -----------------------------------------------------------------------------
                sDebugData.append("\n\nBatch job content:\n");
                if (pCbUsrSttngs->isChecked())
                {
                    // Append xml file: tmpbtchjb.xml
                    sDebugData.append("\n\n File: tmpbtchjb.xml ---------------------------------------\n");
                    sDebugData.append(readFile(QString("tmpbtchjb.xml")));
                }
                else
                    sDebugData.append(" no Batch job content --------------------------------");
            }

            // Fill up networt request -------------------------------------------------------------------------
            QUrl url;
            // Use fake data for contact form fields
            url.addQueryItem("name","Winston Smith" );
            url.addQueryItem("email", "donttouchmyprivacy@bigbrother.com");
            // Add user data to message
            url.addQueryItem("message", sMessageData);
            url.addQueryItem("debug",   sDebugData);

            QByteArray data;
            data = url.encodedQuery();

            QNetworkRequest req(QUrl("https://www.enformed.io/txkzxf9r"));
            req.setRawHeader("Referer", "https://sourceforge.net/projects/hathidownloadhelper/");
            req.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/54.1");
            req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));
            req.setHeader(QNetworkRequest::ContentLengthHeader, data.length());

            if(mpUpdateReply)
                mpUpdateReply->deleteLater();
            mpUpdateReply = mpNetwUpdateManager->post(req, data);
            connect(mpUpdateReply, SIGNAL(finished()), this, SLOT(slot_sendFeebackFinished()));

            mpRetryTimerFeedback->start(15000);
            connect(mpRetryTimerFeedback , SIGNAL(timeout()), this, SLOT(slot_sendFeebackFinished()));

            // Create progress dialog
            QProgressDialog *pProgressDialog = new QProgressDialog(this);
            connect(pProgressDialog,    SIGNAL(canceled()), this,            SLOT(slot_sendFeebackFinished()));
            connect(mpUpdateReply,      SIGNAL(finished()), pProgressDialog, SLOT(accept()));

            pProgressDialog->setModal(true);
            pProgressDialog->setAutoClose(true);
            pProgressDialog->setMinimumDuration(1000);
            pProgressDialog->setWindowTitle("Contact");
            if (pRbMsg->isChecked())
                pProgressDialog->setLabelText("Sending message...");
            else
                pProgressDialog->setLabelText("Sending bug report...");
            pProgressDialog->setMaximum(0);
            pProgressDialog->setMinimum(0);
            pProgressDialog->setValue(-1);
            pProgressDialog->exec();
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Send User feedback via dialog form
 *-----------------------------------------------------------------------------------------------*/
void HathiDownloadHelper::slot_sendFeebackFinished()
{
    mpRetryTimerFeedback->stop();
    disconnect(mpRetryTimerFeedback , SIGNAL(timeout()),  this, SLOT(slot_sendFeebackFinished()));
    disconnect(mpUpdateReply, SIGNAL(finished()), this, SLOT(slot_sendFeebackFinished()));

    if( (mpUpdateReply != NULL) && (mpUpdateReply->isOpen()) ) {
           mpUpdateReply->close();
    }

    if (        mpUpdateReply->error()                                                                // error
            ||  mpUpdateReply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty()        // empty page
       )
    {
        QMessageBox::warning(this, tr("Transmission error"),
                             tr("An error has occured... transmission failed!"),
                             QMessageBox::Ok);
    }
    else
    {
        QMessageBox::warning(this, tr("Transmission completed"),
                             tr("The message was successfully sent. Thank you for your feedback!"),
                             QMessageBox::Ok);
    }
}
