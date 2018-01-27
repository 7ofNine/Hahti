#ifndef CREATEPDFWORKER_H
#define CREATEPDFWORKER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPrinter>
#include <QTextDocument>
#include <QWidget>
#include <QEventLoop>

#if QT_VERSION >= 0x050000
     #include <QStandardPaths>
#else
     #include <QDesktopServices>
#endif

#include <QDebug>

/*-----------------------------------------------------------------------------------------------*
 * Class: pdftk worker
 *-----------------------------------------------------------------------------------------------*/
class CreatePdftkWorker : public QObject
{
     Q_OBJECT

public:
    CreatePdftkWorker( QStringList pdfFileList, QString sOutputFilename, bool *pbAbortWorker);
    ~CreatePdftkWorker();

public slots:
    void slot_mergePdfWithPdftk(void);

signals:
    void updateProgressBar(int);
    void pdfMergeState(QString);
    void finished(bool, QString);
    void pdftkErrorMessage(QString);

private:
    QStringList msParameterList;
    QString msOutputFilename;
    int miNumFiles;
    bool *mpbAbortWorker;
};

/*-----------------------------------------------------------------------------------------------*
 * Class: pdf worker
 *-----------------------------------------------------------------------------------------------*/
class CreatePdfWorker : public QObject
{
     Q_OBJECT

public:
    CreatePdfWorker( QString sTitle, QString sSrcFolder, QString sOutputFolder, QStringList imgFileList, QStringList ocrFileList,  QPrinter *apPrinter, bool *pbAbortWorker, bool bCreatePdfBook, bool bManualSelection);
    ~CreatePdfWorker();

    QString readFile (const QString& sFileName);
    void adjustTextdocumentFontSize(QTextDocument *pTextdocument, qreal  qWidth, qreal  qHeight);

public slots:
    void slot_convertAndMergeImagesToPdfFile(void);
    void slot_convertAndMergeOcrToPdfFile(void);
    void slot_abortThread(bool);

signals:
    void updateProgressBar(int);
    void finished(bool, QString);
    void pdfMergeState(QString);
    void createMessageBox(QString);

private:
    QString msTitle;
    QString msSrcFolder;
    QString msOutputDir;
    QStringList mImgFileList;
    QStringList mOcrFileList;
    QPrinter *mpPrinter;
    bool *mpbAbortWorker;
    bool mbCreatePdfBook;
    bool mbManualSelection;
    bool mbAbortThread;

    QEventLoop *mpLoop;
    QString msFontFamily;
    int mFontsize;
};

#endif // CREATEPDFWORKER_H
