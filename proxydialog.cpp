#include "proxydialog.h"
#include "common.h"

#include <QMessageBox>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QSpacerItem>
#include <QFileDialog>
#include <QDebug>

// Avoid redefinition of same connection between same elements
#define connect(src, sig, trgt, slt) connect(src, sig, trgt, slt, Qt::UniqueConnection)

///////////////////////////////////////////////////////////////////////////////////////////////////
// ProxyDialog class
///////////////////////////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------------------------*
 * Dialog constructor
 *-----------------------------------------------------------------------------------------------*/
ProxyDialog::ProxyDialog(AutoProxy *pAutoProxy, QWidget *parent) :
    QDialog(parent),
    mpAutoProxy(pAutoProxy),
    mpParent(parent)
{
    createProxyDialog();
}

/*-----------------------------------------------------------------------------------------------*
 * Dialog destructor
 *-----------------------------------------------------------------------------------------------*/
ProxyDialog::~ProxyDialog()
{
    //
}

/*-----------------------------------------------------------------------------------------------*
 * Create proxy dialog elements
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::createProxyDialog()
{
    uiProxy = &GuiProxyDialog;

    // Create radiobutton for auto proxy
    QGridLayout* pGridLayoutAutoProxy = new QGridLayout();
    uiProxy->radioButtonAutoProxy   = new QRadioButton(QString("automatic proxy"));
    uiProxy->radioButtonAutoProxy->setChecked(true);
    pGridLayoutAutoProxy->addWidget(uiProxy->radioButtonAutoProxy, 1, 1);
    // Add lable
    QLabel *pLableCountry = new QLabel();
    pLableCountry->setText("select proxy origin: ");
    pGridLayoutAutoProxy->addWidget(pLableCountry, 1, 2, 1, 1, Qt::AlignRight);
    // Add ComboBox
    uiProxy->comboBoxProxyCountry = new QComboBox();
    uiProxy->comboBoxProxyCountry->addItem("US only");
    uiProxy->comboBoxProxyCountry->addItem("all countries");
    uiProxy->comboBoxProxyCountry->setCurrentIndex(0);
    pGridLayoutAutoProxy->addWidget(uiProxy->comboBoxProxyCountry, 1, 3, 1, 1, Qt::AlignLeft);
    // Add Preference button
    QPushButton* pPushButtonPreferences = new QPushButton("Preferences");
    pGridLayoutAutoProxy->addWidget(pPushButtonPreferences, 1, 6 , 1, 1);
    // Add Spacer
    QSpacerItem* spacer1 = new QSpacerItem( 1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pGridLayoutAutoProxy->addItem(spacer1,1,4, 1, 1);
    // Create checkbox for automatic proxy support
    uiProxy->checkBoxAutomaticProxy = new QCheckBox();
    uiProxy->checkBoxAutomaticProxy->setText(tr("enable automatic Proxy Server activation"));
    uiProxy->checkBoxAutomaticProxy->setToolTip("enables HDH to automatically activate proxy server connections to obtain book informations if required");
    pGridLayoutAutoProxy->addWidget(uiProxy->checkBoxAutomaticProxy, 2, 1, 1, 4);
    // Create relationshipgs
    connect(uiProxy->radioButtonAutoProxy, SIGNAL(toggled(bool)), uiProxy->comboBoxProxyCountry, SLOT(setEnabled(bool)));
    connect(uiProxy->radioButtonAutoProxy, SIGNAL(toggled(bool)), pPushButtonPreferences, SLOT(setEnabled(bool)));
    connect(uiProxy->radioButtonAutoProxy, SIGNAL(toggled(bool)), pLableCountry, SLOT(setEnabled(bool)));
    connect(uiProxy->radioButtonAutoProxy, SIGNAL(toggled(bool)), uiProxy->checkBoxAutomaticProxy, SLOT(setEnabled(bool)));
    connect(uiProxy->checkBoxAutomaticProxy, SIGNAL(clicked(bool)), this, SLOT(slot_setAutomaticProxyActivation(bool)));

    // Line
    QFrame *pHorLine0 = new QFrame();
    pHorLine0->setObjectName(QString::fromUtf8("line"));
    pHorLine0->setFrameShape(QFrame::HLine);
    pHorLine0->setFrameShadow(QFrame::Sunken);
    pGridLayoutAutoProxy->addWidget(pHorLine0,3,1,1,6);

    // Create radiobutton for user proxy
    uiProxy->radioButtonUserProxy   = new QRadioButton(QString("manual proxy"));

    // Create groupbox for input mask
    QGridLayout* pGridLayoutUserProxy = new QGridLayout();

    // Row 0 -------------------------------------------------------------------------------
    QLabel *pIpLabel = new QLabel(QString("IP:"));
    pGridLayoutUserProxy->addWidget(pIpLabel,0,1);
    QLabel *pPortLabel = new QLabel(QString("Port:"));
    pGridLayoutUserProxy->addWidget(pPortLabel,0,2);
    QLabel *pProxyTypeLabel = new QLabel(QString("Proxy type:"));
    pGridLayoutUserProxy->addWidget(pProxyTypeLabel,0,3);

    // Row 1 -------------------------------------------------------------------------------
    QLabel *pProxyLabel = new QLabel(QString("Proxy:"));
    pGridLayoutUserProxy->addWidget(pProxyLabel,1,0);
    uiProxy->lineEditProxyIp = new QLineEdit();
    pGridLayoutUserProxy->addWidget(uiProxy->lineEditProxyIp,1,1);
    uiProxy->lineEditProxyPort = new QLineEdit();
    pGridLayoutUserProxy->addWidget(uiProxy->lineEditProxyPort,1,2);
    uiProxy->comboBoxProxyType = new QComboBox();
    pGridLayoutUserProxy->addWidget(uiProxy->comboBoxProxyType,1,3);

    // Row 2 -------------------------------------------------------------------------------
    QFrame *pHorLine = new QFrame();
    pHorLine->setObjectName(QString::fromUtf8("line"));
    pHorLine->setFrameShape(QFrame::HLine);
    pHorLine->setFrameShadow(QFrame::Sunken);
    pGridLayoutUserProxy->addWidget(pHorLine,2,0, 1, 4);

    // Row 3 -------------------------------------------------------------------------------
    QLabel *pUserLabel = new QLabel(QString("User:"));
    pGridLayoutUserProxy->addWidget(pUserLabel,3,0);
    uiProxy->lineEditUsername = new QLineEdit();
    pGridLayoutUserProxy->addWidget(uiProxy->lineEditUsername, 3, 1, 1, 3);

    // Row 4 -------------------------------------------------------------------------------
    QLabel *pPasswordLabel = new QLabel(QString("Password:"));
    pGridLayoutUserProxy->addWidget(pPasswordLabel,4,0);
    uiProxy->lineEditPassword = new QLineEdit();
    uiProxy->lineEditPassword->setEchoMode(QLineEdit::PasswordEchoOnEdit );
    pGridLayoutUserProxy->addWidget(uiProxy->lineEditPassword, 4, 1, 1, 3);

    // Groupbox
    QGroupBox* pGroupBoxProxyDialog = new QGroupBox();
    pGroupBoxProxyDialog->setLayout(pGridLayoutUserProxy);
    pGroupBoxProxyDialog->setTitle("Proxy settings");
    pGroupBoxProxyDialog->setDisabled(true);
    connect(uiProxy->radioButtonAutoProxy, SIGNAL(toggled(bool)), pGroupBoxProxyDialog, SLOT(setDisabled(bool)));
    connect(uiProxy->radioButtonUserProxy, SIGNAL(toggled(bool)), pGroupBoxProxyDialog, SLOT(setEnabled(bool)));
    uiProxy->radioButtonUserProxy->setChecked(false);

    // Create pushbuttons
    QPushButton *pPushButtonOK = new QPushButton(QString("OK"));
    connect(pPushButtonOK, SIGNAL(clicked()), this, SLOT(on_buttonBox_accepted()));
    connect(pPushButtonOK, SIGNAL(clicked()), this, SLOT(accept()));
    QPushButton *pPushButtonCancel = new QPushButton(QString("Cancel"));
    connect(pPushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));

    QHBoxLayout *pHBoxLayoutButtons = new QHBoxLayout();
    pHBoxLayoutButtons->addWidget(pPushButtonOK);
    pHBoxLayoutButtons->addWidget(pPushButtonCancel);
    pHBoxLayoutButtons->setAlignment(Qt::AlignRight);
    pHBoxLayoutButtons->setSizeConstraint(pHBoxLayoutButtons->SetFixedSize);

    // Create overlaying layout
    QVBoxLayout *pVBoxLayoutButtons = new QVBoxLayout();
    pVBoxLayoutButtons->addLayout(pGridLayoutAutoProxy);
    pVBoxLayoutButtons->addWidget(uiProxy->radioButtonUserProxy);
    pVBoxLayoutButtons->addWidget(pGroupBoxProxyDialog);
    pVBoxLayoutButtons->addLayout(pHBoxLayoutButtons);
    pVBoxLayoutButtons->setSizeConstraint(pVBoxLayoutButtons->SetFixedSize);

    mProxyTypeNamesMap["Socks5 Proxy"] = QNetworkProxy::Socks5Proxy;
    mProxyTypeNamesMap["Http Proxy"] = QNetworkProxy::HttpProxy;
    mProxyTypeNamesMap["HttpCaching Proxy"] = QNetworkProxy::HttpCachingProxy;
    mProxyTypeNamesMap["FtpCaching Proxy"] = QNetworkProxy::FtpCachingProxy;

    uiProxy->comboBoxProxyType->addItems(mProxyTypeNamesMap.keys());
    uiProxy->comboBoxProxyType->setCurrentIndex(2); // == uiProxy->comboBoxProxyType->findText("HttpCaching Proxy");

    this->setLayout(pVBoxLayoutButtons);
    this->setWindowTitle(QString("Proxy Setup Dialog"));

    connect(uiProxy->lineEditProxyIp, SIGNAL(textChanged(QString)), this, SLOT(slot_editingIpField(QString)));
    connect(uiProxy->lineEditProxyIp, SIGNAL(editingFinished()), this, SLOT(slot_editingIpFieldFinished()));

    uiProxy->pProxySrcDlg = new ProxySourcesDialog(mpAutoProxy, mpParent);
    connect(pPushButtonPreferences,SIGNAL(pressed()), this, SLOT(slot_openProxySourceDialog()));

#if DISABLE_AUTOPROXY
    // DISABLED IN HDH 1.1.1
    uiProxy->radioButtonAutoProxy->setChecked(false);
    uiProxy->radioButtonAutoProxy->setDisabled(true);
    uiProxy->radioButtonAutoProxy->setCheckable(false);
    uiProxy->radioButtonUserProxy->setChecked(true);
#endif
}

/*-----------------------------------------------------------------------------------------------*
 * Change over to proxy source dialog
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::slot_openProxySourceDialog(void)
{
    uiProxy->pProxySrcDlg->setProxySourceList(mpAutoProxy->getProxySourceList());
    uiProxy->pProxySrcDlg->setUsProxySourceList(mpAutoProxy->getUsProxySourceList());
    uiProxy->pProxySrcDlg->slot_updateProxySourcesList();
    if (uiProxy->pProxySrcDlg->exec() == QDialog::Accepted)
    {
        mpAutoProxy->setProxySourceList(uiProxy->pProxySrcDlg->getProxySourceList());
        mpAutoProxy->setUsProxySourceList(uiProxy->pProxySrcDlg->getUsProxySourceList());
        mpAutoProxy->generateAutoProxySourceFile();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Change over to port input field
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::slot_editingIpField(QString sIpString)
{
    if (sIpString.contains(":"))
    {
        uiProxy->lineEditProxyPort->setFocus();
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Split IP and and port number
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::slot_editingIpFieldFinished()
{
   QString sIpString = uiProxy->lineEditProxyIp->text();

    sIpString.remove(QChar(' '));
    uiProxy->lineEditProxyIp->setText(sIpString);

    if (sIpString.contains(":"))
    {
        uiProxy->lineEditProxyPort->setText(sIpString.mid(sIpString.indexOf(":")+1));
        uiProxy->lineEditProxyIp->setText(sIpString.mid(0, sIpString.indexOf(":")));
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Get values from dialog
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::on_buttonBox_accepted()
{
    mbAutoProxy = uiProxy->radioButtonAutoProxy->isChecked();

    if (!mbAutoProxy)
    {
        msProxyUsername   = uiProxy->lineEditUsername->text();
        msProxyPassword   = uiProxy->lineEditPassword->text();

        msProxyIp         = uiProxy->lineEditProxyIp->text();
        msProxyIp.remove(QChar(' '));
        QString sProxyPort = uiProxy->lineEditProxyPort->text();
        bool ok;
        miProxyPort       = sProxyPort.toInt(&ok, 10);

        if (!ok)
        {
            uiProxy->lineEditProxyPort->setText("failed!");
            msProxyIp = "";
        }

        miProxyType = mProxyTypeNamesMap[uiProxy->comboBoxProxyType->currentText()];
    }
    else
    {
        mbUsProxyOnly = uiProxy->comboBoxProxyCountry->currentText().contains("US");
    }

    saveXmlProxySetup();
}


/*-----------------------------------------------------------------------------------------------*
 * Save pagesetup.xml configuration file
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::saveXmlProxySetup(void)
{
    // Store Settings in proxysetup.xml file
    QString sPageSetup;
    sPageSetup.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    sPageSetup.append("<proxysetup>\n");
    sPageSetup.append("<proxyip>" +  msProxyIp + "</proxyip>\n");
    sPageSetup.append("<proxyport>" + QString::number(miProxyPort) + "</proxyport>\n");
    sPageSetup.append("<proxyusername>" + msProxyUsername + "</proxyusername>\n");
    sPageSetup.append("<proxypasswordc>" + encryptString(msProxyPassword) + "</proxypasswordc>\n");
    sPageSetup.append("<proxytype>" + QString::number(miProxyType) + "</proxytype>\n");
    sPageSetup.append("<proxyautoproxy>" + QString::number(mbAutoProxy) + "</proxyautoproxy>\n");
    sPageSetup.append("<proxyusonly>" + QString::number(mbUsProxyOnly) + "</proxyusonly>\n");
    sPageSetup.append("</proxysetup>\n");
    writeFile(this, QString("proxysetup.xml"), &sPageSetup);
}

/*-----------------------------------------------------------------------------------------------*
 * Apply pagesetup.xml configuration file
 *-----------------------------------------------------------------------------------------------*/
