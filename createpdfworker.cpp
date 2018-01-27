#include "createpdfworker.h"
//#include <fileselectdialog.h>
#include <hathidownloadhelper.h>

#include <QProcess>
#include <QDir>
#include <QTextImageFormat>
#include <QTextCursor>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPainter>
#include <QTimer>
#include <QFileInfo>
#include <QDebug>

/*-----------------------------------------------------------------------------------------------*
 * Constructor for pdftk worker
 *-----------------------------------------------------------------------------------------------*/
CreatePdftkWorker::CreatePdftkWorker( QStringList pdfFileList, QString sOutputFilename, bool *pbAbortWorker) :
    msParameterList(pdfFileList),
    msOutputFilename(sOutputFilename),
    mpbAbortWorker(pbAbortWorker)
{
    miNumFiles = pdfFileList.count();
}

/*-----------------------------------------------------------------------------------------------*
 * Destructor for pdftk worker
 *-----------------------------------------------------------------------------------------------*/
CreatePdftkWorker::~CreatePdftkWorker()
{
    //
}

/*-----------------------------------------------------------------------------------------------*
 * Slot: Merge pdf files with pdftk
 *-----------------------------------------------------------------------------------------------*/
void CreatePdftkWorker::slot_mergePdfWithPdftk(void)
{
    // Syntax: pdftk a.pdf b.pdf cat output out.pdf
    QProcess runPdftk;
    QChar cSeparator;

#if defined(Q_OS_WIN)
    msOutputFilename.replace("/","\\");
    cSeparator = '\\';
#else
    cSeparator = '/';
#endif

    // Temporary output files
    QStringList sTempOutputFilesList;
    QString sTempOutputFileBook = msOutputFilename.mid(0,msOutputFilename.lastIndexOf(cSeparator)+1) + QString("book_full_view.pdf.final");
    QString sTempOutputFileName = msOutputFilename.mid(0,msOutputFilename.lastIndexOf(cSeparator)+1) + QString("book_full_view.pdf.part");
    QString sSomePages;
    QString sErrorMsg;

    qDebug() << "PDFMERGER: MERGE PDF FILES WITH PDFTK";

#if defined(Q_OS_LINUX)
    // Merge Pdf files using pdftk application on LINUX
    QString sPdftkScript = "/usr/bin/pdftk ";
#elif defined(Q_OS_WIN)
    // Merge Pdf files using pdftk application on WINDOWS
    QString sPdftkScript = "pdftk\\pdftk ";

    QString sTmp;
    for (int i=0; i < msParameterList.count(); i++)
    {
        sTmp = msParameterList.at(i);
        msParameterList.replace(i,sTmp.replace("/", "\\"));
    }
#elif defined(Q_OS_MAC)
    // Merge Pdf files using pdftk application on MAC OS
    QString sPdftkScript = "/usr/local/bin/pdftk ";
#elif  defined(Q_OS_FREEBSD)
    //TODO: NOT TESTED
    QString sPdftkScript = "/usr/local/bin/pdftk ";
#elif defined (Q_OS_OS2)
    //TODO: NOT TESTED
    QString sPdftkScript = "/usr/bin/pdftk ";
#else
    //TODO: NOT TESTED
    QString sPdftkScript = "/usr/bin/pdftk ";
#endif

    int iCurPageIdx = 0;
    int iNumPages2Merge;
    int iCycle = 0;

    // For all pages in list
    while ( (iCurPageIdx < msParameterList.count())  && !*mpbAbortWorker )
    {
        // Copy pages in temp string and check max page num and max string length
        sSomePages.clear();
        iNumPages2Merge = 0;
        while(  ( sSomePages.count() < 8000)  && (iCurPageIdx < msParameterList.count()) && (iNumPages2Merge < 50))
        {
            sSomePages.append("\"" + msParameterList.at(iCurPageIdx++) + + "\" " );
            iNumPages2Merge++;
        }

        qDebug() << "PDFMERGER: CREATING TEMP FILE";

        // Merge Pages
        emit pdfMergeState( QString("Merging pdfs using pdftk....page %1").arg(QString::number( iCurPageIdx )));
        emit updateProgressBar( iCurPageIdx );

        sTempOutputFilesList.append(sTempOutputFileName + QString("_%1").arg(iCycle++,4,10, QChar('0')));
        runPdftk.start(sPdftkScript + sSomePages + QString(" cat output ") + "\"" + sTempOutputFilesList.last() + "\"");
        if (!runPdftk.waitForStarted())
        {
            qDebug() << "PDFMERGER: CREATING TEMP FILE FAILED";
#if defined(Q_OS_WIN)
            emit pdfMergeState( QString("Error: unable to execute 'pdftk' application."));
#else
            emit pdfMergeState(QString("Error: unable to execute 'pdftk' application. Is 'pdftk' installed on your OS?"));
#endif
            emit updateProgressBar(0);
            emit finished(true, "");

            return;
        }

        if (!runPdftk.waitForFinished())
        {
            qDebug() << "PDFMERGER: Error: Waiting for 'pdftk' application.";
            emit pdfMergeState(QString("Error: Waiting for 'pdftk' application."));
            emit updateProgressBar(0);
            emit finished(true, "");

            return;
        }

        // Check for pdftk error
        runPdftk.waitForReadyRead();
        sErrorMsg = runPdftk.readAllStandardError();
        if (!sErrorMsg.isEmpty())
        {
            qDebug() << "PDFMERGER: PDFTK-ERROR: " << sErrorMsg;

            emit pdftkErrorMessage(sErrorMsg);
            emit pdfMergeState(QString("Error: Corrupted file detected by 'pdftk' application."));
            emit updateProgressBar(0);

            // Remove all temporary files
            foreach (QString sFile, sTempOutputFilesList)
                QFile::remove(sFile);

            return;
        }
    }

    if (!*mpbAbortWorker)
    {
        // Merge temp files
        qDebug() << "PDFMERGER: MERGING TEMP FILES";
        emit pdfMergeState( QString("Merging pdfs using pdftk....finalizing"));
        runPdftk.start(sPdftkScript + "\""+ sTempOutputFilesList.join("\" \"") + "\"" + QString(" cat output ") + "\"" + sTempOutputFileBook + "\"");//msOutputFilename
        if (!runPdftk.waitForStarted())
        {
            qDebug() << "PDFMERGER: MERGING TEMP FILES FAILURE - WAIT FOR STARTED";
#if defined(Q_OS_WIN)
            emit pdfMergeState( QString("Error: unable to execute 'pdftk' application."));
#else
            emit pdfMergeState(QString("Error: unable to execute 'pdftk' application. Is 'pdftk' installed on your OS?"));
#endif
            emit updateProgressBar(0);
            emit finished(true, "");

            return;
        }

        if (!runPdftk.waitForFinished())
        {
            qDebug() << "PDFMERGER: Error: Waiting for 'pdftk' application.";
            emit pdfMergeState(QString("Error: Waiting for 'pdftk' application."));
            emit updateProgressBar(0);
            emit finished(true, "");

            return;
        }

        // Check for pdftk error
        runPdftk.waitForReadyRead();
        sErrorMsg = runPdftk.readAllStandardError();
        if (!sErrorMsg.isEmpty())
        {
            qDebug() << "PDFMERGER: PDFTK-ERROR: " << sErrorMsg;

            emit pdftkErrorMessage(sErrorMsg);
            emit pdfMergeState(QString("Error: Waiting for 'pdftk' application."));
            emit updateProgressBar(0);
            emit finished(true, "");

            // Remove all temporary files
            foreach (QString sFile, sTempOutputFilesList)
                QFile::remove(sFile);

            return;
        }
    }
    else if (*mpbAbortWorker)
    {
        emit pdfMergeState( QString("Merging pdfs using pdftk canceled..."));
    }

    runPdftk.waitForFinished();

    // Rename output file
    QFile::rename(sTempOutputFileBook, msOutputFilename);

    // Remove all temporary files
    foreach (QString sFile, sTempOutputFilesList)
        QFile::remove(sFile);

    emit updateProgressBar( miNumFiles );

    if (*mpbAbortWorker)
    {
        qDebug() << "PDFMERGER: PROCESS ABORTED!";
        emit finished(true, "");
    }
    else
    {
        qDebug() << "PDFMERGER: PROCESS FINISHED";
        emit finished(true, msOutputFilename);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Constructor for pdf worker
 *-----------------------------------------------------------------------------------------------*/
CreatePdfWorker::CreatePdfWorker(QString sTitle, QString sSrcFolder, QString sOutputFolder, QStringList imgFileList, QStringList ocrFileList,  QPrinter *apPrinter, bool *pbAbortWorker, bool bCreatePdfBook, bool bManualSelection) :
    msTitle(sTitle),
    msSrcFolder(sSrcFolder),
    msOutputDir(sOutputFolder),
    mImgFileList(imgFileList),
    mOcrFileList(ocrFileList),
    mpPrinter(apPrinter),
    mpbAbortWorker(pbAbortWorker),
    mbCreatePdfBook(bCreatePdfBook),
    mbManualSelection(bManualSelection)
{
    mbAbortThread       = true;
    mpLoop = new QEventLoop;
    // Wait for at most 1 day for user action
    QTimer::singleShot(24 * 60 * 60 * 1000,mpLoop, SLOT(quit()));

    msFontFamily =  QString("Times New Roman");
    mFontsize = 8;
}

/*-----------------------------------------------------------------------------------------------*
 * Destructor
 *-----------------------------------------------------------------------------------------------*/
CreatePdfWorker::~CreatePdfWorker()
{
    //
}

/*-----------------------------------------------------------------------------------------------*
 * Slot: Ask user for threat abortion
 *-----------------------------------------------------------------------------------------------*/
void CreatePdfWorker::slot_abortThread(bool bAbortFlag)
{
    mbAbortThread       = bAbortFlag;
    // Stop waiting loop
    mpLoop->quit();
}

/*-----------------------------------------------------------------------------------------------*
 * Slot: convert and merge images to a pdf book or single pdf files
 *-----------------------------------------------------------------------------------------------*/
void CreatePdfWorker::slot_convertAndMergeImagesToPdfFile()
{
     // Set outputname for pdf book
     QString sFilename = msTitle;
     if (sFilename.length() < 2)
         sFilename = "book";
     // limit title length
     if (sFilename.length() > 50)
         sFilename.truncate(sFilename.indexOf(QChar(' '),50));
     sFilename.append("_full_view.pdf.part");

     // Page elements
     QImage image;
     QUrl imageUri;
     QTextDocument imageDocument;
     QTextImageFormat imageFormat;
     QTextCursor cursor(&imageDocument);
     QRectF pageSize(mpPrinter->pageRect());
     QString ocrText;
     QGraphicsScene scene;
     QTextDocument textDocument;
     QFont documentFont(msFontFamily, mFontsize);
     QGraphicsTextItem itemText;
     QGraphicsTextItem itemImage;
     QPainter painter;
     int iExtPos;

     if ( mbCreatePdfBook )
     {
         // Set printer output file
         mpPrinter->setOutputFileName(msOutputDir + "/"  + sFilename);

         // Create and merge pdfs using QPrinter
         if (! painter.begin(mpPrinter)) { // failed to open file
             qWarning("failed to open file, is it writable?");
             emit pdfMergeState(QString("Failed to open pdf file, is it writable?"));
             emit updateProgressBar(0);
             emit finished(true,"");

             return;
         }
     }

     // Add document elements; content of elements will be updated dynamically
     scene.addItem(&itemText);
     scene.addItem(&itemImage);

     // For eachfile in the list
     for ( int i = 0;  (i < mImgFileList.count()) && !*mpbAbortWorker; i++ )
     {
         // Update progressbar
         emit updateProgressBar(i);
#if defined(Q_OS_MAC)
         if ( !mOcrFileList.isEmpty() )
             emit pdfMergeState(QString("creating searchable pdf page %1 of %2").arg(i+1).arg(mImgFileList.count()));
         else
             emit pdfMergeState(QString("creating pdf page %1 of %2").arg(i+1).arg(mImgFileList.count()));
#else
         if ( !mOcrFileList.isEmpty() )
             emit pdfMergeState(QString("creating searchable pdf page %1 of %2 with %3 dpi").arg(i+1).arg(mImgFileList.count()).arg(mpPrinter->resolution()));
         else
             emit pdfMergeState(QString("creating pdf page %1 of %2 with %3 dpi").arg(i+1).arg(mImgFileList.count()).arg(mpPrinter->resolution()));
#endif

         // Load image files
         sFilename = mImgFileList.at(i);

         if (!image.load(sFilename))
         {
            QString sMessage = QString("Unable to load image file: %1").arg(mImgFileList.at(i).right(mImgFileList.at(i).length()-mImgFileList.at(i).lastIndexOf("/")-1));
            emit pdfMergeState(sMessage);

            sMessage.append("\nDo you like to go on and insert a blank page instead?");

            emit createMessageBox(sMessage);

            // Wait for user response
            mpLoop->exec();

            if ( !mbAbortThread )
            {
                // Create empty image
                image = QImage( qRound(pageSize.width()), qRound(pageSize.height()), QImage::Format_RGB16);
                image.fill(Qt::transparent);
            }
            else
            {
                emit pdfMergeState(QString("Aborted by user..."));
                emit updateProgressBar(0);
                emit finished(true,"");

                return;
            }
        }

// TODO:  Conversion for monochrome is working, but QPrinter does only support JPEG format. Are there any Qt Pdf alternatives?
//        // Convert png images to monochrome
//        if (sFilename.contains(".png") )
//        {
//            image=  image.convertToFormat(QImage::Format_Mono, Qt::MonoOnly|Qt::ThresholdDither|Qt::AvoidDither);
//        }

        // Adjust image size
        image = image.scaled( qRound(pageSize.width()), qRound(pageSize.height()),   Qt::KeepAspectRatio );

        // Update image document
        imageUri = QString ("file://%1").arg(mImgFileList.at(i));
        imageDocument.clear();
        imageDocument.addResource(QTextDocument::ImageResource, imageUri, QVariant(image));
        imageFormat.setWidth(image.width());
        imageFormat.setHeight(image.height());
        imageFormat.setName(imageUri.toString());
        cursor.insertImage(imageFormat);

        if ( !mOcrFileList.isEmpty() && !mbManualSelection )
        {
            // Setup page
            textDocument.clear();
            textDocument.setPageSize(image.size());
            textDocument.setTextWidth(-1);
            textDocument.setDefaultFont(documentFont);

            if ( i < mOcrFileList.count())
            {
                // Load ocr file
                ocrText = readFile(mOcrFileList.at(i));
            }
            else
            {
                // Empty document
                ocrText = QString("<div>_EMPTY_</div>");
            }

            // Add text
            textDocument.setHtml(ocrText);

            // Adjust font size of text document
            adjustTextdocumentFontSize(&textDocument, image.width(),  image.height() );

            // Add ocr to scene
            itemText.setDocument(&textDocument);
        }

        // Add image to scene
        itemImage.setDocument(&imageDocument);

        // Create Single pages filename
        if ( !mbCreatePdfBook )
        {
            // Create Single page filenames
            sFilename = mImgFileList.at(i);
            iExtPos = sFilename.lastIndexOf("\\");
            if (iExtPos < 0)
                iExtPos = sFilename.lastIndexOf("/");
            sFilename.remove(0,iExtPos+1);
            iExtPos = sFilename.lastIndexOf(".");
            if (iExtPos < 1)
                iExtPos = sFilename.length();
            sFilename.truncate(iExtPos);
            sFilename.append(".pdf");
            // Set output filename
            mpPrinter->setOutputFileName(msOutputDir + "/"  + sFilename);
            if (! painter.begin(mpPrinter)) { // failed to open file
                qWarning("failed to open file, is it writable?");
                emit pdfMergeState(QString("Failed to open pdf file, is it writable?"));
                emit updateProgressBar(0);
                emit finished(true,"");

                return;
            }
        }

         // Create page
         scene.render(&painter);
         // Close page
         if ( !mbCreatePdfBook )
         {
             // Close painter
             painter.end();
         }
         else if ( (i+1) < mImgFileList.count() )
         {
             mpPrinter->newPage();
         }
    }

    if (!*mpbAbortWorker)
    emit updateProgressBar(mImgFileList.count());

    if ( mbCreatePdfBook )
    {
     // Close painter
     painter.end();
    }

    // Delete items
    imageUri.clear();
    itemImage.deleteLater();
    itemText.deleteLater();
    imageDocument.deleteLater();
    ocrText.clear();
    textDocument.deleteLater();
    scene.deleteLater();

    emit finished(mbCreatePdfBook, mpPrinter->outputFileName());
}

/*-----------------------------------------------------------------------------------------------*
 * Slot: Convert OCR plaintext only to pdf book or single pages
 *-----------------------------------------------------------------------------------------------*/
void CreatePdfWorker::slot_convertAndMergeOcrToPdfFile()
{
    // Set outputname for pdf book
    QString sFilename = msTitle;
    if (sFilename.length() < 2)
        sFilename = "book";
    // limit title length
    if (sFilename.length() > 50)
        sFilename.truncate(sFilename.indexOf(QChar(' '),50));
    sFilename.append("_full_view_text_only.pdf.part");

    // Page elements
    QString ocrText;
    QGraphicsScene scene;
    QTextDocument textDocument;
    QFont documentFont(msFontFamily, mFontsize);
    QGraphicsTextItem itemText;
    QPainter painter;
    int iExtPos;

    if ( mbCreatePdfBook )
    {
         // Set pdf output file
         mpPrinter->setOutputFileName(msOutputDir + "/"  + sFilename);

        // Open file
        if (! painter.begin(mpPrinter)) { // failed to open file
            qWarning("failed to open file, is it writable?");
            emit pdfMergeState(QString("Failed to open pdf file, is it writable?"));
            emit updateProgressBar(0);
            emit finished(true,"");

            return;
        }
    }

    // Add document elements; content of elements will be updated dynamically
    scene.addItem(&itemText);

    // For each file in the list
    for ( int i = 0;  (i < mOcrFileList.count()) && !*mpbAbortWorker; i++ )
    {
        // Update progressbar
        emit updateProgressBar(i);

#if defined(Q_OS_MAC)
        emit pdfMergeState(QString("creating plaintext pdf page %1 of %2").arg(i+1).arg(mOcrFileList.count()));
#else

        emit pdfMergeState(QString("creating plaintext pdf page %1 of %2 with %3 dpi").arg(i+1).arg(mOcrFileList.count()).arg(mpPrinter->resolution()));
#endif

        // Update font size
        textDocument.clear();
        textDocument.setPageSize(QSize( mpPrinter->width(),  mpPrinter->height()));
        textDocument.setTextWidth(-1);
        textDocument.setDefaultFont(QFont(msFontFamily, mFontsize));

        // Load and set ocr text from file
        ocrText = readFile(mOcrFileList.at(i));
        textDocument.setHtml(ocrText);

        // Adjust font size of text document
        adjustTextdocumentFontSize(&textDocument,mpPrinter->width(), mpPrinter->height() );

        // Add ocr to scene
        itemText.setDocument(&textDocument);


        // Create Single page filenames
        if ( !mbCreatePdfBook )
        {
            // Create filename
            sFilename = mOcrFileList.at(i);
            iExtPos = sFilename.lastIndexOf("\\");
            if (iExtPos < 0)
                iExtPos = sFilename.lastIndexOf("/");
            sFilename.remove(0,iExtPos+1);
            iExtPos = sFilename.lastIndexOf(".");
            if (iExtPos < 1)
                iExtPos = sFilename.length();
            sFilename.truncate(iExtPos);
            sFilename.append("_text_only.pdf");

            // Set output filename
            mpPrinter->setOutputFileName(msOutputDir + "/"  + sFilename);

            if (! painter.begin(mpPrinter)) { // failed to open file
                qWarning("failed to open file, is it writable?");
                emit pdfMergeState(QString("Failed to open pdf file, is it writable?"));
                emit updateProgressBar(0);
                emit finished(true,"");

                return;
            }
        }

        // Create page
        scene.render(&painter);
        // Close page
        if ( !mbCreatePdfBook )
        {
            // Close painter
            painter.end();
        }
        else if ( (i+1) < mOcrFileList.count())
        {
            mpPrinter->newPage();
        }
    }

    if (!*mpbAbortWorker)
    emit updateProgressBar(mImgFileList.count());

    if ( mbCreatePdfBook )
    {
        // Close painter
        painter.end();
    }

    itemText.deleteLater();
    textDocument.deleteLater();
    scene.deleteLater();

    emit finished(mbCreatePdfBook, mpPrinter->outputFileName());
}

/*-----------------------------------------------------------------------------------------------*
* Read file: ocr, xml
*-----------------------------------------------------------------------------------------------*/
QString CreatePdfWorker::readFile (const QString& sFileName)
{
    QFileInfo fi(sFileName);
#if defined(Q_OS_WIN)
    QString sPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#else
#if QT_VERSION >= 0x050000
     QString sPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data/organization/application";
#else
     QString sPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
     //sPath = QDir(sPath).absolutePath() + "/";
#endif // Q_OS_WIN

     if (fi.isAbsolute())
     {
         sPath = fi.path();
     }

     QDir filePath;
     if (filePath.mkpath(sPath))
     {
        QFile file(sPath + QDir::separator() + fi.fileName());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream stream(&file);
            file.deleteLater();
            return stream.readAll();
        }
        file.deleteLater();
    }
    return "";
}

/*-----------------------------------------------------------------------------------------------*
 * Adjust textdocument font size to match page size
 *-----------------------------------------------------------------------------------------------*/
void CreatePdfWorker::adjustTextdocumentFontSize(QTextDocument *pTextdocument, qreal  qWidth, qreal  qHeight)
{
    int iPointSize = pTextdocument->defaultFont().pointSize();
    qreal qDiffWidthRatio = qWidth/pTextdocument->idealWidth();
    iPointSize = (int)( iPointSize*qDiffWidthRatio);
    if (iPointSize > (int) 100)
        iPointSize =  100;
    if (iPointSize < (int) 2)
        iPointSize =  1;
    pTextdocument->setDefaultFont(QFont(msFontFamily, iPointSize));

    // Adjust font size to fit image height
    while ( (pTextdocument->size().height() > qHeight) && (iPointSize > (int) 2)  )
    {
        iPointSize--;
        pTextdocument->setDefaultFont(QFont(msFontFamily, iPointSize));
    }

    // If text still does not match page size use plain, unformatted text
    if ( qHeight < pTextdocument->size().height() )
    {
        QString sPlaintext = pTextdocument->toPlainText();
        sPlaintext.replace("\n", " ");
        pTextdocument->setTextWidth(qWidth);
        pTextdocument->setPlainText(sPlaintext);

        // If text still does not match page skip ocr text
        if ( qHeight < pTextdocument->size().height() )
        {
            pTextdocument->clear();
            pTextdocument->setHtml(QString("<div>_SKIPPED_</div>"));
        }
    }
    else
    {
        // Update default font size
        mFontsize =  pTextdocument->defaultFont().pointSize();
    }
}
