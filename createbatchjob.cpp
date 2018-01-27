#include "createbatchjob.h"

#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QProgressBar>
#include <QTimer>
#include <QTime>
#include <QEvent>
#include <QApplication>

#include <QDebug>

#define MAX_NUM_RETIRES_LOADBOOKINFO 5

/*-----------------------------------------------------------------------------------------------*
 * Dialog class constructor
 *-----------------------------------------------------------------------------------------------*/
CreateBatchJob::CreateBatchJob(QWidget *parent, tUserSettings* ptUserSettings) :
    QWidget(parent),
    mptUserSettings(ptUserSettings)
{
    createBatchJobGui();
    createAddBookDialog();
    slot_loadTmpTableWidgetBatchJobs();
    mbAddNewItem = true;
    msLastJobFilename = mptUserSettings->sDestinationFolder + "/batchjob.xml";

    setWindowFlags( windowFlags() |Qt::WindowMinimizeButtonHint );
}

/*-----------------------------------------------------------------------------------------------*
 * Dialog class destructor
 *-----------------------------------------------------------------------------------------------*/
CreateBatchJob::~CreateBatchJob()
{
    //
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Set new fixed size of GUI on resize event
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::changeEvent  ( QEvent  *event )
{
    if( event->type() == QEvent::WindowStateChange )
    {
        if( isMinimized() )
        {
            emit minimize();
        }
        else
        {
            emit maximize();
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: create book info group box
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::createBatchJobGui()
{
    uiBatchJob = &GuiBatchJobDialog;

    // Buttons
    uiBatchJob->pushButtonStart         = new QPushButton(QString("Start download"));
    connect(uiBatchJob->pushButtonStart, SIGNAL(clicked()), this, SLOT(on_pushButtonStart_clicked()));
    QSpacerItem* spacer1                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    uiBatchJob->pushButtonAddBook       = new QPushButton(QString("Add book"));
    connect(uiBatchJob->pushButtonAddBook, SIGNAL(clicked()), this, SLOT(on_pushButtonAddBook_clicked()));
    uiBatchJob->pushButtonEditBook      = new QPushButton(QString("Edit book"));
    QSpacerItem* spacer2                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Minimum );
    connect(uiBatchJob->pushButtonEditBook, SIGNAL(clicked()), this, SLOT(on_pushButtonEditBook_clicked()));
    uiBatchJob->pushButtonRemoveBook    = new QPushButton(QString("Remove book"));
    connect(uiBatchJob->pushButtonRemoveBook, SIGNAL(clicked()), this, SLOT(on_pushButtonRemoveBook_clicked()));
    uiBatchJob->pushButtonRemoveAll     = new QPushButton(QString("Remove all"));
    connect(uiBatchJob->pushButtonRemoveAll, SIGNAL(clicked()), this, SLOT(on_pushButtonRemoveAll_clicked()));
    QSpacerItem* spacer3                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Minimum );
    uiBatchJob->pushButtonMoveUp        = new QPushButton(QString("Move up"));
    connect(uiBatchJob->pushButtonMoveUp, SIGNAL(clicked()), this, SLOT(on_pushButtonMoveUp_clicked()));
    uiBatchJob->pushButtonMoveDown      = new QPushButton(QString("Move down"));
    connect(uiBatchJob->pushButtonMoveDown, SIGNAL(clicked()), this, SLOT(on_pushButtonMoveDown_clicked()));

    QSpacerItem* spacer4                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    uiBatchJob->pushButtonSave          = new QPushButton(QString("Save"));
    connect(uiBatchJob->pushButtonSave, SIGNAL(clicked()), this, SLOT(on_pushButtonSave_clicked()));
    uiBatchJob->pushButtonLoad          = new QPushButton(QString("Load"));
    connect(uiBatchJob->pushButtonLoad, SIGNAL(clicked()), this, SLOT(on_pushButtonLoad_clicked()));

    // Layoutbox for buttons
    QVBoxLayout* pVLayoutBox = new QVBoxLayout();
    pVLayoutBox->addWidget(uiBatchJob->pushButtonStart);
    pVLayoutBox->addItem(spacer1);
    pVLayoutBox->addWidget(uiBatchJob->pushButtonAddBook);
    pVLayoutBox->addWidget(uiBatchJob->pushButtonEditBook);
    pVLayoutBox->addItem(spacer2);
    pVLayoutBox->addWidget(uiBatchJob->pushButtonRemoveBook);
    pVLayoutBox->addWidget(uiBatchJob->pushButtonRemoveAll);
    pVLayoutBox->addItem(spacer3);
    pVLayoutBox->addWidget(uiBatchJob->pushButtonMoveUp);
    pVLayoutBox->addWidget(uiBatchJob->pushButtonMoveDown);
    pVLayoutBox->addItem(spacer4);
    pVLayoutBox->addWidget(uiBatchJob->pushButtonSave);
    pVLayoutBox->addWidget(uiBatchJob->pushButtonLoad);

    // Table
    uiBatchJob->tableWidgetBatchJobs = new QTableWidget(this);
    uiBatchJob->tableWidgetBatchJobs->setColumnCount(6);
    QStringList sColumnHeaders;
    sColumnHeaders.append("Book ID");
    sColumnHeaders.append("Download settings");
    sColumnHeaders.append("Download folder");
    sColumnHeaders.append("Pdf merge & conversion");
    sColumnHeaders.append("Job state");
    sColumnHeaders.append("Progress");
    uiBatchJob->tableWidgetBatchJobs->setHorizontalHeaderLabels(sColumnHeaders);
    uiBatchJob->tableWidgetBatchJobs->verticalHeader()->setVisible(false);
    uiBatchJob->tableWidgetBatchJobs->setStyleSheet("QTableView {selection-background-color: red;}");
    uiBatchJob->tableWidgetBatchJobs->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    uiBatchJob->tableWidgetBatchJobs->setShowGrid(true);
    uiBatchJob->tableWidgetBatchJobs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    uiBatchJob->tableWidgetBatchJobs->setSelectionBehavior(QAbstractItemView::SelectRows);
    uiBatchJob->tableWidgetBatchJobs->setAlternatingRowColors(true);
    uiBatchJob->tableWidgetBatchJobs->setStyleSheet("alternate-background-color: light;");   

    mpHeaderView = new QHeaderView(Qt::Horizontal);
    uiBatchJob->tableWidgetBatchJobs->setHorizontalHeader(mpHeaderView);

#if QT_VERSION >= 0x050000
    mpHeaderView->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mpHeaderView->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mpHeaderView->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    mpHeaderView->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    mpHeaderView->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    mpHeaderView->setSectionResizeMode(5, QHeaderView::Stretch);
#else
    mpHeaderView->setResizeMode(0, QHeaderView::ResizeToContents);
    mpHeaderView->setResizeMode(1, QHeaderView::ResizeToContents);
    mpHeaderView->setResizeMode(2, QHeaderView::ResizeToContents);
    mpHeaderView->setResizeMode(3, QHeaderView::ResizeToContents);
    mpHeaderView->setResizeMode(4, QHeaderView::ResizeToContents);
    mpHeaderView->setResizeMode(5, QHeaderView::Stretch);
#endif

    connect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_numberOpenJobs()));
    connect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));

    // Horizontal layout
    QHBoxLayout *pHLayout = new QHBoxLayout();

    pHLayout->addWidget(uiBatchJob->tableWidgetBatchJobs);
    pHLayout->addLayout(pVLayoutBox);
    pHLayout->setSizeConstraint(QLayout::SetMinimumSize);

    this->setLayout(pHLayout);
    this->setWindowTitle("Batch Job Dialog");
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: create add book dialog
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::createAddBookDialog()
{
    mpUiAddBookDialog = new QDialog;
    mpUiAddBookDialog->setWindowTitle("Add New Book Item");
    uiAddBook = &GuiAddBookDialog;

    // Book information group box
    uiAddBook->groupBoxBookInfo    = new QGroupBox(tr("Book information"));
    uiAddBook->gridLayoutBookInfo = new QGridLayout;

    uiAddBook->labelBookURL        = new QLabel(tr("Book URL: "));
    uiAddBook->lineEditBookURL     = new QLineEdit();
    uiAddBook->lineEditBookURL->setPlaceholderText(QString("Enter Hathitrust.org book URL or book ID here"));
    uiAddBook->gridLayoutBookInfo->addWidget(uiAddBook->labelBookURL, 1, 0);
    uiAddBook->gridLayoutBookInfo->addWidget(uiAddBook->lineEditBookURL, 1, 1, 1, 4);

    uiAddBook->gridLayoutBookInfo->setColumnStretch(0, 1);
    uiAddBook->gridLayoutBookInfo->setColumnStretch(1, 5);
    uiAddBook->gridLayoutBookInfo->setColumnStretch(2, 10);
    uiAddBook->gridLayoutBookInfo->setColumnStretch(3, 5);
    uiAddBook->gridLayoutBookInfo->setColumnStretch(4, 5);
    uiAddBook->gridLayoutBookInfo->setColumnStretch(5, 1);

    uiAddBook->groupBoxBookInfo->setLayout(uiAddBook->gridLayoutBookInfo);

    // Download settings group box
    uiAddBook->groupBoxDownloadSettings      = new QGroupBox(tr("Download settings"));
    uiAddBook->gridLayoutDownloadSettings    = new QGridLayout;

    uiAddBook->labelFormat     = new QLabel(tr("Download pages as "));
    uiAddBook->radioButtonPdf  = new QRadioButton(tr("pdfs (1 pdf per page, searchable text,  watermark on left border,  max. 30 pages / 5 min)"));
    uiAddBook->radioButtonPdf->setChecked(mptUserSettings->bDownloadPdfs);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->labelFormat, 1, 0);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->radioButtonPdf, 1, 1, 1, 5);

    uiAddBook->radioButtonImages   = new QRadioButton(tr("images (1 file per page, pdf generation possible, fast download), image zoom:"));
    uiAddBook->radioButtonImages->setChecked(mptUserSettings->bDownloadImages);
    uiAddBook->comboBoxZoom        = new QComboBox(0);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->radioButtonImages, 2, 1, 1, 3);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->comboBoxZoom, 2, 4);
    connect(uiAddBook->radioButtonImages , SIGNAL(toggled(bool)),uiAddBook->comboBoxZoom  , SLOT(setEnabled(bool)));
    uiAddBook->comboBoxZoom ->setEnabled(mptUserSettings->bDownloadImages);

    uiAddBook->checkBoxGetOcr  = new QCheckBox(tr("download OCR text (required for creation of searchable pdfs from images)"));
    uiAddBook->checkBoxGetOcr->setChecked(mptUserSettings->bDownloadOcr);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->checkBoxGetOcr, 3, 1, 1, 5);

    uiAddBook->labelDstFolder      = new QLabel(tr("Destination folder: "));
    uiAddBook->lineEditDstFolder   = new QLineEdit(0);
    uiAddBook->lineEditDstFolder->setText(mptUserSettings->sDestinationFolder);
    uiAddBook->toolButtonDstFolder = new QToolButton(0);
    uiAddBook->toolButtonDstFolder->setText(tr("..."));
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->labelDstFolder, 5, 0);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->lineEditDstFolder, 5, 1, 1, 3);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->toolButtonDstFolder, 5, 4, 1, 1);

    uiAddBook->checkBoxWebProxy = new QCheckBox(tr("enable WebProxies"));
    uiAddBook->checkBoxWebProxy->setChecked(mptUserSettings->bEnableWebProxy);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->checkBoxWebProxy, 6, 1, 1, 1);

    uiAddBook->checkBoxResume = new QCheckBox(tr("resume book download"));
    uiAddBook->checkBoxResume->setChecked(mptUserSettings->bResume);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->checkBoxResume, 6, 2, 1, 1, Qt::AlignCenter);

    uiAddBook->checkBoxAutoProxy = new QCheckBox(tr("enable AutoProxy"));
    uiAddBook->checkBoxAutoProxy->setChecked(mptUserSettings->bEnableAutoProxy);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->checkBoxAutoProxy, 6, 3, 1, 1);
    uiAddBook->comboBoxProxyUsOnly        = new QComboBox(0);
    uiAddBook->comboBoxProxyUsOnly->addItem("US only");
    uiAddBook->comboBoxProxyUsOnly->addItem("all countries");
    uiAddBook->comboBoxProxyUsOnly->setCurrentIndex(0);
    uiAddBook->gridLayoutDownloadSettings->addWidget(uiAddBook->comboBoxProxyUsOnly, 6, 4, 1, 1);
    uiAddBook->comboBoxProxyUsOnly->setEnabled(mptUserSettings->bEnableAutoProxy);
    connect(uiAddBook->checkBoxAutoProxy , SIGNAL(toggled(bool)),uiAddBook->comboBoxProxyUsOnly  , SLOT(setEnabled(bool)));

    uiAddBook->gridLayoutDownloadSettings->setColumnStretch(0,1);
    uiAddBook->gridLayoutDownloadSettings->setColumnStretch(1,5);
    uiAddBook->gridLayoutDownloadSettings->setColumnStretch(2,10);
    uiAddBook->gridLayoutDownloadSettings->setColumnStretch(3,5);
    uiAddBook->gridLayoutDownloadSettings->setColumnStretch(4,5);
    uiAddBook->gridLayoutDownloadSettings->setColumnStretch(5,1);

    uiAddBook->groupBoxDownloadSettings->setLayout(uiAddBook->gridLayoutDownloadSettings);

    // Pdf & conversion group box
    uiAddBook->groupBoxMergeAndConversion    = new QGroupBox(tr("PDF merge && conversion"));
    uiAddBook->gridLayoutMergeAndConversion  = new QGridLayout;

    uiAddBook->labelOutputOptions     = new QLabel(tr("Output options: "));
    uiAddBook->radioButtonMergePdftk  = new QRadioButton(tr("merge pdfs"));
    uiAddBook->radioButtonMergePdftk->setChecked(mptUserSettings->bMergePdf);
    uiAddBook->gridLayoutMergeAndConversion->addWidget(uiAddBook->labelOutputOptions, 1, 0);
    uiAddBook->gridLayoutMergeAndConversion->addWidget(uiAddBook->radioButtonMergePdftk, 1, 1,1,5);

    uiAddBook->radioButtonMergeImages  = new QRadioButton(tr("convert && merge images to pdf book"));
    uiAddBook->radioButtonMergeImages->setChecked(mptUserSettings->bImage2Book);
    uiAddBook->labelPrinterRes         = new QLabel(tr("set pdf resolution: "));
    uiAddBook->comboBoxPrinterRes      = new QComboBox(0);
    uiAddBook->gridLayoutMergeAndConversion->addWidget(uiAddBook->radioButtonMergeImages, 2, 1,1,2);
   // TODO: This option might be added later
   // uiAddBook->gridLayoutMergeAndConversion->addWidget(uiAddBook->labelPrinterRes, 2, 3);
   // uiAddBook->gridLayoutMergeAndConversion->addWidget(uiAddBook->comboBoxPrinterRes, 2, 4);

    uiAddBook->radioButtonConvertImages    = new QRadioButton(tr("convert images to single pdf files"));
    uiAddBook->radioButtonConvertImages->setChecked(mptUserSettings->bImage2Pdf);
    uiAddBook->gridLayoutMergeAndConversion->addWidget(uiAddBook->radioButtonConvertImages, 3, 1,1,2);

    uiAddBook->checkBoxConvertOcrOnly = new QCheckBox(tr("use plain text (ocr text) only"));
    uiAddBook->checkBoxConvertOcrOnly->setChecked(mptUserSettings->bOcrOnly);
    uiAddBook->gridLayoutMergeAndConversion->addWidget(uiAddBook->checkBoxConvertOcrOnly, 4, 1,1,4);

    uiAddBook->radioButtonDoNothing  = new QRadioButton(tr("do nothing"));
    uiAddBook->gridLayoutMergeAndConversion->addWidget(uiAddBook->radioButtonDoNothing, 5, 1,1,5);

    uiAddBook->gridLayoutMergeAndConversion->setColumnStretch(0,1);
    uiAddBook->gridLayoutMergeAndConversion->setColumnStretch(1,5);
    uiAddBook->gridLayoutMergeAndConversion->setColumnStretch(2,10);
    uiAddBook->gridLayoutMergeAndConversion->setColumnStretch(3,5);
    uiAddBook->gridLayoutMergeAndConversion->setColumnStretch(4,5);
    uiAddBook->gridLayoutMergeAndConversion->setColumnStretch(5,1);

    uiAddBook->groupBoxMergeAndConversion->setLayout(uiAddBook->gridLayoutMergeAndConversion);

    QPushButton* pPushButtonOK = new QPushButton(QString("OK"));
    QPushButton* pPushButtonCancel = new QPushButton(QString("Cancel"));
    QHBoxLayout *pHLayout = new QHBoxLayout;
    pHLayout->setAlignment(Qt::AlignRight);
    pHLayout->addWidget(pPushButtonOK);
    pHLayout->addWidget(pPushButtonCancel);

    // Book information group box
    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->addWidget(uiAddBook->groupBoxBookInfo);
    pVLayout->addWidget(uiAddBook->groupBoxDownloadSettings);
    pVLayout->addWidget(uiAddBook->groupBoxMergeAndConversion);
    pVLayout->addLayout(pHLayout);

    mpUiAddBookDialog->setLayout(pVLayout);
    mpUiAddBookDialog->setModal(true);
    connect(pPushButtonOK, SIGNAL(clicked()), mpUiAddBookDialog, SLOT(accept()));
    connect(pPushButtonCancel, SIGNAL(clicked()), mpUiAddBookDialog, SLOT(reject()));
    connect(mpUiAddBookDialog, SIGNAL(accepted()), this, SLOT(slot_addBookItem()));

    // Create comboBox entries for zoom level
    QVector<int>* mpviZoomlevel = new QVector <int>;
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
    QVector<int>* mpviDpi = new QVector <int>;
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
    uiAddBook->comboBoxZoom->addItems(sComboListItems);
    uiAddBook->comboBoxZoom->setCurrentIndex(mptUserSettings->iZoom);
    uiAddBook->comboBoxPrinterRes->addItems(sComboListItems);
    uiAddBook->comboBoxPrinterRes->setCurrentIndex(mptUserSettings->iZoom);

    // Create dependencies
    connect(uiAddBook->radioButtonImages, SIGNAL(toggled(bool)), uiAddBook->checkBoxGetOcr, SLOT(setChecked(bool)));
    connect(uiAddBook->radioButtonImages, SIGNAL(toggled(bool)), uiAddBook->comboBoxZoom, SLOT(setEnabled(bool)));
    connect(uiAddBook->radioButtonImages, SIGNAL(toggled(bool)), uiAddBook->comboBoxZoom, SLOT(setEnabled(bool)));
    connect(uiAddBook->radioButtonImages, SIGNAL(toggled(bool)), uiAddBook->radioButtonMergePdftk, SLOT(setDisabled(bool)));
    connect(uiAddBook->radioButtonImages, SIGNAL(toggled(bool)), uiAddBook->radioButtonMergeImages, SLOT(setChecked(bool)));
    connect(uiAddBook->radioButtonMergePdftk, SIGNAL(toggled(bool)), uiAddBook->checkBoxConvertOcrOnly, SLOT(setDisabled(bool)));
    connect(uiAddBook->radioButtonPdf, SIGNAL(toggled(bool)), uiAddBook->radioButtonConvertImages, SLOT(setDisabled(bool)));
    connect(uiAddBook->radioButtonPdf, SIGNAL(toggled(bool)), uiAddBook->radioButtonMergeImages, SLOT(setDisabled(bool)));
    connect(uiAddBook->radioButtonPdf, SIGNAL(toggled(bool)), uiAddBook->radioButtonMergePdftk, SLOT(setChecked(bool)));
    connect(uiAddBook->radioButtonDoNothing, SIGNAL(toggled(bool)), uiAddBook->checkBoxConvertOcrOnly, SLOT(setDisabled(bool)));
    connect(uiAddBook->toolButtonDstFolder, SIGNAL(clicked()), this, SLOT(slot_toolButtonDstFolder_clicked()));
    connect(uiAddBook->checkBoxWebProxy, SIGNAL(clicked(bool)), this, SLOT(slot_disableAutoProxy(bool)));
    connect(uiAddBook->checkBoxAutoProxy, SIGNAL(clicked(bool)), this, SLOT(slot_disableWebProxy(bool)));

    uiAddBook->radioButtonDoNothing->setChecked(!mptUserSettings->bAutoPdfBook);