int ProxyDialog::setXmlProxySetup(QNetworkProxy *pUserProxy)
{
    QString sProxySetup = readFile(QString("proxysetup.xml"));

    if (sProxySetup.isEmpty())
        return 0;

    bool ok;
    int iTagValue;
    bool bTagValue;

    QString sTagValue  = myXmlReader(&sProxySetup, "proxyip");
    if (!sTagValue.isEmpty())
        msProxyIp = sTagValue;

    sTagValue  = myXmlReader(&sProxySetup, "proxyport");
    if (!sTagValue.isEmpty())
    {
        iTagValue = sTagValue.toInt(&ok,10);
        if (ok)
            miProxyPort= iTagValue;
    }

    sTagValue  = myXmlReader(&sProxySetup, "proxytype");
    if (!sTagValue.isEmpty())
    {
        iTagValue = sTagValue.toInt(&ok,10);
        if (ok)
            miProxyType = iTagValue;
    }

    sTagValue  = myXmlReader(&sProxySetup, "proxyusername");
    msProxyUsername = sTagValue;

    sTagValue  = myXmlReader(&sProxySetup, "proxypasswordc");
    if (sTagValue.isEmpty())
    {
        // Plain password - compatiblity to previous versions
        sTagValue  = myXmlReader(&sProxySetup, "proxypassword");
        msProxyPassword = sTagValue;
    }
    else
    {
        // Encrypted passwaord
        msProxyPassword = encryptString(sTagValue);
    }

    sTagValue  = myXmlReader(&sProxySetup, "proxyautoproxy");
    if (!sTagValue.isEmpty())
    {
        bTagValue = sTagValue.toInt(&ok,10);
        if (ok)
            mbAutoProxy = bTagValue;
    }

    sTagValue  = myXmlReader(&sProxySetup, "proxyusonly");
    if (!sTagValue.isEmpty())
    {
        bTagValue = sTagValue.toInt(&ok,10);
        if (ok)
        {
            mbUsProxyOnly = bTagValue;
        }
    }

    // Apply settings to proxy
    pUserProxy->setHostName(msProxyIp);
    pUserProxy->setPort(miProxyPort);
    pUserProxy->setUser(msProxyUsername);
    pUserProxy->setPassword(msProxyPassword);
    pUserProxy->setType((QNetworkProxy::ProxyType) miProxyType);

    mpAutoProxy->setAutoProxyState(mbAutoProxy);
    mpAutoProxy->setUsProxyOnlyState(mbUsProxyOnly);

    this->fillProxySetupDialog();

    return ok;
}

