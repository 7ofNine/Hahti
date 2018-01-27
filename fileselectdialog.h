#ifndef FILESELECTDIALOG_H
#define FILESELECTDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QListWidget>


class FileSelectDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit FileSelectDialog(QWidget *parent = 0, QString sSrcPath = "");
    ~FileSelectDialog();
    QStringList getFileList();
    void setFileTypes(QStringList sFileTypes);
    
private slots:
    void on_pushButtonAddFile_clicked();
    void on_pushButtonRemoveFile_clicked();
    void on_pushButtonMoveUp_clicked();
    void on_pushButtonMoveDown_clicked();

private:
    void createFileSelectDialog();

    struct GuiFileSelectDialog {

        QListWidget *listWidgetFiles;

    }GuiFileSelectDialog;

    struct GuiFileSelectDialog *uiFileSelect;

    QStringList mFileTypes;
    QString mSrcPath;
};

#endif // FILESELECTDIALOG_H
