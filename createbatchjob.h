#ifndef CREATEBATCHJOB_H
#define CREATEBATCHJOB_H

#include "common.h"

#include <QTableWidget>
#include <QWidget>
#include <QDialog>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QToolButton>
#include <QHeaderView>
#include <QPushButton>

class CreateBatchJob : public QWidget
{
    Q_OBJECT
    
public:
    explicit CreateBatchJob(QWidget *parent = 0, tUserSettings *ptUserSettings = 0);
    ~CreateBatchJob();
    QString extractBookIdFromUrl(QString sUrl);
    void toggleStartCancelButton(bool bState);
    void addBookJob(QString sUrl);
    void startBatchJob(void);
    void numberOpenJobs(void);
    void setGuiFont(QFont guiFont);

signals:
    void batchJobActivated(bool);
    void setProxyState(bool);
    void setBookUrl(QString);
    void setDownloadOptions(QString);
    void setDstFolder(QString);
    void setMergeOptions(QString);
    void getBookInfo(void);
    void stopPageLoad(void);
    void startDownload(void);
    void stopDownload(void);
    void stopPdfMerge(void);
    void minimize(void);
    void maximize(void);
    void numberOpenJobs(int);

private slots:

    void slot_adjustGuiGrid();
    void slot_addBookItem();
    void slot_toolButtonDstFolder_clicked();
    void slot_runNextJob(bool);
    void slot_pushButtonCancel_clicked();
    void slot_bookInfoState(bool);
    void slot_bookInfoCopyright(QString sCopyright);
    void on_proxyConnectionEstablished(bool bState);
    void slot_pdfMergeState(bool);
    void slot_downloadState(bool);
    void slot_updateProgressbar(int iProgress);
    void slot_clipboardChanged(QString sUrl);
    void slot_loadTmpTableWidgetBatchJobs(void);
    void slot_saveTmpTableWidgetBatchJobs(void);
    void on_pushButtonAddBook_clicked();
    void on_pushButtonEditBook_clicked();
    void on_pushButtonRemoveBook_clicked();
    void on_pushButtonRemoveAll_clicked();
    void on_pushButtonMoveDown_clicked();
    void on_pushButtonMoveUp_clicked();
    void on_pushButtonSave_clicked();
    void on_pushButtonLoad_clicked();
    void on_pushButtonStart_clicked();
    void slot_disableAutoProxy(bool bIsChecked);
    void slot_disableWebProxy(bool bIsChecked);
    void slot_numberOpenJobs(void);

private:
    void createAddBookDialog();
    void createBatchJobGui();
    void changeEvent ( QEvent  * event );
    void updateTmpTableWidgetBatchJobsFile(void);

    QDialog *mpUiAddBookDialog;

    struct GuiBatchJobDialog {

        QTableWidget    *tableWidgetBatchJobs;

        QPushButton     *pushButtonStart;
        QPushButton     *pushButtonAddBook;
        QPushButton     *pushButtonEditBook;
        QPushButton     *pushButtonRemoveBook;
        QPushButton     *pushButtonRemoveAll;
        QPushButton     *pushButtonMoveUp;
        QPushButton     *pushButtonMoveDown;
        QPushButton     *pushButtonSave;
        QPushButton     *pushButtonLoad;

    } GuiBatchJobDialog;

    struct GuiBatchJobDialog *uiBatchJob;

    struct GuiAddBookDialog {
        // Book info groupbox elements
        QGroupBox       *groupBoxBookInfo;
        QGridLayout     *gridLayoutBookInfo;
        QLabel          *labelBookURL;
        QLineEdit       *lineEditBookURL;
        QPushButton     *pushButtonGetInfo;

        // Download settings groupbox elements
        QGroupBox       *groupBoxDownloadSettings;
        QGridLayout     *gridLayoutDownloadSettings;
        QLabel          *labelFormat;
        QRadioButton    *radioButtonPdf;
        QRadioButton    *radioButtonImages;
        QComboBox       *comboBoxZoom;
        QCheckBox       *checkBoxGetOcr;
        QLabel          *labelDstFolder;
        QCheckBox       *checkBoxResume;
        QCheckBox       *checkBoxWebProxy;
        QCheckBox       *checkBoxAutoProxy;
        QComboBox       *comboBoxProxyUsOnly;
        QLineEdit       *lineEditDstFolder;
        QToolButton     *toolButtonDstFolder;

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
        QRadioButton *radioButtonDoNothing;

    } GuiAddBookDialog;

    struct GuiAddBookDialog *uiAddBook;

    QHeaderView         *mpHeaderView;
    bool                mbJobCanceled;
    bool                mbAddNewItem;
    tUserSettings*      mptUserSettings;
    QString             msLastJobFilename;
    int                 miLoadBookInfoRetries;
};

#endif // CREATEBATCHJOB_H