/*-----------------------------------------------------------------------------------------------*
 * Encrypt/Decrypt Proxy password
 *-----------------------------------------------------------------------------------------------*/
QString ProxyDialog::encryptString(QString sInput)
{
    // Encryption key: change this string to create your personal key (used for en- and decryption)
    QString sKey = "%$#>#%232s+as#l)URa0$!@";
    // Chiffre / plain text
    QString strEncrypted;

    for (int i = 0; i < sInput.length(); i++)
    {
        strEncrypted.append(QChar(sInput.at(i).toLatin1() ^ sKey.at(i % sKey.length()).toLatin1()));
    }

    return strEncrypted;
}

/*-----------------------------------------------------------------------------------------------*
 * Fill proxy dialog with used settings
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::fillProxySetupDialog()
{
    // User Proxy
    uiProxy->lineEditProxyIp->setText(msProxyIp);
    uiProxy->lineEditProxyPort->setText(QString::number(miProxyPort));
    uiProxy->lineEditUsername->setText(msProxyUsername);
    uiProxy->lineEditPassword->setText(msProxyPassword);
    int iIdx = uiProxy->comboBoxProxyType->findText(mProxyTypeNamesMap.key( (QNetworkProxy::ProxyType) miProxyType));
    if (iIdx < 0)
        iIdx = 2;
    uiProxy->comboBoxProxyType->setCurrentIndex(iIdx);

    // AutoProxy
    uiProxy->radioButtonAutoProxy->setChecked(mbAutoProxy);
    uiProxy->comboBoxProxyCountry->setEnabled(mbAutoProxy);
    uiProxy->radioButtonUserProxy->setChecked(!mbAutoProxy);
    uiProxy->comboBoxProxyCountry->setCurrentIndex((int)!mbUsProxyOnly);
    // Enable automatic proxy activation
    uiProxy->checkBoxAutomaticProxy->setChecked(mbAutomaticProxy);
}

/*-----------------------------------------------------------------------------------------------*
 * Return proxy IP
 *-----------------------------------------------------------------------------------------------*/
