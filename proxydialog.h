#ifndef PROXYDIALOG_H
#define PROXYDIALOG_H

#include "autoproxy.h"

#include <QDialog>
#include <QNetworkProxy>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QListWidget>
#include <QLabel>


//-------------------------------------------------------------------------
class ProxyAddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProxyAddDialog(AutoProxy *pAutoProxy,
                            QWidget *parent = 0);
    void setActualProxySourceList(QStringList* psActualProxySourceList);

signals:
    void proxySourceAdded(void);

private slots:
    void on_pushButtonAdd_clicked();
    void slot_urlCheckState(bool bSuccess);
    void slot_checkProxiesCancel(void);

private:
    AutoProxy   *mpAutoProxy;
    QLineEdit   *mpUrlLineEdit;
    QLabel      *mpLabelAddState;
    QStringList *mpsActualProxySourceList;
};


//-------------------------------------------------------------------------
class ProxySourcesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProxySourcesDialog(AutoProxy *pAutoProxy, QWidget *parent = 0);
    ~ProxySourcesDialog();
    QStringList getProxySourceList(void);
    void setProxySourceList(QStringList sProxySourceList);
    QStringList getUsProxySourceList(void);
    void setUsProxySourceList(QStringList sProxySourceList);

public slots:
    void slot_updateProxySourcesList(void);

private slots:
    void on_pushButtonAddProxy_clicked(void);
    void on_pushButtonRemoveProxy_clicked(void);
    void on_pushButtonAddUsProxy_clicked(void);
    void on_pushButtonRemoveUsProxy_clicked(void);
    void slot_checkProxies(void);
    void slot_checkProxiesCancel(void);
    void slot_proxyCheckState(bool);
    void slot_restoreDefault(void);
    void slot_updateProxySourcesOnChange(QListWidgetItem* changedItem);
    void slot_clearFocus(void);


private:
    QPushButton *mpPushButtonOk;
    QPushButton *mpPushButtonCancel;
    QPushButton *mpPushButtonCheck;
    QPushButton *mpPushButtonDefault;
    QPushButton *mpPushButtonAddProxy;
    QPushButton *mpPushButtonRemoveProxy;
    QPushButton *mpPushButtonAddUsProxy;
    QPushButton *mpPushButtonRemoveUsProxy;

    bool mbCancelCheckProxies;
    int miproxy;

    void createProxySourcesDialog(void);
    AutoProxy   *mpAutoProxy;
    ProxyAddDialog *mpProxyAddDialog;
    QWidget     *mpParent;
    QStringList msProxySourceList;
    QStringList msUsProxySourceList;

    struct GuiProxySourcesDialog {

        QListWidget *listWidgetProxyUrls;
        QListWidget *listWidgetProxyUsOnlyUrls;
    }GuiProxySourcesDialog;

    struct GuiProxySourcesDialog *uiUrlSelect;
};

//-----------------------------------------------------------------------------
class ProxyDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ProxyDialog(AutoProxy *pAutoProxy, QWidget *parent = 0);
    ~ProxyDialog();

    QString getProxyIp(void);
    int     getProxyPort(void);
    QNetworkProxy::ProxyType getProxyType(void);
    bool    getAutoProxy(void);
    bool    getAutoProxyActivation(void);
    QString getProxyUsername(void);
    QString getProxyPassword(void);
    bool    getUsProxyOnlyState(void);
    void    setProxyIp(QString);
    void    setProxyPort(int);
    void    setProxyType(int);
    void    setAutoProxy(bool);
    void    setAutomaticProxyActivation(bool);
    void    setProxyUsername(QString);
    void    setProxyPassword(QString);
    void    setUsProxyOnlyState(bool bUsOnlyProxyState);
    void    fillProxySetupDialog(void);
    int     setXmlProxySetup(QNetworkProxy *pUserProxy);
    void    saveXmlProxySetup(void);
    QString encryptString(QString sInput);
    
private slots:
    void on_buttonBox_accepted();
    void slot_enableProxyFields(bool bEnable);
    void slot_editingIpField(QString sIpString);
    void slot_openProxySourceDialog(void);
    void slot_editingIpFieldFinished(void);
    void    slot_setAutomaticProxyActivation(bool);

private:
    void createProxyDialog();
    AutoProxy   *mpAutoProxy;
    QWidget     *mpParent;

    struct tGuiProxyDialog {

        QLineEdit *lineEditProxyIp;
        QLineEdit *lineEditProxyPort;
        QComboBox *comboBoxProxyType;
        QLineEdit *lineEditUsername;
        QLineEdit *lineEditPassword;
        QRadioButton *radioButtonAutoProxy;
        QCheckBox *checkBoxAutomaticProxy;
        QRadioButton *radioButtonUserProxy;
        QComboBox *comboBoxProxyCountry;
        ProxySourcesDialog *pProxySrcDlg;

    }GuiProxyDialog;

    struct tGuiProxyDialog *uiProxy;

    QString msProxyIp;
    int     miProxyPort;
    int     miProxyType;
    bool    mbAutoProxy;
    bool    mbUsProxyOnly;
    bool    mbAutomaticProxy;
    QString msProxyUsername;
    QString msProxyPassword;

    QMap <QString, QNetworkProxy::ProxyType> mProxyTypeNamesMap;

};

#endif // PROXYDIALOG_H