#if DISABLE_WEBPROXY
    // DISABLED IN HDH 1.1.1
    uiAddBook->checkBoxWebProxy->setChecked(false);
    uiAddBook->checkBoxWebProxy->setDisabled(true);
    uiAddBook->checkBoxWebProxy->setCheckable(false);
#endif
#if DISABLE_AUTOPROXY
    uiAddBook->checkBoxAutoProxy->setChecked(false);
    uiAddBook->checkBoxAutoProxy->setDisabled(true);
    uiAddBook->checkBoxAutoProxy->setCheckable(false);
#endif
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: adjust column grid size of GUI elements
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_adjustGuiGrid()
{
    int iMaxWidth = -1;

    for( int i = 0; i < 6; i++ )
    {
        iMaxWidth = uiAddBook->gridLayoutBookInfo->cellRect(0,i).width();

        if (iMaxWidth < uiAddBook->gridLayoutDownloadSettings->cellRect(0,i).width())
            iMaxWidth =  uiAddBook->gridLayoutDownloadSettings->cellRect(0,i).width();
        if (iMaxWidth < uiAddBook->gridLayoutMergeAndConversion->cellRect(0,i).width())
            iMaxWidth =  uiAddBook->gridLayoutMergeAndConversion->cellRect(0,i).width();

        uiAddBook->gridLayoutMergeAndConversion->setColumnMinimumWidth(i,iMaxWidth);
        uiAddBook->gridLayoutBookInfo->setColumnMinimumWidth(i,iMaxWidth);
        uiAddBook->gridLayoutDownloadSettings->setColumnMinimumWidth(i,iMaxWidth);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: set font size
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::setGuiFont(QFont guiFont)
{
    uiBatchJob->tableWidgetBatchJobs->setFont(guiFont);
}


/*-----------------------------------------------------------------------------------------------*
 * Copy from clipboard
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_clipboardChanged(QString sUrl)
{
    uiAddBook->lineEditBookURL->setText(sUrl);
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Update batchjob data to temporary file tmpbtchjb.xml
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::updateTmpTableWidgetBatchJobsFile()
{
    if (uiBatchJob->tableWidgetBatchJobs->rowCount() > 0)
        slot_saveTmpTableWidgetBatchJobs();
    else
    {
        QString sEmpty;
        writeFile(this, QString("tmpbtchjb.xml"), &sEmpty);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Write batchjob data to temporary file tmpbtchjb.xml
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_saveTmpTableWidgetBatchJobs()
{
    if (uiBatchJob->tableWidgetBatchJobs->rowCount() > 0)
    {
        QWidget *pTableItem = uiBatchJob->tableWidgetBatchJobs->cellWidget(uiBatchJob->tableWidgetBatchJobs->rowCount()-1, 5);
        if (pTableItem)
        {
            // Store jobs in tmpbtchjb.xml file
            QString sJobSetup;
            sJobSetup.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

            for (int iRow = 0; iRow <  uiBatchJob->tableWidgetBatchJobs->rowCount(); iRow++ )
            {
                sJobSetup.append("<job>\n");
                sJobSetup.append("<bookid>" +  uiBatchJob->tableWidgetBatchJobs->item(iRow,0)->text() + "</bookid>\n");
                sJobSetup.append("<downloadformat>" + uiBatchJob->tableWidgetBatchJobs->item(iRow,1)->text() + "</downloadformat>\n");
                sJobSetup.append("<dstfolder>" + uiBatchJob->tableWidgetBatchJobs->item(iRow,2)->text() + "</dstfolder>\n");
                sJobSetup.append("<mergeaction>" +uiBatchJob->tableWidgetBatchJobs->item(iRow,3)->text() + "</mergeaction>\n");
                sJobSetup.append("<jobstate>" + uiBatchJob->tableWidgetBatchJobs->item(iRow,4)->text() + "</jobstate>\n");

                if (uiBatchJob->tableWidgetBatchJobs->cellWidget(iRow,5)!= 0)
                {
                    const QProgressBar *pPgbar = dynamic_cast<const QProgressBar *>( uiBatchJob->tableWidgetBatchJobs->cellWidget(iRow,5) );
                    sJobSetup.append("<progress>" + QString::number(pPgbar->value()) + "/" + QString::number(pPgbar->maximum()) + "</progress>\n");
                }
                sJobSetup.append("</job>\n");
            }
            writeFile(this, QString("tmpbtchjb.xml"), &sJobSetup);
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Load batchjob data from temporyry file tmpbtchjb.xml
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_loadTmpTableWidgetBatchJobs()
{
    // Load open jobs from batchjob.xml file
    QString sJobSetup = readFile("tmpbtchjb.xml");

    int iStartFrom = 0;
    int iRow;
    myXmlReader(&sJobSetup, "job", &iStartFrom);
    disconnect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
    while(iStartFrom > 0 )
    {
        // Add new row and set items
        iRow = uiBatchJob->tableWidgetBatchJobs->rowCount();
        uiBatchJob->tableWidgetBatchJobs->insertRow(iRow);

        QTableWidgetItem *pBookIdItem = new QTableWidgetItem;
        pBookIdItem->setText(myXmlReader(&sJobSetup, "bookid", &iStartFrom));
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 0, pBookIdItem);

        QTableWidgetItem *pDownloadFormat = new QTableWidgetItem;
        pDownloadFormat->setText(myXmlReader(&sJobSetup, "downloadformat", &iStartFrom));
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 1, pDownloadFormat);

        QTableWidgetItem *pDstFolderItem = new QTableWidgetItem;
        pDstFolderItem->setText(myXmlReader(&sJobSetup, "dstfolder", &iStartFrom));
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 2, pDstFolderItem);

        QTableWidgetItem *pPdfMergeItem = new QTableWidgetItem;
        pPdfMergeItem->setText(myXmlReader(&sJobSetup, "mergeaction", &iStartFrom));
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 3, pPdfMergeItem);

        QTableWidgetItem *pJobStateItem = new QTableWidgetItem;
        pJobStateItem->setText(myXmlReader(&sJobSetup, "jobstate", &iStartFrom));
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 4, pJobStateItem);
        if (uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->text().contains("done", Qt::CaseInsensitive))
             uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->setBackgroundColor(Qt::green);

        if (pJobStateItem->text().contains("done", Qt::CaseInsensitive))
        {
                uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->setBackgroundColor(Qt::green);
        }
        else if (pJobStateItem->text().contains("download ok", Qt::CaseInsensitive))
        {
            // Do nothing
        }
        else
        {
            pJobStateItem->setText("open");
        }

        QString sProgress = myXmlReader(&sJobSetup, "progress", &iStartFrom);
        QProgressBar *pPgbar = new QProgressBar();
        pPgbar->setAlignment(Qt::AlignHCenter);
        if (sProgress.isEmpty())
        {
            pPgbar->setValue(50);
        }
        else
        {
            pPgbar->setValue(sProgress.mid(0,sProgress.indexOf("/")).toInt());
        }
        pPgbar->setMaximum(100);
        uiBatchJob->tableWidgetBatchJobs->setCellWidget(iRow, 5 ,pPgbar);

        // Check for further jobs
        myXmlReader(&sJobSetup, "job", &iStartFrom);
    }
    connect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
}

/*-----------------------------------------------------------------------------------------------*
 * Determine number of open jobs
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::numberOpenJobs(void)
{
    slot_numberOpenJobs();
}

/*-----------------------------------------------------------------------------------------------*
 * Determine number of open jobs
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_numberOpenJobs(void)
{
    int iNumJobs = 0;
    for (int iRow=0; iRow < uiBatchJob->tableWidgetBatchJobs->rowCount(); iRow++)
    {
        if (uiBatchJob->tableWidgetBatchJobs->item(iRow, 4) != 0)
            if(!uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->text().contains("Done",Qt::CaseInsensitive))
                iNumJobs++;
    }

    if (uiBatchJob->tableWidgetBatchJobs->rowCount() > 0)
    {
#if QT_VERSION >= 0x050000
        mpHeaderView->setSectionResizeMode(5, QHeaderView::ResizeToContents);
#else
        mpHeaderView->setResizeMode(5, QHeaderView::ResizeToContents);
#endif
    }
    else
    {
#if QT_VERSION >= 0x050000
        mpHeaderView->setSectionResizeMode(5, QHeaderView::Stretch);
#else
        mpHeaderView->setResizeMode(5, QHeaderView::Stretch);
#endif
    }

    emit numberOpenJobs(iNumJobs);
}
/*-----------------------------------------------------------------------------------------------*
 * GUI:  Open "add book" dialog
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_pushButtonAddBook_clicked()
{
    mpUiAddBookDialog->show();
    slot_adjustGuiGrid();
}

/*-----------------------------------------------------------------------------------------------*
 * GUI:  Open "edit book" dialog
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_pushButtonEditBook_clicked()
{
    int iRow = uiBatchJob->tableWidgetBatchJobs->selectionModel()->currentIndex().row();

    if (iRow < 0)
    {
        QMessageBox::information(this, tr("Edit Book"),tr("No item selected."));
    }
    else
    {
        mbAddNewItem = false;
        mpUiAddBookDialog->show();
        slot_adjustGuiGrid();

        // Set URL
        QString sBookUrl = QString("https://babel.hathitrust.org/cgi/pt?id=");
        sBookUrl.append( uiBatchJob->tableWidgetBatchJobs->item(iRow,0)->text());
        sBookUrl.append(";");
        uiAddBook->lineEditBookURL->setText(sBookUrl);

        // Set download format
        if (uiBatchJob->tableWidgetBatchJobs->item(iRow,1)->text().contains("pdfs"))
        {
            uiAddBook->radioButtonPdf->setChecked(true);
            uiAddBook->radioButtonMergeImages->setEnabled(false);
            uiAddBook->radioButtonConvertImages->setEnabled(false);
            uiAddBook->checkBoxConvertOcrOnly->setEnabled(false);
            uiAddBook->comboBoxZoom->setEnabled(false);
        }
        else
        {
            uiAddBook->radioButtonImages->setChecked(true);
            uiAddBook->radioButtonMergePdftk->setEnabled(false);
        }

        if ( uiBatchJob->tableWidgetBatchJobs->item(iRow,1)->text().contains("OCR") )
            uiAddBook->checkBoxGetOcr->setChecked(true);
        else
            uiAddBook->checkBoxGetOcr->setChecked(false);

         if ( uiBatchJob->tableWidgetBatchJobs->item(iRow,1)->text().contains("webproxy") )
             uiAddBook->checkBoxWebProxy->setChecked(true);
         else
             uiAddBook->checkBoxWebProxy->setChecked(false);

         if ( uiBatchJob->tableWidgetBatchJobs->item(iRow,1)->text().contains("resume") )
             uiAddBook->checkBoxResume->setChecked(true);
         else
             uiAddBook->checkBoxResume->setChecked(false);

         if ( uiBatchJob->tableWidgetBatchJobs->item(iRow,1)->text().contains("autoproxy") )
         {
             uiAddBook->checkBoxAutoProxy->setChecked(true);
             uiAddBook->comboBoxProxyUsOnly->setCurrentIndex(!uiBatchJob->tableWidgetBatchJobs->item(iRow,1)->text().contains("US only"));
         }
         else
             uiAddBook->checkBoxAutoProxy->setChecked(false);

        // Set destination folder
        uiAddBook->lineEditDstFolder->setText(uiBatchJob->tableWidgetBatchJobs->item(iRow,2)->text());

        // Set merge action
        uiAddBook->lineEditDstFolder->setText(uiBatchJob->tableWidgetBatchJobs->item(iRow,2)->text());

        // Set merge action
        if ( uiBatchJob->tableWidgetBatchJobs->item(iRow,3)->text().contains("merge pdfs"))
            uiAddBook->radioButtonMergePdftk->setChecked(true);
        else if (uiBatchJob->tableWidgetBatchJobs->item(iRow,3)->text().contains("create pdf book"))
            uiAddBook->radioButtonMergeImages->setChecked(true);
        else if (uiBatchJob->tableWidgetBatchJobs->item(iRow,3)->text().contains("create single pdfs"))
            uiAddBook->radioButtonConvertImages->setChecked(true);
        else
            uiAddBook->radioButtonDoNothing->setChecked(true);

        if (uiBatchJob->tableWidgetBatchJobs->item(iRow,3)->text().contains("ocr only"))
            uiAddBook->checkBoxConvertOcrOnly->setChecked(true);
    }
}


/*-----------------------------------------------------------------------------------------------*
 * GUI:  Add book to table list
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::addBookJob(QString sUrl)
{
    if ( (sUrl.contains("hathitrust.org") && sUrl.contains("pt?id="))
        || sUrl.contains("hdl.handle.net") )
    {
        // Book info
        QTableWidgetItem *pBookIdItem = new QTableWidgetItem;
        pBookIdItem->setText(extractBookIdFromUrl(sUrl));

        // Download settings
        QTableWidgetItem *pDownloadFormat = new QTableWidgetItem;
        QString sFormat;
        if (mptUserSettings->bDownloadPdfs)
            sFormat = QString("pdfs + ");
        else
            sFormat =  QString("images + ");

        if (mptUserSettings->bDownloadOcr)
            sFormat.append(QString("OCR + "));

        sFormat.append(uiAddBook->comboBoxZoom->itemText(mptUserSettings->iZoom));

        if (mptUserSettings->bEnableWebProxy)
            sFormat.append(QString("+ webproxy"));

        if (mptUserSettings->bResume)
            sFormat.append(QString("+ resume"));

        if (mptUserSettings->bEnableAutoProxy)
        {
            sFormat.append(QString("+ autoproxy + all countries"));
        }

        pDownloadFormat->setText(sFormat);

        // Dst folder
        QTableWidgetItem *pDstFolderItem = new QTableWidgetItem;
        if (mptUserSettings->bCreateDownloadFolder)
        {
            if (mptUserSettings->sDestinationFolder.contains("[bookTitle]") || mptUserSettings->sDestinationFolder.contains("[bookID]"))
                pDstFolderItem->setText(mptUserSettings->sDestinationFolder);
            else
            {
                if (mptUserSettings->bCreateDownloadFolderByTitle)
                    pDstFolderItem->setText(mptUserSettings->sDestinationFolder + "[bookTitle]/");
                else if (mptUserSettings->bCreateDownloadFolder)
                    pDstFolderItem->setText(mptUserSettings->sDestinationFolder+ "[bookID]/");
                else
                    pDstFolderItem->setText(mptUserSettings->sDestinationFolder);
            }
        }
        else
          pDstFolderItem->setText(mptUserSettings->sDestinationFolder);

        // PDF merge and conversion
        QTableWidgetItem *pPdfMergeItem = new QTableWidgetItem;
        QString sPdfMergeAction;

        if (mptUserSettings->bMergePdf)
            sPdfMergeAction = QString("merge pdfs");
        else if (mptUserSettings->bImage2Book)
            sPdfMergeAction = QString("create pdf book");
        else if (mptUserSettings->bImage2Pdf)
            sPdfMergeAction = QString("create single pdfs");
        else
            sPdfMergeAction = QString("do nothing");

        if (mptUserSettings->bOcrOnly && mptUserSettings->bDownloadOcr)
            sPdfMergeAction.append(QString(" + ocr only"));

        pPdfMergeItem->setText(sPdfMergeAction);

        // Job state
        QTableWidgetItem *pJobStateItem = new QTableWidgetItem;
        pJobStateItem->setText("open");

        QProgressBar *pPgbar = new QProgressBar();
        pPgbar->setAlignment(Qt::AlignHCenter);
        pPgbar->setValue(0);
        pPgbar->setMaximum(100);

        int iRow = 0;

        // Add new row and set items
        disconnect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
        iRow = uiBatchJob->tableWidgetBatchJobs->rowCount();
        uiBatchJob->tableWidgetBatchJobs->insertRow(iRow);

        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 0, pBookIdItem);
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 1, pDownloadFormat);
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 2, pDstFolderItem);
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 3, pPdfMergeItem);
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 4, pJobStateItem);
        uiBatchJob->tableWidgetBatchJobs->setCellWidget(iRow, 5 ,pPgbar);
        slot_saveTmpTableWidgetBatchJobs();
        connect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
    }
}


/*-----------------------------------------------------------------------------------------------*
 * GUI:  Add book item to table list
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_addBookItem()
{
    if ( (uiAddBook->lineEditBookURL->text().contains("hathitrust.org") && uiAddBook->lineEditBookURL->text().contains("pt?id="))
        || uiAddBook->lineEditBookURL->text().contains("hdl.handle.net") )
    {
        // Book info
        QTableWidgetItem *pBookIdItem = new QTableWidgetItem;
        pBookIdItem->setText(extractBookIdFromUrl(uiAddBook->lineEditBookURL->text()));

        // Download settings
        QTableWidgetItem *pDownloadFormat = new QTableWidgetItem;
        QString sFormat;
        if (uiAddBook->radioButtonPdf->isChecked())
            sFormat = QString("pdfs + ");
        else
            sFormat =  QString("images + ");

        if (  uiAddBook->checkBoxGetOcr->isChecked())
            sFormat.append(QString("OCR + "));

        sFormat.append(uiAddBook->comboBoxZoom->currentText());

        if (  uiAddBook->checkBoxWebProxy->isChecked())
            sFormat.append(QString("+ webproxy"));

        if (  uiAddBook->checkBoxResume->isChecked())
            sFormat.append(QString("+ resume"));

        if (  uiAddBook->checkBoxAutoProxy->isChecked())
        {
            sFormat.append(QString("+ autoproxy + "));
            sFormat.append(uiAddBook->comboBoxProxyUsOnly->currentText());
        }

        pDownloadFormat->setText(sFormat);

        // Dst folder
        QTableWidgetItem *pDstFolderItem = new QTableWidgetItem;
        if (uiAddBook->lineEditDstFolder->text().at(uiAddBook->lineEditDstFolder->text().length()-1) != QChar('/'))
            uiAddBook->lineEditDstFolder->setText(uiAddBook->lineEditDstFolder->text() + "/");

        if (mptUserSettings->bCreateDownloadFolder)
        {
            if (uiAddBook->lineEditDstFolder->text().contains("[bookTitle]") || uiAddBook->lineEditDstFolder->text().contains("[bookID]"))
                pDstFolderItem->setText(uiAddBook->lineEditDstFolder->text());
            else
            {
                if (mptUserSettings->bCreateDownloadFolderByTitle)
                    pDstFolderItem->setText(uiAddBook->lineEditDstFolder->text() + "[bookTitle]/");
                else if (mptUserSettings->bCreateDownloadFolder)
                    pDstFolderItem->setText(uiAddBook->lineEditDstFolder->text() + "[bookID]/");
                else
                    pDstFolderItem->setText(uiAddBook->lineEditDstFolder->text());
            }
        }
        else
             pDstFolderItem->setText(uiAddBook->lineEditDstFolder->text());

        // PDF merge and conversion
        QTableWidgetItem *pPdfMergeItem = new QTableWidgetItem;
        QString sPdfMergeAction;

        if (uiAddBook->radioButtonMergePdftk->isChecked())
            sPdfMergeAction = QString("merge pdfs");
        else if (uiAddBook->radioButtonMergeImages->isChecked())
            sPdfMergeAction = QString("create pdf book");
        else if (uiAddBook->radioButtonConvertImages->isChecked())
            sPdfMergeAction = QString("create single pdfs");
        else
            sPdfMergeAction = QString("do nothing");

        if (uiAddBook->checkBoxConvertOcrOnly->isEnabled() && uiAddBook->checkBoxConvertOcrOnly->isChecked())
            sPdfMergeAction.append(QString(" + ocr only"));

        pPdfMergeItem->setText(sPdfMergeAction);

        // Job state
        QTableWidgetItem *pJobStateItem = new QTableWidgetItem;
        pJobStateItem->setText("open");

        QProgressBar *pPgbar = new QProgressBar();
        pPgbar->setAlignment(Qt::AlignHCenter);
        pPgbar->setValue(0);
        pPgbar->setMaximum(100);
        disconnect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
        int iRow = 0;
        if (mbAddNewItem)
        {
            // Add new row and set items
            iRow = uiBatchJob->tableWidgetBatchJobs->rowCount();
            uiBatchJob->tableWidgetBatchJobs->insertRow(iRow);
        }
        else
        {
            iRow = uiBatchJob->tableWidgetBatchJobs->selectionModel()->currentIndex().row();
        }

        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 0, pBookIdItem);
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 1, pDownloadFormat);
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 2, pDstFolderItem);
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 3, pPdfMergeItem);
        uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 4, pJobStateItem);
        uiBatchJob->tableWidgetBatchJobs->setCellWidget(iRow, 5 ,pPgbar);
        slot_saveTmpTableWidgetBatchJobs();
        connect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
    }

    // Reset elements
    mbAddNewItem = true;
    uiAddBook->lineEditBookURL->setText("");
    uiAddBook->lineEditDstFolder->setText(QDir::homePath() + QString("/Downloads/"));

    // Bring Hahti Download Helper to front
#if defined(Q_OS_WIN)
    QWidget::activateWindow();
#elif defined(Q_OS_LINUX)
    QWidget::raise();
#elif defined(Q_WS_MAC)
    QWidget::raise();
#elif  defined(Q_OS_FREEBSD)
    QWidget::raise();
#else
    QWidget::raise();
#endif
}

/*-----------------------------------------------------------------------------------------------*
 * Extract book ID from url
 *-----------------------------------------------------------------------------------------------*/
QString CreateBatchJob::extractBookIdFromUrl(QString sUrl)
{
    sUrl.append(";");
    int iStartPos  = sUrl.indexOf(QString("pt?id=")) + 6;
    int iStopPos   = sUrl.indexOf(QString(";"),iStartPos);

    return sUrl.mid(iStartPos, iStopPos-iStartPos);
}


/*-----------------------------------------------------------------------------------------------*
 * GUI: Download directory selection dialog
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_toolButtonDstFolder_clicked()
{
    QString sDirectory = QFileDialog::getExistingDirectory(this, tr("Choose Or Create Directory"),
                                                           uiAddBook->lineEditDstFolder->text(),
                                                           QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog );
    if (!sDirectory.isEmpty())
    {
        uiAddBook->lineEditDstFolder->setText(sDirectory);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI:  Remove first or selected book table item
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_pushButtonRemoveBook_clicked()
{
     int iRow = uiBatchJob->tableWidgetBatchJobs->selectionModel()->currentIndex().row();
     uiBatchJob->tableWidgetBatchJobs->removeRow(iRow);
     slot_numberOpenJobs();
     updateTmpTableWidgetBatchJobsFile();

     if (uiBatchJob->tableWidgetBatchJobs->rowCount() > 0)
     {
        if (iRow >= uiBatchJob->tableWidgetBatchJobs->rowCount())
            iRow--;
        uiBatchJob->tableWidgetBatchJobs->selectRow(iRow);
        uiBatchJob->tableWidgetBatchJobs->setFocus();
     }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI:  Remove all books from table
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_pushButtonRemoveAll_clicked()
{
     uiBatchJob->tableWidgetBatchJobs->setRowCount(0);
     slot_numberOpenJobs();
     updateTmpTableWidgetBatchJobsFile();
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Move item up
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_pushButtonMoveUp_clicked()
{
    int iCurrentRow = uiBatchJob->tableWidgetBatchJobs->currentRow();

    if (iCurrentRow > 0 )
    {
        disconnect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));

        QTableWidgetItem* pTableWidgetMoveUpItem;
        QTableWidgetItem* pTableWidgetMoveDownItem;
        for (int iCol = 0; iCol < uiBatchJob->tableWidgetBatchJobs->columnCount()-1; iCol++)
        {
            pTableWidgetMoveUpItem = uiBatchJob->tableWidgetBatchJobs->takeItem(iCurrentRow, iCol);
            pTableWidgetMoveDownItem = uiBatchJob->tableWidgetBatchJobs->takeItem(iCurrentRow-1, iCol);

            uiBatchJob->tableWidgetBatchJobs->setItem(iCurrentRow, iCol, pTableWidgetMoveDownItem);
            uiBatchJob->tableWidgetBatchJobs->setItem(iCurrentRow-1, iCol, pTableWidgetMoveUpItem);
        }
        // Swap progress bar values
        QProgressBar *pWidgetMoveUp = dynamic_cast<QProgressBar *>(uiBatchJob->tableWidgetBatchJobs->cellWidget(iCurrentRow, uiBatchJob->tableWidgetBatchJobs->columnCount()-1));
        QProgressBar *pWidgetMoveDown = dynamic_cast<QProgressBar *>(uiBatchJob->tableWidgetBatchJobs->cellWidget(iCurrentRow-1, uiBatchJob->tableWidgetBatchJobs->columnCount()-1));
        int iProgressMoveUpValue     = pWidgetMoveUp->value();
        int iProgressMoveDownValue   = pWidgetMoveDown->value();
        int iProgressMoveUpValueMax   = pWidgetMoveUp->maximum();
        int iProgressMoveDownValueMax = pWidgetMoveDown->maximum();

        pWidgetMoveUp->setValue(iProgressMoveDownValue);
        pWidgetMoveUp->setMaximum(iProgressMoveDownValueMax);
        pWidgetMoveDown->setValue(iProgressMoveUpValue);
        pWidgetMoveDown->setMaximum(iProgressMoveUpValueMax);

        uiBatchJob->tableWidgetBatchJobs->selectRow(iCurrentRow-1);

        connect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
        slot_saveTmpTableWidgetBatchJobs();
    }

    uiBatchJob->tableWidgetBatchJobs->setFocus();
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Move item down
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_pushButtonMoveDown_clicked()
{

    int iCurrentRow = uiBatchJob->tableWidgetBatchJobs->currentRow();

    if (iCurrentRow < uiBatchJob->tableWidgetBatchJobs->rowCount()-1 )
    {
        disconnect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));

        QTableWidgetItem* pTableWidgetMoveUpItem;
        QTableWidgetItem* pTableWidgetMoveDownItem;
        for (int iCol = 0; iCol < uiBatchJob->tableWidgetBatchJobs->columnCount(); iCol++)
        {
            pTableWidgetMoveUpItem = uiBatchJob->tableWidgetBatchJobs->takeItem(iCurrentRow+1, iCol);
            pTableWidgetMoveDownItem = uiBatchJob->tableWidgetBatchJobs->takeItem(iCurrentRow, iCol);

            uiBatchJob->tableWidgetBatchJobs->setItem(iCurrentRow+1, iCol, pTableWidgetMoveDownItem);
            uiBatchJob->tableWidgetBatchJobs->setItem(iCurrentRow, iCol, pTableWidgetMoveUpItem);
        }
        // Swap progress bar values
        QProgressBar *pWidgetMoveUp = dynamic_cast<QProgressBar *>(uiBatchJob->tableWidgetBatchJobs->cellWidget(iCurrentRow+1, uiBatchJob->tableWidgetBatchJobs->columnCount()-1));
        QProgressBar *pWidgetMoveDown = dynamic_cast<QProgressBar *>(uiBatchJob->tableWidgetBatchJobs->cellWidget(iCurrentRow, uiBatchJob->tableWidgetBatchJobs->columnCount()-1));
        int iProgressMoveUpValue     = pWidgetMoveUp->value();
        int iProgressMoveDownValue   = pWidgetMoveDown->value();
        int iProgressMoveUpValueMax   = pWidgetMoveUp->maximum();
        int iProgressMoveDownValueMax = pWidgetMoveDown->maximum();

        pWidgetMoveUp->setValue(iProgressMoveDownValue);
        pWidgetMoveUp->setMaximum(iProgressMoveDownValueMax);
        pWidgetMoveDown->setValue(iProgressMoveUpValue);
        pWidgetMoveDown->setMaximum(iProgressMoveUpValueMax);

        uiBatchJob->tableWidgetBatchJobs->selectRow(iCurrentRow+1);

        connect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
        slot_saveTmpTableWidgetBatchJobs();
    }

            uiBatchJob->tableWidgetBatchJobs->setFocus();
}


/*-----------------------------------------------------------------------------------------------*
 * GUI:  Save book list in xml file
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_pushButtonSave_clicked()
{
    if (uiBatchJob->tableWidgetBatchJobs->rowCount() > 0)
    {
        // Ask user for filename and destination folder
        QString sFilename = msLastJobFilename;
#if defined(Q_OS_MAC)
                /* Workarround for issue: QTBUG-27664 */
                sFilename = QFileDialog::getSaveFileName( this,
                                                        "Save File",
                                                        sFilename,
                                                        "(*.xml)", 0, QFileDialog::DontUseNativeDialog ).toUtf8();
#else

        sFilename = QFileDialog::getSaveFileName( this,
                                      "Save File",
                                     sFilename,
                                      "(*.xml)" );
#endif
        if(!sFilename.isEmpty())
        {
            msLastJobFilename = sFilename;

            // Store jobs in batchjob.xml file
            QString sJobSetup;
            sJobSetup.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

            for (int iRow = 0; iRow <  uiBatchJob->tableWidgetBatchJobs->rowCount(); iRow++ )
            {
                sJobSetup.append("<job>\n");
                sJobSetup.append("<bookid>" +  uiBatchJob->tableWidgetBatchJobs->item(iRow,0)->text() + "</bookid>\n");
                sJobSetup.append("<downloadformat>" + uiBatchJob->tableWidgetBatchJobs->item(iRow,1)->text() + "</downloadformat>\n");
                sJobSetup.append("<dstfolder>" + uiBatchJob->tableWidgetBatchJobs->item(iRow,2)->text() + "</dstfolder>\n");
                sJobSetup.append("<mergeaction>" +uiBatchJob->tableWidgetBatchJobs->item(iRow,3)->text() + "</mergeaction>\n");
                sJobSetup.append("<jobstate>" + uiBatchJob->tableWidgetBatchJobs->item(iRow,4)->text() + "</jobstate>\n");

                if (uiBatchJob->tableWidgetBatchJobs->cellWidget(iRow,5)!= 0)
                {
                    const QProgressBar *pPgbar = dynamic_cast<const QProgressBar *>( uiBatchJob->tableWidgetBatchJobs->cellWidget(iRow,5) );
                    sJobSetup.append("<progress>" + QString::number(pPgbar->value()) + "/" + QString::number(pPgbar->maximum()) + "</progress>\n");
                }
                sJobSetup.append("</job>\n");
            }
            if (writeFile(this, sFilename, &sJobSetup))
                 QMessageBox::information(this, tr("Write to file"),tr("Job saved."));
        }
    }
    else
    {
        QMessageBox::information(this, tr("Write to file"),tr("Empty book list! No data to be saved."));
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Load jobs from file
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_pushButtonLoad_clicked()
{
    // Ask user for filename and source folder
    QString sFilename = msLastJobFilename;
#if defined(Q_OS_MAC)
    /* Workarround for issue: QTBUG-27664 */
    sFilename = QFileDialog::getOpenFileName( this,
                                              "Load File",
                                              sFilename,
                                              "(*.xml)", 0, QFileDialog::DontUseNativeDialog ).toUtf8();
#else

    sFilename = QFileDialog::getOpenFileName( this,
                                  "Load File",
                                 sFilename,
                                  "(*.xml)" );
#endif
    if(!sFilename.isEmpty())
    {
        msLastJobFilename = sFilename;

        // Load open jobs from batchjob.xml file
        QString sJobSetup = readFile(sFilename);

        int iStartFrom = 0;
        int iRow;
        myXmlReader(&sJobSetup, "job", &iStartFrom);
        disconnect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
        while(iStartFrom > 0 )
        {
            // Add new row and set items
            iRow = uiBatchJob->tableWidgetBatchJobs->rowCount();
            uiBatchJob->tableWidgetBatchJobs->insertRow(iRow);

            QTableWidgetItem *pBookIdItem = new QTableWidgetItem;
            pBookIdItem->setText(myXmlReader(&sJobSetup, "bookid", &iStartFrom));
            uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 0, pBookIdItem);

            QTableWidgetItem *pDownloadFormat = new QTableWidgetItem;
            pDownloadFormat->setText(myXmlReader(&sJobSetup, "downloadformat", &iStartFrom));
            uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 1, pDownloadFormat);

            QTableWidgetItem *pDstFolderItem = new QTableWidgetItem;
            pDstFolderItem->setText(myXmlReader(&sJobSetup, "dstfolder", &iStartFrom));
            uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 2, pDstFolderItem);

            QTableWidgetItem *pPdfMergeItem = new QTableWidgetItem;
            pPdfMergeItem->setText(myXmlReader(&sJobSetup, "mergeaction", &iStartFrom));
            uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 3, pPdfMergeItem);

            QTableWidgetItem *pJobStateItem = new QTableWidgetItem;
            pJobStateItem->setText(myXmlReader(&sJobSetup, "jobstate", &iStartFrom));
            uiBatchJob->tableWidgetBatchJobs->setItem(iRow, 4, pJobStateItem);

            if (pJobStateItem->text().contains("done", Qt::CaseInsensitive))
            {
                uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->setBackgroundColor(Qt::green);
            }
            else if (pJobStateItem->text().contains("download ok", Qt::CaseInsensitive))
            {
                // Do nothing
            }
            else
                pJobStateItem->setText("open");

            QString sProgress = myXmlReader(&sJobSetup, "progress", &iStartFrom);
            QProgressBar *pPgbar = new QProgressBar();
            pPgbar->setAlignment(Qt::AlignHCenter);
            if (sProgress.isEmpty())
            {
                pPgbar->setValue(50);
            }
            else
            {
                pPgbar->setValue(sProgress.mid(0,sProgress.indexOf("/")).toInt());
            }
            pPgbar->setMaximum(100);
            uiBatchJob->tableWidgetBatchJobs->setCellWidget(iRow, 5 ,pPgbar);

            // Check for further jobs
            myXmlReader(&sJobSetup, "job", &iStartFrom);
        }
        slot_saveTmpTableWidgetBatchJobs();
        connect(uiBatchJob->tableWidgetBatchJobs, SIGNAL(cellChanged(int,int)), this, SLOT(slot_saveTmpTableWidgetBatchJobs()));
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Start download job
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::startBatchJob()
{
    on_pushButtonStart_clicked();
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Start download job
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_pushButtonStart_clicked()
{
    static bool bRestartRequested = false;

    toggleStartCancelButton(true);
    mbJobCanceled = false;
    miLoadBookInfoRetries = 0;
    uiBatchJob->pushButtonAddBook->setEnabled(false);
    uiBatchJob->pushButtonRemoveBook->setEnabled(false);
    uiBatchJob->pushButtonLoad->setEnabled(false);
    uiBatchJob->pushButtonSave->setEnabled(false);
    uiBatchJob->pushButtonMoveUp->setEnabled(false);
    uiBatchJob->pushButtonMoveDown->setEnabled(false);

    // On restart request
    if (bRestartRequested)
    {
        bRestartRequested = false;
        for(int j = 0; j < uiBatchJob->tableWidgetBatchJobs->rowCount(); j++)
        {
            if (uiBatchJob->tableWidgetBatchJobs->item(j, 4)->text().contains("failed"))
                uiBatchJob->tableWidgetBatchJobs->item(j, 4)->setText("open");
        }
    }

    // Find first open job
    int i;
    for (i = 0; i < uiBatchJob->tableWidgetBatchJobs->rowCount(); i++)
    {
        if (uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("open") ||
            uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("canceled") )
        {
            // Scroll to right side
            uiBatchJob->tableWidgetBatchJobs->setCurrentCell(i,5);
            uiBatchJob->tableWidgetBatchJobs->scrollToItem(uiBatchJob->tableWidgetBatchJobs->item(i, 5), QAbstractItemView::EnsureVisible);

            // Collect download information
            QString sBookUrl = QString("https://babel.hathitrust.org/cgi/pt?id=");
            sBookUrl.append( uiBatchJob->tableWidgetBatchJobs->item(i,0)->text());
            sBookUrl.append(";");
            QString sDownloadSettings   = uiBatchJob->tableWidgetBatchJobs->item(i,1)->text();
            QString sDstFolder          = uiBatchJob->tableWidgetBatchJobs->item(i,2)->text();
            QString sMergeSettings      = uiBatchJob->tableWidgetBatchJobs->item(i,3)->text();

            // Setup download and conversion settings in main GUI
            emit setBookUrl(sBookUrl);
            emit setDownloadOptions(sDownloadSettings);
            emit setDstFolder(sDstFolder);
            emit setMergeOptions(sMergeSettings);

            // Initial control action
#if !DISABLE_AUTOPROXY
// DISABLED IN HDH 1.1.1
            if (sDownloadSettings.contains("autoproxy"))
            {
                // Start download process with proxy connection
                uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("connecting to proxy...");
                emit setProxyState(true);
            }
            else
#endif
            {
                // Start download process by getting book information
                uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("loading book info...");
                emit getBookInfo();
            }

            break;
        }
    }

    // Check if all jobs are done
    if (i == uiBatchJob->tableWidgetBatchJobs->rowCount())
    {
        bRestartRequested = true;
        toggleStartCancelButton(false);
        uiBatchJob->tableWidgetBatchJobs->clearSelection();

        QApplication::alert(this);
    }
}


/*-----------------------------------------------------------------------------------------------*
 * Proxy connection established, start download now
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::on_proxyConnectionEstablished(bool bState)
{
    if (!mbJobCanceled)
    {
        int i;
        for (i = 0; i < uiBatchJob->tableWidgetBatchJobs->rowCount(); i++)
        {
            if (uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("connecting to proxy..."))
            {
                if (bState)
                {
                    // Setup download and conversion settings in main GUI                    
                    // Get book info
                    uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("loading book info...");
                    emit getBookInfo();

                    break;

                }
                else
                {
                    // Proxy failed
                    uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("proxy connection failed.");
                    uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setBackgroundColor(Qt::red);
                    // Run next job
                    slot_runNextJob(true);

                    break;
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Check copyright restrictions
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_bookInfoCopyright(QString sCopyright)
{
    if (sCopyright.compare("in-copyright") == 0)
    {
        miLoadBookInfoRetries = MAX_NUM_RETIRES_LOADBOOKINFO;
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Update job state when book info state has been changed
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_bookInfoState(bool bState)
{
    if ( !mbJobCanceled )
    {
        for (int i = 0; i < uiBatchJob->tableWidgetBatchJobs->rowCount(); i++)
        {
            if (uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("loading book info...")
                || uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("connecting to proxy...") )
            {
                if (bState)
                {
                    miLoadBookInfoRetries = 0;
                    uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("downloading files...");
                    // Start download
                    emit startDownload();
                }
                else
                {
                    miLoadBookInfoRetries++;

                    // Retry in case of AutoProxy
                    if ((uiBatchJob->tableWidgetBatchJobs->item(i,1)->text().contains("autoproxy")) && (miLoadBookInfoRetries < MAX_NUM_RETIRES_LOADBOOKINFO))
                    {
                        uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("open");
                    }
                    else
                    {
                        uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("failed to load book info.");
                        uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setBackgroundColor(Qt::red);

                        miLoadBookInfoRetries = 0;
                    }
                    slot_runNextJob(true);
                }

                break;
            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Update job state when download state has been changed
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_downloadState(bool bState)
{
    if ( !mbJobCanceled )
    {
        for (int i = 0; i < uiBatchJob->tableWidgetBatchJobs->rowCount(); i++)
        {
            if (uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("downloading files..."))
            {
                if (bState)
                {
                    uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("download completed. Start pdf action...");
                }
                else
                {
                    uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("download failed.");
                    uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setBackgroundColor(Qt::red);
                    slot_runNextJob(true);
                }
                break;
            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Update job state when pdf merge state has been changed
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_pdfMergeState(bool bState)
{
    if ( !mbJobCanceled )
    {
        for (int iRow = 0; iRow < uiBatchJob->tableWidgetBatchJobs->rowCount(); iRow++)
        {
            if (uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->text().contains("completed"))
            {
                QProgressBar *pPgbar = dynamic_cast<QProgressBar *>( uiBatchJob->tableWidgetBatchJobs->cellWidget(iRow,5) );
                pPgbar->setValue(pPgbar->maximum());

                if (bState)
                {
                    uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->setText("done.");
                    uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->setBackgroundColor(Qt::green);
                    uiBatchJob->tableWidgetBatchJobs->clearSelection();
                }
                else
                {
                    uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->setText("download ok,but pdf action failed.");
                    uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->setBackgroundColor(Qt::red);

                    if (!uiBatchJob->tableWidgetBatchJobs->item(iRow, 1)->text().contains("resume"))
                        uiBatchJob->tableWidgetBatchJobs->item(iRow, 1)->setText(uiBatchJob->tableWidgetBatchJobs->item(iRow, 1)->text() + " + resume" );
                }

                slot_runNextJob(true);

                break;
            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Update job state when pdf merge state has been changed
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_updateProgressbar(int iProgress)
{
    if ( !mbJobCanceled )
    {
        for (int iRow = 0; iRow < uiBatchJob->tableWidgetBatchJobs->rowCount(); iRow++)
        {
            if (uiBatchJob->tableWidgetBatchJobs->item(iRow, 4)->text().contains("downloading files..."))
            {
                QProgressBar *pPgbar = dynamic_cast<QProgressBar *>( uiBatchJob->tableWidgetBatchJobs->cellWidget(iRow,5) );
                if (iProgress >= 0)
                {
                    pPgbar->setValue(iProgress);
                    pPgbar->setMaximum(100);
                }
                else
                {
                    pPgbar->setValue(-1);
                    pPgbar->setMaximum(0);
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Continue with next job
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_runNextJob(bool bState)
{
    if ( !mbJobCanceled && bState )
    {
        slot_numberOpenJobs();
        on_pushButtonStart_clicked();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: Cancel job
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_pushButtonCancel_clicked()
{
    toggleStartCancelButton(false);

    if ( !mbJobCanceled )
    {
        mbJobCanceled = true;
        int i;
        for (i = 0; i <uiBatchJob->tableWidgetBatchJobs->rowCount(); i++)
        {
            if ( uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("loading book info") ||
                 uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("connecting to proxy") ||
                 uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("downloading files") ||
                 uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("Start pdf action") )
            break;
        }

        if ( i < uiBatchJob->tableWidgetBatchJobs->rowCount() )
        {
            if ( uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("loading book info") ||
                 uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("connecting to proxy") )
            {
                emit stopPageLoad();
            }
            if (uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("downloading files") )
            {
                QProgressBar *pPgbar = dynamic_cast<QProgressBar *>( uiBatchJob->tableWidgetBatchJobs->cellWidget(i,5) );
                if (pPgbar->maximum() == 0)
                {
                    pPgbar->setValue(0);
                    pPgbar->setMaximum(100);
                }
                emit stopDownload();
            }
            if ( uiBatchJob->tableWidgetBatchJobs->item(i, 4)->text().contains("Start pdf action") )
            {
                emit stopPdfMerge();
            }
            uiBatchJob->tableWidgetBatchJobs->item(i, 4)->setText("canceled");
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * GUI: toggle start/cancel button
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::toggleStartCancelButton(bool bState)
{
    if (bState)
    {
        // Change button to "Cancel" of Download
        uiBatchJob->pushButtonStart->setText("Cancel");
        disconnect(uiBatchJob->pushButtonStart,SIGNAL(clicked()),this, SLOT(on_pushButtonStart_clicked()));
        connect(uiBatchJob->pushButtonStart, SIGNAL(clicked()), SLOT(slot_pushButtonCancel_clicked()));

        emit batchJobActivated(true);
    }
    else
    {
        // Change button to "Start"of Download
        uiBatchJob->pushButtonStart->setText("Start download");
        disconnect(uiBatchJob->pushButtonStart,SIGNAL(clicked()),this, SLOT(slot_pushButtonCancel_clicked()));
        connect(uiBatchJob->pushButtonStart, SIGNAL(clicked()), SLOT(on_pushButtonStart_clicked()));

        emit batchJobActivated(false);
    }

    uiBatchJob->pushButtonAddBook->setEnabled(!bState);
    uiBatchJob->pushButtonRemoveBook->setEnabled(!bState);
    uiBatchJob->pushButtonRemoveAll->setEnabled(!bState);
    uiBatchJob->pushButtonEditBook->setEnabled(!bState);
    uiBatchJob->pushButtonLoad->setEnabled(!bState);
    uiBatchJob->pushButtonSave->setEnabled(!bState);
    uiBatchJob->pushButtonMoveUp->setEnabled(!bState);
    uiBatchJob->pushButtonMoveDown->setEnabled(!bState);
}

/*-----------------------------------------------------------------------------------------------*
 * Enable / Disable AutoProxy checkbox
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_disableAutoProxy(bool bIsChecked)
{
    if (bIsChecked)
        uiAddBook->checkBoxAutoProxy->setChecked(false);
}

/*-----------------------------------------------------------------------------------------------*
 * Enable / Disable WebProxy checkbox
 *-----------------------------------------------------------------------------------------------*/
void CreateBatchJob::slot_disableWebProxy(bool bIsChecked)
{
    if (bIsChecked)
        uiAddBook->checkBoxWebProxy->setChecked(false);
}