QString ProxyDialog::getProxyIp()
{
    return msProxyIp;
}

/*-----------------------------------------------------------------------------------------------*
 * Set proxy IP
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::setProxyIp(QString sProxyIp)
{
    msProxyIp = sProxyIp;
    fillProxySetupDialog();
}


/*-----------------------------------------------------------------------------------------------*
 * Return proxy port
 *-----------------------------------------------------------------------------------------------*/
int ProxyDialog::getProxyPort()
{
    return miProxyPort;
}

/*-----------------------------------------------------------------------------------------------*
 * Set proxy port
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::setProxyPort(int iProxyPort)
{
   miProxyPort = iProxyPort;
   fillProxySetupDialog();
}

/*-----------------------------------------------------------------------------------------------*
 * Return proxy username
 *-----------------------------------------------------------------------------------------------*/
QString ProxyDialog::getProxyUsername()
{
    return msProxyUsername;
}

/*-----------------------------------------------------------------------------------------------*
 * Set proxy username
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::setProxyUsername(QString sProxyUsername)
{
    msProxyUsername = sProxyUsername;
    fillProxySetupDialog();
}

/*-----------------------------------------------------------------------------------------------*
 * Return proxy password
 *-----------------------------------------------------------------------------------------------*/
QString ProxyDialog::getProxyPassword()
{
    return msProxyPassword;
}

/*-----------------------------------------------------------------------------------------------*
 * Set proxy password
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::setProxyPassword( QString sProxyPassword)
{
    msProxyPassword = sProxyPassword;
    fillProxySetupDialog();
}

/*-----------------------------------------------------------------------------------------------*
 * Return proxy type
 *-----------------------------------------------------------------------------------------------*/
QNetworkProxy::ProxyType ProxyDialog::getProxyType()
{
    return (QNetworkProxy::ProxyType) miProxyType;
}

/*-----------------------------------------------------------------------------------------------*
 * Set proxy type
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::setProxyType(int iProxyType)
{
   miProxyType = iProxyType;
   fillProxySetupDialog();
}

/*-----------------------------------------------------------------------------------------------*
 * Return proxy type
 *-----------------------------------------------------------------------------------------------*/
bool ProxyDialog::getAutoProxy()
{
    return mbAutoProxy;
}

/*-----------------------------------------------------------------------------------------------*
 * Set proxy type
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::setAutoProxy(bool bAutoProxy)
{
   mbAutoProxy = bAutoProxy;
   fillProxySetupDialog();
}

/*-----------------------------------------------------------------------------------------------*
 * Return proxy type
 *-----------------------------------------------------------------------------------------------*/
bool ProxyDialog::getAutoProxyActivation(void)
{
    return mbAutomaticProxy;
}

/*-----------------------------------------------------------------------------------------------*
 * Set automatic proxy support
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::setAutomaticProxyActivation(bool bAutomaticProxy)
{
   mbAutomaticProxy = bAutomaticProxy;
   fillProxySetupDialog();
}

/*-----------------------------------------------------------------------------------------------*
 * Set automatic proxy support
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::slot_setAutomaticProxyActivation(bool bAutomaticProxy)
{
    mbAutomaticProxy = bAutomaticProxy;
}

/*-----------------------------------------------------------------------------------------------*
 * Return US only proxy state
 *-----------------------------------------------------------------------------------------------*/
bool ProxyDialog::getUsProxyOnlyState()
{
    return mbUsProxyOnly;
}

/*-----------------------------------------------------------------------------------------------*
 * Set US only proxy state
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::setUsProxyOnlyState(bool bUsOnlyProxyState)
{
    mbUsProxyOnly = bUsOnlyProxyState;
    fillProxySetupDialog();
}

/*-----------------------------------------------------------------------------------------------*
 * Enable / Disable manual proxy input fields
 *-----------------------------------------------------------------------------------------------*/
