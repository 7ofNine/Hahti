#include "fileselectdialog.h"

#include <QFileDialog>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QHBoxLayout>

#define connect(src, sig, trgt, slt) connect(src, sig, trgt, slt, Qt::UniqueConnection)

/*-----------------------------------------------------------------------------------------------*
 * Dialog Constructor
 *-----------------------------------------------------------------------------------------------*/
FileSelectDialog::FileSelectDialog(QWidget *parent, QString sSrcPath) :
    QDialog(parent),
    mSrcPath(sSrcPath)
{
    createFileSelectDialog();
}

/*-----------------------------------------------------------------------------------------------*
 * Dialog Destructor
 *-----------------------------------------------------------------------------------------------*/
FileSelectDialog::~FileSelectDialog()
{
    //
}

/*-----------------------------------------------------------------------------------------------*
 * Create file select dialog
 *-----------------------------------------------------------------------------------------------*/
void FileSelectDialog::createFileSelectDialog()
{
    QVBoxLayout *pVBoxLayout            = new QVBoxLayout;

    QPushButton *pPushButtonOk          = new QPushButton(QString("Merge"));
    connect(pPushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    pVBoxLayout->addWidget(pPushButtonOk);

    QPushButton *pPushButtonCancel      = new QPushButton(QString("Cancel"));
    connect(pPushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    pVBoxLayout->addWidget(pPushButtonCancel);

    QSpacerItem* spacer1                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pVBoxLayout->addItem(spacer1);

    QPushButton *pPushButtonAddFiles    = new QPushButton(QString("Add files"));
    connect(pPushButtonAddFiles, SIGNAL(clicked()), this, SLOT(on_pushButtonAddFile_clicked()));
    pVBoxLayout->addWidget(pPushButtonAddFiles);

    QPushButton *pPushButtonRemoveFiles = new QPushButton(QString("Remove files"));
    connect(pPushButtonRemoveFiles, SIGNAL(clicked()), this, SLOT(on_pushButtonRemoveFile_clicked()));
    pVBoxLayout->addWidget(pPushButtonRemoveFiles);

    QSpacerItem* spacer2                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pVBoxLayout->addItem(spacer2);

    QPushButton *pPushButtonMoveUp      = new QPushButton(QString("Move up"));
    connect(pPushButtonMoveUp, SIGNAL(clicked()), this, SLOT(on_pushButtonMoveUp_clicked()));
    pVBoxLayout->addWidget(pPushButtonMoveUp);

    QPushButton *pPushButtonMoveDown    = new QPushButton(QString("Move down"));
    connect(pPushButtonMoveDown, SIGNAL(clicked()), this, SLOT(on_pushButtonMoveDown_clicked()));
    pVBoxLayout->addWidget(pPushButtonMoveDown);

    uiFileSelect                        = &GuiFileSelectDialog;
    uiFileSelect->listWidgetFiles       = new QListWidget();

    QHBoxLayout *pHBoxLayout            = new QHBoxLayout;

    pHBoxLayout->addWidget(uiFileSelect->listWidgetFiles);
    pHBoxLayout->addLayout(pVBoxLayout);

    this->setLayout(pHBoxLayout);
    this->setWindowTitle(QString("File Selection Dialog"));

}

/*-----------------------------------------------------------------------------------------------*
 * Add file dialog
 *-----------------------------------------------------------------------------------------------*/
void FileSelectDialog::on_pushButtonAddFile_clicked()
{
    QFileDialog dialog(this);
    QString sFilter;
    for (int i=0; i < mFileTypes.count(); i++)
        sFilter.append(QString("%1 ").arg(mFileTypes.at(i)));
#if defined(Q_OS_MAC)
    /* Workarround for issue: QTBUG-27664 */
    QStringList fileNames = QFileDialog::getOpenFileNames( this,
                                                "Select one or more files to open",
                                                mSrcPath,
                                                "(" + sFilter + ")", 0, QFileDialog::DontUseNativeDialog );
#else
    QStringList fileNames = dialog.getOpenFileNames(this,
                                                    "Select one or more files to open",
                                                    mSrcPath,
                                                    "(" + sFilter + ")" );
#endif

    if (!fileNames.isEmpty())
    {
        uiFileSelect->listWidgetFiles->addItems(fileNames);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Remove file action
 *-----------------------------------------------------------------------------------------------*/
void FileSelectDialog::on_pushButtonRemoveFile_clicked()
{
    QListWidgetItem *currentItem = uiFileSelect->listWidgetFiles->currentItem();
    delete currentItem;
}

/*-----------------------------------------------------------------------------------------------*
 * Move item up
 *-----------------------------------------------------------------------------------------------*/
void FileSelectDialog::on_pushButtonMoveUp_clicked()
{
    int iCurrentRow = uiFileSelect->listWidgetFiles->currentRow();

    if (iCurrentRow > 0 )
    {
        QListWidgetItem *currentItem = uiFileSelect->listWidgetFiles->takeItem(iCurrentRow);
        uiFileSelect->listWidgetFiles->insertItem(iCurrentRow-1, currentItem);
        uiFileSelect->listWidgetFiles->setCurrentRow(iCurrentRow-1);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Move item down
 *-----------------------------------------------------------------------------------------------*/
void FileSelectDialog::on_pushButtonMoveDown_clicked()
{
    int iCurrentRow =  uiFileSelect->listWidgetFiles->currentRow();

    if (iCurrentRow < ( uiFileSelect->listWidgetFiles->count() -1))
    {
        QListWidgetItem *currentItem = uiFileSelect->listWidgetFiles->takeItem(iCurrentRow);
        uiFileSelect->listWidgetFiles->insertItem(iCurrentRow+1, currentItem);
        uiFileSelect->listWidgetFiles->setCurrentRow(iCurrentRow+1);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * set file typles
 *-----------------------------------------------------------------------------------------------*/
void FileSelectDialog::setFileTypes(QStringList sFileTypes)
{
    mFileTypes = sFileTypes;
}

/*-----------------------------------------------------------------------------------------------*
 * Return file list
 *-----------------------------------------------------------------------------------------------*/
QStringList FileSelectDialog::getFileList()
{
    QStringList fileList;

    for (int i=0; i < uiFileSelect->listWidgetFiles->count(); i++ )
        fileList.append(  uiFileSelect->listWidgetFiles->item(i)->text());

    return fileList;
}