void ProxyDialog::slot_enableProxyFields(bool bEnable)
{
    uiProxy->lineEditProxyIp->setEnabled(bEnable);
    uiProxy->lineEditProxyPort->setEnabled(bEnable);
    uiProxy->lineEditUsername->setEnabled(bEnable);
    uiProxy->lineEditPassword->setEnabled(bEnable);
    uiProxy->comboBoxProxyType->setEnabled(bEnable);
    uiProxy->comboBoxProxyCountry->setDisabled(bEnable);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ProxySourcesDialog class
///////////////////////////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------------------------*
 * Dialog Constructor
 *-----------------------------------------------------------------------------------------------*/
ProxySourcesDialog::ProxySourcesDialog(AutoProxy *pAutoProxy, QWidget *parent) :
    QDialog(parent),
    mpAutoProxy(pAutoProxy),
    mpParent(parent)
{
    createProxySourcesDialog();
    mbCancelCheckProxies = false;

    // Init default proxy sources
    msProxySourceList   = mpAutoProxy->getProxySourceList();
    msUsProxySourceList = mpAutoProxy->getUsProxySourceList();
}

/*-----------------------------------------------------------------------------------------------*
 * Dialog Destructor
 *-----------------------------------------------------------------------------------------------*/
ProxySourcesDialog::~ProxySourcesDialog()
{
   //
}

/*-----------------------------------------------------------------------------------------------*
 * Create proxy sources dialog
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::createProxySourcesDialog()
{
    mpProxyAddDialog = new ProxyAddDialog(mpAutoProxy,
                                          mpParent);
    connect(mpProxyAddDialog, SIGNAL(proxySourceAdded()), this, SLOT(slot_updateProxySourcesList()));

    QGridLayout *pGridLayout        = new QGridLayout;
    QVBoxLayout *pVBoxLayoutCntrl   = new QVBoxLayout;

    // Dialog Cntrl ----------------------------------------------------------------------------
    mpPushButtonOk      = new QPushButton(QString("Ok"));
    connect(mpPushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    pVBoxLayoutCntrl->addWidget(mpPushButtonOk);

    mpPushButtonCancel  = new QPushButton(QString("Cancel"));
    connect(mpPushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(this, SIGNAL(rejected()), this, SLOT(slot_checkProxiesCancel()));
    pVBoxLayoutCntrl->addWidget(mpPushButtonCancel);

    mpPushButtonCheck  = new QPushButton(QString("Check\nProxies"));
    mpPushButtonCheck->setToolTip(QString("Checks if proxy sources are compatible to HDH proxy list extraction methods.\nURLs might be blocked for some time due to too frequent requests."));
    connect(mpPushButtonCheck, SIGNAL(clicked()), this, SLOT(slot_checkProxies()));
    pVBoxLayoutCntrl->addWidget(mpPushButtonCheck);

    mpPushButtonDefault  = new QPushButton(QString("Restore\nDefault"));
    connect(mpPushButtonDefault, SIGNAL(clicked()), this, SLOT(slot_restoreDefault()));
    pVBoxLayoutCntrl->addWidget(mpPushButtonDefault);

    pGridLayout->addLayout(pVBoxLayoutCntrl, 0, 1, 1, 1);

    // Prxy List Cntrl -------------------------------------------------------------------------
    QVBoxLayout* pVBoxLayoutCntrlPrxy   = new QVBoxLayout;
    QSpacerItem* spacer1                = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pVBoxLayoutCntrlPrxy->addItem(spacer1);

    mpPushButtonAddProxy    = new QPushButton(QString("Add Proxy"));
    mpPushButtonAddProxy->setToolTip(QString("For adding URLs with internaltional proxy lists"));
    connect(mpPushButtonAddProxy, SIGNAL(clicked()), this, SLOT(on_pushButtonAddProxy_clicked()));
    pVBoxLayoutCntrlPrxy->addWidget(mpPushButtonAddProxy);

    mpPushButtonRemoveProxy = new QPushButton(QString("Remove Proxy"));
    connect(mpPushButtonRemoveProxy, SIGNAL(clicked()), this, SLOT(on_pushButtonRemoveProxy_clicked()));
    pVBoxLayoutCntrlPrxy->addWidget(mpPushButtonRemoveProxy);

    pGridLayout->addLayout(pVBoxLayoutCntrlPrxy, 1, 1, 1, 1);

    // US Proxy ListCntrl ------------------------------------------------------------------------
    QVBoxLayout *pVBoxLayoutCntrlUsPrxy     = new QVBoxLayout;
    QSpacerItem* spacer2              = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    pVBoxLayoutCntrlUsPrxy->addItem(spacer2);

    mpPushButtonAddUsProxy    = new QPushButton(QString("Add US Proxy"));
    mpPushButtonAddUsProxy->setToolTip(QString("For adding URLs with US-only proxy lists"));
    connect(mpPushButtonAddUsProxy, SIGNAL(clicked()), this, SLOT(on_pushButtonAddUsProxy_clicked()));
    pVBoxLayoutCntrlUsPrxy->addWidget(mpPushButtonAddUsProxy);

    mpPushButtonRemoveUsProxy = new QPushButton(QString("Remove US Proxy"));
    connect(mpPushButtonRemoveUsProxy, SIGNAL(clicked()), this, SLOT(on_pushButtonRemoveUsProxy_clicked()));
    pVBoxLayoutCntrlUsPrxy->addWidget(mpPushButtonRemoveUsProxy);
    pGridLayout->addLayout(pVBoxLayoutCntrlUsPrxy, 2, 1, 1, 1);

    // Proxy source List ---------------------------------------------------------------------------
    uiUrlSelect                             = &GuiProxySourcesDialog;
    QGroupBox* pGroupBoxProxyList           = new QGroupBox();
    pGroupBoxProxyList->setTitle("Proxy Sources");
    uiUrlSelect->listWidgetProxyUrls        = new QListWidget();
    QVBoxLayout *pVBoxLayoutProxyList       = new QVBoxLayout();
    pVBoxLayoutProxyList->addWidget(uiUrlSelect->listWidgetProxyUrls);
    connect( uiUrlSelect->listWidgetProxyUrls, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(slot_updateProxySourcesOnChange(QListWidgetItem*)));
    connect( uiUrlSelect->listWidgetProxyUrls, SIGNAL(itemSelectionChanged()), this, SLOT(slot_clearFocus()));
    pGroupBoxProxyList->setLayout(pVBoxLayoutProxyList);
    pGridLayout->addWidget(pGroupBoxProxyList, 0, 0, 2, 1);

    // US Proxy source List ------------------------------------------------------------------------
    QGroupBox* pGroupBoxProxyListUsOnly     = new QGroupBox();
    pGroupBoxProxyListUsOnly->setTitle("US only Proxy Sources");
    uiUrlSelect->listWidgetProxyUsOnlyUrls  = new QListWidget();
    QVBoxLayout *pVBoxLayoutProxyLisUsOnlyt = new QVBoxLayout();
    pVBoxLayoutProxyLisUsOnlyt->addWidget(uiUrlSelect->listWidgetProxyUsOnlyUrls);
    connect( uiUrlSelect->listWidgetProxyUsOnlyUrls, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(slot_updateProxySourcesOnChange(QListWidgetItem*)));
    connect( uiUrlSelect->listWidgetProxyUsOnlyUrls, SIGNAL(itemSelectionChanged()), this, SLOT(slot_clearFocus()));
    pGroupBoxProxyListUsOnly->setLayout(pVBoxLayoutProxyLisUsOnlyt);
    pGridLayout->addWidget(pGroupBoxProxyListUsOnly, 2, 0, 1, 1);

    this->setLayout(pGridLayout);
    this->setWindowTitle(QString("Proxy Sources Dialog"));
}

/*-----------------------------------------------------------------------------------------------*
 * get proxy sources list
 *-----------------------------------------------------------------------------------------------*/
QStringList ProxySourcesDialog::getProxySourceList(void)
{
    return msProxySourceList;
}

/*-----------------------------------------------------------------------------------------------*
 * set proxy sources list
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::setProxySourceList(QStringList sProxySourceList)
{
    msProxySourceList = sProxySourceList;
}

/*-----------------------------------------------------------------------------------------------*
 * get US proxy sources list
 *-----------------------------------------------------------------------------------------------*/
QStringList ProxySourcesDialog::getUsProxySourceList(void)
{
    return msUsProxySourceList;
}

/*-----------------------------------------------------------------------------------------------*
 * set US proxy sources list
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::setUsProxySourceList(QStringList sUsProxySourceList)
{
    msUsProxySourceList = sUsProxySourceList;
}

/*-----------------------------------------------------------------------------------------------*
 * Check proxies from lists
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::slot_checkProxies()
{
    if (mbCancelCheckProxies)
    {
        miproxy = 0;
        mbCancelCheckProxies = false;
    }

    if (miproxy == 0)
    {
        // Disable gui elements
        mpPushButtonOk->setEnabled(false);
        mpPushButtonCheck->setEnabled(false);
        mpPushButtonDefault->setEnabled(false);
        mpPushButtonAddProxy->setEnabled(false);
        mpPushButtonRemoveProxy->setEnabled(false);
        mpPushButtonAddUsProxy->setEnabled(false);
        mpPushButtonRemoveUsProxy->setEnabled(false);

        disconnect(mpPushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
        connect(mpPushButtonCancel, SIGNAL(clicked()), this, SLOT(slot_checkProxiesCancel()));

        disconnect(mpAutoProxy, SIGNAL(updateAutoProxyCheckStateResult(bool)), mpProxyAddDialog, SLOT(slot_urlCheckState(bool)));
        connect(mpAutoProxy, SIGNAL(updateAutoProxyCheckStateResult(bool)), this, SLOT(slot_proxyCheckState(bool)));

        // Reset background color
        for(int i=0; i <(uiUrlSelect->listWidgetProxyUrls->count() + uiUrlSelect->listWidgetProxyUsOnlyUrls->count());i++)
        {
            if (i < uiUrlSelect->listWidgetProxyUrls->count())
                uiUrlSelect->listWidgetProxyUrls->item( i)->setBackground( QColor("white"));
            else
                uiUrlSelect->listWidgetProxyUsOnlyUrls->item(i - msProxySourceList.length())->setBackground( QColor("white"));
        }
    }

    if (miproxy < (msProxySourceList.length() + msUsProxySourceList.length()) )
    {
        if ( miproxy < msProxySourceList.length())
        {
            mpAutoProxy->checkProxyServerSource(msProxySourceList.at(miproxy));
        }
        else
        {
            mpAutoProxy->checkProxyServerSource(msUsProxySourceList.at(miproxy - msProxySourceList.length()));
        }
        miproxy++;
    }
    else
    {
        // Enable gui elements
        mpPushButtonOk->setEnabled(true);
        mpPushButtonCheck->setEnabled(true);
        mpPushButtonDefault->setEnabled(true);
        mpPushButtonAddProxy->setEnabled(true);
        mpPushButtonRemoveProxy->setEnabled(true);
        mpPushButtonAddUsProxy->setEnabled(true);
        mpPushButtonRemoveUsProxy->setEnabled(true);

        disconnect(mpPushButtonCancel, SIGNAL(clicked()), this, SLOT(slot_checkProxiesCancel()));
        connect(mpPushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));

        disconnect(mpAutoProxy, SIGNAL(updateAutoProxyCheckStateResult(bool)), this, SLOT(slot_proxyCheckState(bool)));
        connect(mpAutoProxy, SIGNAL(updateAutoProxyCheckStateResult(bool)), mpProxyAddDialog, SLOT(slot_urlCheckState(bool)));

        disconnect(this, SIGNAL(rejected()), mpPushButtonCancel, SIGNAL(clicked()));

        miproxy = 0;
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Cancel check proxies from lists
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::slot_checkProxiesCancel()
{
    mbCancelCheckProxies = true;
    mpAutoProxy->abortAutoProxyRequest();

    // Enable gui elements
    mpPushButtonOk->setEnabled(true);
    mpPushButtonCheck->setEnabled(true);
    mpPushButtonDefault->setEnabled(true);
    mpPushButtonAddProxy->setEnabled(true);
    mpPushButtonRemoveProxy->setEnabled(true);
    mpPushButtonAddUsProxy->setEnabled(true);
    mpPushButtonRemoveUsProxy->setEnabled(true);

    disconnect(mpPushButtonCancel, SIGNAL(clicked()), this, SLOT(slot_checkProxiesCancel()));
    connect(mpPushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));

    disconnect(mpAutoProxy, SIGNAL(updateAutoProxyCheckStateResult(bool)), this, SLOT(slot_proxyCheckState(bool)));
    connect(mpAutoProxy, SIGNAL(updateAutoProxyCheckStateResult(bool)), mpProxyAddDialog, SLOT(slot_urlCheckState(bool)));

    disconnect(this, SIGNAL(rejected()), mpPushButtonCancel, SIGNAL(clicked()));
}

/*-----------------------------------------------------------------------------------------------*
 * Highlight checked proxies from lists
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::slot_proxyCheckState(bool bSuccess)
{
    static int iproxy = miproxy -1;

    if (mbCancelCheckProxies)
    {
        iproxy = 0;
        mbCancelCheckProxies = false;

        return;
    }

    iproxy = miproxy -1;
    if (iproxy >= (msProxySourceList.length() + msUsProxySourceList.length()))
            iproxy = 0;

    if (iproxy < (msProxySourceList.length() + msUsProxySourceList.length()) )
    {
        if ( iproxy < msProxySourceList.length())
        {
           if (iproxy < uiUrlSelect->listWidgetProxyUrls->count())
           {
                if (bSuccess)
                    uiUrlSelect->listWidgetProxyUrls->item( iproxy)->setBackground(QColor("lightgreen"));
                else
                    uiUrlSelect->listWidgetProxyUrls->item( iproxy)->setBackground( QColor("red"));
           }
        }
        else
        {
            if (iproxy < (uiUrlSelect->listWidgetProxyUsOnlyUrls->count()+uiUrlSelect->listWidgetProxyUrls->count()))
            {
                if (bSuccess)
                    uiUrlSelect->listWidgetProxyUsOnlyUrls->item(iproxy- msProxySourceList.length())->setBackground(QColor("lightgreen"));
                else
                    uiUrlSelect->listWidgetProxyUsOnlyUrls->item(iproxy - msProxySourceList.length())->setBackground( QColor("red"));
            }
        }
    }
    iproxy++;
    slot_checkProxies();
}

/*-----------------------------------------------------------------------------------------------*
 * Restore default proxy sources list
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::slot_restoreDefault(void)
{
    uiUrlSelect->listWidgetProxyUsOnlyUrls->clear();
    uiUrlSelect->listWidgetProxyUrls->clear();

    // Init default proxy sources
    mpAutoProxy->initDefaultProxySources();
    msProxySourceList = mpAutoProxy->getProxySourceList();
    msUsProxySourceList = mpAutoProxy->getUsProxySourceList();
    // Update lists
    slot_updateProxySourcesList();
}

/*-----------------------------------------------------------------------------------------------*
 * Update proxy sources list on change
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::slot_updateProxySourcesOnChange(QListWidgetItem* changedItem)
{
    int iRow = changedItem->listWidget()->row(changedItem);
    if (changedItem->listWidget() == uiUrlSelect->listWidgetProxyUrls)
    {
        if ( (iRow >= 0) && (iRow < msProxySourceList.length()))
        {
            msProxySourceList.replace(iRow, changedItem->text());
        }
    }
    else
    {
        if ( (iRow >= 0) && (iRow < msUsProxySourceList.length()))
        {
            msUsProxySourceList.replace(iRow, changedItem->text());
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Clear focus
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::slot_clearFocus(void)
{
    if (uiUrlSelect->listWidgetProxyUrls->hasFocus())
    {
        uiUrlSelect->listWidgetProxyUsOnlyUrls->clearSelection();
        uiUrlSelect->listWidgetProxyUsOnlyUrls->setCurrentItem(0);
    }

    if (uiUrlSelect->listWidgetProxyUsOnlyUrls->hasFocus())
    {
        uiUrlSelect->listWidgetProxyUrls->clearSelection();
        uiUrlSelect->listWidgetProxyUrls->setCurrentItem(0);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Update proxy sources list
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::slot_updateProxySourcesList()
{
    // Clear list widgets
    uiUrlSelect->listWidgetProxyUrls->clear();
    uiUrlSelect->listWidgetProxyUsOnlyUrls->clear();

    // Fill proxy list with latest source urls
    QListWidgetItem *pUrlItem;
    foreach(QString sSourceUrl, msProxySourceList)
    {
        pUrlItem = new QListWidgetItem(sSourceUrl);
        pUrlItem->setFlags(pUrlItem->flags()|Qt::ItemIsEditable);
        uiUrlSelect->listWidgetProxyUrls->addItem(pUrlItem);       
    }

    // Fill US proxy list with latest source urls
    foreach(QString sSourceUrl, msUsProxySourceList)
    {
        pUrlItem = new QListWidgetItem(sSourceUrl);
        pUrlItem->setFlags(pUrlItem->flags()|Qt::ItemIsEditable);
        uiUrlSelect->listWidgetProxyUsOnlyUrls->addItem(pUrlItem);
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Open Add  proxy dialog
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::on_pushButtonAddProxy_clicked()
{
    mpProxyAddDialog->setActualProxySourceList(&msProxySourceList);
    mpProxyAddDialog->exec();
    slot_updateProxySourcesList();
}

/*-----------------------------------------------------------------------------------------------*
 * Open Add US proxy dialog
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::on_pushButtonAddUsProxy_clicked()
{
    mpProxyAddDialog->setActualProxySourceList(&msUsProxySourceList);
    mpProxyAddDialog->exec();
    slot_updateProxySourcesList();
}

/*-----------------------------------------------------------------------------------------------*
 * Remove proxy from list action
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::on_pushButtonRemoveProxy_clicked()
{
    QListWidgetItem *pCurrentItem = uiUrlSelect->listWidgetProxyUrls->currentItem();
    if (pCurrentItem != NULL )
    {
        int iItemIdx = msProxySourceList.indexOf(pCurrentItem->text());
        if (iItemIdx > -1)
        {
            msProxySourceList.removeAt(iItemIdx);
            delete pCurrentItem;
        }
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Remove proxy from list action
 *-----------------------------------------------------------------------------------------------*/
void ProxySourcesDialog::on_pushButtonRemoveUsProxy_clicked()
{
    QListWidgetItem *pCurrentItem = uiUrlSelect->listWidgetProxyUsOnlyUrls->currentItem();
    if (pCurrentItem != NULL )
    {
        int iItemIdx = msUsProxySourceList.indexOf(pCurrentItem->text());
        if (iItemIdx > -1)
        {
            msUsProxySourceList.removeAt(iItemIdx);
            delete pCurrentItem;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ProxyAddDialog class
///////////////////////////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------------------------*
 * Dialog Constructor
 *-----------------------------------------------------------------------------------------------*/
ProxyAddDialog::ProxyAddDialog(AutoProxy *pAutoProxy,
                               QWidget *parent) :
  QDialog(parent),
  mpAutoProxy(pAutoProxy)
{
     QVBoxLayout *pVBoxLayoutCntrl  = new QVBoxLayout;
     QLabel *pLabelPlsEnter         = new QLabel("Please enter source url: (Hint: works best with https/sll proxies)");
     pVBoxLayoutCntrl->addWidget(pLabelPlsEnter);
     mpUrlLineEdit                  = new QLineEdit;
     mpUrlLineEdit->setPlaceholderText(QString("Enter proxy source url here"));
     pVBoxLayoutCntrl->addWidget(mpUrlLineEdit);
     QLabel *pLabelCheckState       = new QLabel("");
     pVBoxLayoutCntrl->addWidget(pLabelCheckState);
     mpLabelAddState       = new QLabel("");
     pVBoxLayoutCntrl->addWidget(mpLabelAddState);
     QHBoxLayout *pHBoxLayoutCntrl  = new QHBoxLayout;
     QSpacerItem* spacer1              = new QSpacerItem( 100, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
     pHBoxLayoutCntrl->addItem(spacer1);
     QPushButton *pAddButton         = new QPushButton("Add");
     pHBoxLayoutCntrl->addWidget(pAddButton, 0, Qt::AlignRight);
     QPushButton *pCloseButton     = new QPushButton("Close");
     pHBoxLayoutCntrl->addWidget(pCloseButton, 0, Qt::AlignRight);
     pVBoxLayoutCntrl->addLayout(pHBoxLayoutCntrl);

     connect(pCloseButton, SIGNAL(clicked()), this, SLOT(close()));
     connect(pCloseButton, SIGNAL(clicked()), mpLabelAddState, SLOT(clear()));
     connect(pCloseButton, SIGNAL(clicked()), pLabelCheckState, SLOT(clear()));
     connect(pCloseButton, SIGNAL(clicked()), mpUrlLineEdit, SLOT(clear()));
     connect(pCloseButton, SIGNAL(clicked()), this, SLOT(slot_checkProxiesCancel()));
     connect(pAddButton,   SIGNAL(clicked()), pLabelCheckState, SLOT(clear()));
     connect(pAddButton,   SIGNAL(clicked()), this, SLOT(on_pushButtonAdd_clicked()));
     connect(pAddButton,   SIGNAL(clicked(bool)), this,SLOT(setDisabled(bool)));
     connect(mpAutoProxy,  SIGNAL(updateAutoProxyCheckState(QString)), pLabelCheckState, SLOT(setText(QString)));
     connect(mpAutoProxy,  SIGNAL(updateAutoProxyCheckStateResult(bool)), this, SLOT(slot_urlCheckState(bool)));

     this->setLayout(pVBoxLayoutCntrl);
     this->setWindowTitle(QString("Add Proxy Sources Dialog"));

     Qt::WindowFlags flags = this->windowFlags();
     Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
     flags = flags & (~helpFlag);
     this->setWindowFlags(flags);
}

/*-----------------------------------------------------------------------------------------------*
 * Check user url
 *-----------------------------------------------------------------------------------------------*/
void ProxyAddDialog::on_pushButtonAdd_clicked()
{
    mpLabelAddState->clear();

    if (!mpUrlLineEdit->text().isEmpty())
    {
        mpUrlLineEdit->setText((QString) QUrl::fromPercentEncoding(mpUrlLineEdit->text().toLatin1()).toLatin1());

        if(!mpUrlLineEdit->text().contains("http"))
            mpUrlLineEdit->setText(mpUrlLineEdit->text().prepend("http://"));

        if (mpsActualProxySourceList->filter( mpUrlLineEdit->text(),Qt::CaseInsensitive).isEmpty())
            mpAutoProxy->checkProxyServerSource(mpUrlLineEdit->text());
        else
            mpLabelAddState->setText("Url already member of source list.");
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Check user url
 *-----------------------------------------------------------------------------------------------*/
void ProxyAddDialog::slot_urlCheckState(bool bSuccess)
{
    if (bSuccess)
    {
        mpsActualProxySourceList->append(mpUrlLineEdit->text());
        mpUrlLineEdit->setText("");
        mpLabelAddState->setText("Url has been added to source list.");

        emit proxySourceAdded();
    }
    else
    {
        mpLabelAddState->setText("");
    }
}

/*-----------------------------------------------------------------------------------------------*
 * Set actual proxy source list
 *-----------------------------------------------------------------------------------------------*/
void ProxyAddDialog::setActualProxySourceList(QStringList *psActualProxySourceList)
{
    mpsActualProxySourceList = psActualProxySourceList;
}

/*-----------------------------------------------------------------------------------------------*
 * Cancel request
 *-----------------------------------------------------------------------------------------------*/
void ProxyAddDialog::slot_checkProxiesCancel()
{
    mpAutoProxy->abortAutoProxyRequest();
}
