/**
 * OlympusCamera-RemoteControl: main function / main classes
 *
 * @copyright	2022 Siegfried Walz
 * @license     https://www.gnu.org/licenses/lgpl-3.0.txt GNU Lesser General Public License
 * @author      Siegfried Walz
 * @link        https://software.bswalz.de/
 * @package     OlyCamera-RC
 */
/*
 * This file is part of OlyCamera-RC
 *
 * OlyCamera-RC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OlyCamera-RC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OlyCamera-RC. If not, see <http://www.gnu.org/licenses/>.
 *
 * OlyCamera-RC was tested under Android 4.1/5.0 (SDK 16/21) and Linux and
 * was developped using Qt 5.12.12 resp. Qt 5.15.2 .
 */

#include "main.h"
#include "mainwindow.h"

#include <common/network/networkhelper.h> // Separate git-repo
#include <common/model/EnumParameter.h>   // Separate git-repo
#include <common/model/Parameter.h>		  // Separate git-repo

#include <QGuiApplication>
#include <QApplication>
#include <QRunnable>
#include <QLocale>
#include <QTranslator>
#include <QRunnable>
#include <QThreadPool>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkDatagram>
#include <QMessageBox>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
	QApplication app(argc, argv);
	QTranslator translator;

	const QStringList uiLanguages = QLocale::system().uiLanguages();
	for (const QString &locale : uiLanguages) {
		const QString baseName = "OlyCamera-RC_" + QLocale(locale).name();
		if (translator.load(":/i18n/" + baseName)) {
			app.installTranslator(&translator);
			break;
			}
		}

	MainWindow mainWindow;
	mainWindow.layoutUI();
	app.setWindowIcon(QIcon(":/res/icon.png"));

	de::bswalz::olycamerarc::CMainController::getInstance(); // Instantiates main controller
	de::bswalz::olycamerarc::CQMLBackend * pQMLBackend = de::bswalz::olycamerarc::CMainController::getInstance()->getQMLBackend();
	de::bswalz::olycamerarc::CMainController::getInstance()->init(&app, &mainWindow);

    mainWindow.setMainController(de::bswalz::olycamerarc::CMainController::getInstance());
    mainWindow.show();
	return app.exec();
}

namespace de { namespace bswalz { namespace olycamerarc {
namespace {
// -----------------------------------------------------------------------
// Anonymous class CNetworkObserver
// -----------------------------------------------------------------------
class CNetworkObserver : public QRunnable {
public:
	CNetworkObserver(CMainController * pMainController)
			: QRunnable(), m_pOwner(pMainController), m_StopRequest(false) {}
	virtual ~CNetworkObserver() {}
	virtual void run() override;
	void    stopRequest() { m_StopRequest = true; }
private:
	CMainController * m_pOwner;
	bool              m_StopRequest;
};

// -----------------------------------------------------------------------
// Anonymous debugging function
// -----------------------------------------------------------------------
const bool W_DEBUG_ENABLED = false;
void wDebug(const QString & s) {
	if (W_DEBUG_ENABLED)
		QMessageBox::information(nullptr, "Information", s);
}

// -----------------------------------------------------------------------
// Anonymous class CInitState
// -----------------------------------------------------------------------
class CInitState : public CMainStateMachine::IState {
public:
	static CInitState * getInstance();
	virtual void pressFocusButton(CMainStateMachine * pSource) override;
	virtual void commandsProcessed(CMainStateMachine * pSource, EOlyCommands) override;
	virtual void tearDown(CMainStateMachine * pSource) override;
	void		 init(CMainStateMachine * pSource);
	virtual EState getId() const override { return Init; }
private:
	CInitState() {}
	static std::unique_ptr<CInitState> m_upInstance;
};

// -----------------------------------------------------------------------
// Anonymous class CFocusRequestState
// -----------------------------------------------------------------------
class CFocusRequestState : public CMainStateMachine::IState {
public:
	static CFocusRequestState * getInstance();
	virtual void commandsProcessed(CMainStateMachine * pSource, EOlyCommands) override;
	virtual void error(CMainStateMachine * pSource) override { timeout(pSource); }
	virtual void timeout(CMainStateMachine * pSource) override;
	virtual void tearDown(CMainStateMachine * pSource) override;
	virtual EState getId() const override { return FocusRequest; }
private:
	CFocusRequestState() {}
	static std::unique_ptr<CFocusRequestState> m_upInstance;
};

// -----------------------------------------------------------------------
// Anonymous class CFocussedState
// -----------------------------------------------------------------------
class CFocussedState : public CMainStateMachine::IState {
public:
	static CFocussedState * getInstance();
	virtual void releaseFocusButton(CMainStateMachine * pSource) override;
	virtual void pressShutterButton(CMainStateMachine * pSource) override;
	virtual void tearDown(CMainStateMachine * pSource) override;
	virtual EState getId() const override { return Focussed; }
private:
	CFocussedState() {}
	static std::unique_ptr<CFocussedState> m_upInstance;
};

// -----------------------------------------------------------------------
// Anonymous class CFocusReleaseState
// -----------------------------------------------------------------------
class CFocusReleaseState : public CMainStateMachine::IState {
public:
	static CFocusReleaseState * getInstance();
	virtual void commandsProcessed(CMainStateMachine * pSource, EOlyCommands) override;
	virtual void error(CMainStateMachine * pSource) override { timeout(pSource); }
	virtual void timeout(CMainStateMachine * pSource) override;
	virtual void tearDown(CMainStateMachine * pSource) override;
	virtual EState getId() const override { return FocusRelease; }
private:
	CFocusReleaseState() {}
	static std::unique_ptr<CFocusReleaseState> m_upInstance;
};

// -----------------------------------------------------------------------
// Anonymous class CTriggerRequestState
// -----------------------------------------------------------------------
class CTriggerRequestState : public CMainStateMachine::IState {
public:
	static CTriggerRequestState * getInstance();
	virtual void releaseShutterButton(CMainStateMachine * pSource) override;
	virtual void commandsProcessed(CMainStateMachine * pSource, EOlyCommands) override;
	virtual void error(CMainStateMachine * pSource) override { timeout(pSource); }
	virtual void timeout(CMainStateMachine * pSource) override;
	virtual void tearDown(CMainStateMachine * pSource) override;
	virtual EState getId() const override { return TriggerRequest; }
private:
	CTriggerRequestState() {}
	static std::unique_ptr<CTriggerRequestState> m_upInstance;
};

// -----------------------------------------------------------------------
// Anonymous class CTriggeredState
// -----------------------------------------------------------------------
class CTriggeredState : public CMainStateMachine::IState {
public:
	static CTriggeredState * getInstance();
	virtual void releaseShutterButton(CMainStateMachine * pSource) override;
	virtual void tearDown(CMainStateMachine * pSource) override;
	virtual EState getId() const override { return Triggered; }
private:
	CTriggeredState() {}
	static std::unique_ptr<CTriggeredState> m_upInstance;
};

// -----------------------------------------------------------------------
// Anonymous class CTriggerReleaseState
// -----------------------------------------------------------------------
class CTriggerReleaseState : public CMainStateMachine::IState {
public:
	static CTriggerReleaseState * getInstance();
	virtual void commandsProcessed(CMainStateMachine * pSource, EOlyCommands) override;
	virtual void error(CMainStateMachine * pSource) override { timeout(pSource); }
	virtual void timeout(CMainStateMachine * pSource) override;
	virtual void tearDown(CMainStateMachine * pSource) override;
	virtual EState getId() const override { return TriggerRelease; }
private:
	CTriggerReleaseState() {}
	static std::unique_ptr<CTriggerReleaseState> m_upInstance;
};

} // End anonymous namespace

// -----------------------------------------------------------------------
// Class CRTPDatagramHandler
// -----------------------------------------------------------------------
class CRTPDatagramHandler : public mvc::Model {
public:
	CRTPDatagramHandler() : mvc::Model("RTPDatagramHandler"), m_PayloadNumber(0uL) {}
	virtual ~CRTPDatagramHandler() {}
	void	processDatagram(const QByteArray &);
	std::queue<QByteArray> & getPayloads() { return m_Payloads; }
private:
	QByteArray m_PartialPayload;
	std::queue<QByteArray> m_Payloads;
	u_int32_t  m_PayloadNumber;
};

// -----------------------------------------------------------------------
// Default Gateway of OlyCamera
// -----------------------------------------------------------------------
const std::string OLY_DEFAULT_GATEWAY = "192.168.0.10";
const std::string LOCAL_HOST          = "127.0.0.1";

// -----------------------------------------------------------------------
// Implementation of class CMainController
// -----------------------------------------------------------------------
std::unique_ptr<CMainController> CMainController::m_upInstance = std::unique_ptr<CMainController>();

// -----------------------------------------------------------------------
CMainController * CMainController::getInstance() {
	if (m_upInstance.get() == nullptr)
		m_upInstance.reset(new CMainController());
	return m_upInstance.get();
}

// -----------------------------------------------------------------------
void CMainController::init(QCoreApplication * pApp, QWidget * pRootWidget) {
	var_array<int> wifi_values = { (int)EWifiNotConnected, (int)EWifiConnected, (int)EWifiOlyCameraConnected };
    m_upWifiStatus.reset( new CEnumParameter("WifiStatus", EWifiNotConnected, wifi_values) );
    m_upLocalIpAddress.reset( new CAStringParameter("LocalIpAddress", LOCAL_HOST) );

	m_StateMachine.init();
	m_StateMachine.addListener(this);
	m_pNetworkAccessManager = new QNetworkAccessManager();
	m_pRTPDatagramHandler   = new CRTPDatagramHandler();

	m_QMLBackend.init(this, pRootWidget);

	connect(pApp, SIGNAL(aboutToQuit()), this, SLOT(tearDown()));
	connect(this, SIGNAL(dispatchExposurePropertiesRequest()), this, SLOT(_requestExposureProperties()), Qt::QueuedConnection);
	connect(this, SIGNAL(dispatchLifeViewImageRequest()), this, SLOT(_requestLifeViewImage()), Qt::QueuedConnection);
    connect(this, SIGNAL(dispatchCommandListRequest()),   this, SLOT(_requestCommandList()), Qt::QueuedConnection);
    connect(this, SIGNAL(notifyWifiStatusChanged()),      this, SLOT(_notifyWifiStatusChanged()), Qt::QueuedConnection);
	connect(this, SIGNAL(notifyCameraValueChanged(QVariant,QVariant)), pRootWidget, SLOT(notifyCameraValueChanged(QVariant,QVariant)));

	registerAt(m_upWifiStatus.get(), true /*Notifies QML widget*/);
    registerAt(m_pRTPDatagramHandler, false);

	QThreadPool::globalInstance()->start(m_pNetworkObserver);
}

// -----------------------------------------------------------------------
void CMainController::tearDown() {
	dynamic_cast<CNetworkObserver*>(m_pNetworkObserver)->stopRequest();
	disconnect(this, SIGNAL(dispatchExposurePropertiesRequest()), this, SLOT(_requestExposureProperties()));
	disconnect(this, SIGNAL(dispatchLifeViewImageRequest()), this, SLOT(_requestLifeViewImage()));
    disconnect(this, SIGNAL(dispatchCommandListRequest()),   this, SLOT(_requestCommandList()));
    disconnect(this, SIGNAL(notifyWifiStatusChanged()),      this, SLOT(_notifyWifiStatusChanged()));
	if (m_pUDPServerSocket != nullptr)
		disconnect(((QIODevice*)m_pUDPServerSocket), SIGNAL(readyRead()), this, SLOT(udpReadyRead()));
	m_QMLBackend.tearDown();
	QThread::msleep(800);

	m_StateMachine.tearDown();

	unregisterAt(m_pRTPDatagramHandler);
    unregisterAt(m_upWifiStatus.get());
	if (m_pUDPServerSocket != nullptr)
		delete m_pUDPServerSocket;
	delete m_pRTPDatagramHandler;
	delete m_pNetworkAccessManager;
	m_pUDPServerSocket      = nullptr;
	m_pRTPDatagramHandler   = nullptr;
	m_pNetworkAccessManager = nullptr;
}

// -----------------------------------------------------------------------
// Inherited from View
void CMainController::update(const Model * pModel, void * pObject) {
	if (pModel == m_upWifiStatus.get())
		updateWifiStatus();
	else if (pModel == m_pRTPDatagramHandler) {
		const QByteArray & image = m_pRTPDatagramHandler->getPayloads().front();
		m_QMLBackend.lifeviewImageChanged(image);
		m_pRTPDatagramHandler->getPayloads().pop();
		}
}

// -----------------------------------------------------------------------
// Inherited from View
void CMainController::stateEntered(EState stateId) {
	qDebug() << "State changed: " << (int)stateId;
	switch (stateId) {
		case FocusRequest :
                enqueueLifeViewCommand(false /* stop */);
				m_OlyCameraCommands.push(EOCSetShutterMode);
				m_OlyCameraCommands.push(EOC1stPush);
                //m_OlyCameraCommands.push(EOCGetRecView); // Leads to Internal Error 1005
                processCameraCommand();
				break;
		case Focussed :
				// Intentionally left blank
				break;
		case FocusRelease :
				m_OlyCameraCommands.push(EOC1stRelease);
				m_OlyCameraCommands.push(EOCSetRecMode);
				processCameraCommand();
				break;
		case TriggerRequest :
				m_OlyCameraCommands.push(EOC2ndPush);
				processCameraCommand();
				break;
		case Triggered :
				// Intentionally left blank
				break;
		case TriggerRelease :
				m_OlyCameraCommands.push(EOC2ndRelease);
				processCameraCommand();
				break;
		default:
				break;
		}
	this->getQMLBackend()->cameraStatusChanged(stateId);
}

// -----------------------------------------------------------------------
CMainController::CMainController()
	: QObject(), m_pNetworkObserver(nullptr), m_upWifiStatus(nullptr), m_upLocalIpAddress(nullptr),
	  m_pNetworkAccessManager(nullptr), m_pUDPServerSocket(nullptr), m_pNetworkReply(nullptr),
    m_pRTPDatagramHandler(nullptr), m_CameraMode(ECM_Undefined), m_ExposureMode(EEM_Undefined),
    m_HasShutterSpeedValue(false), m_LifeViewEnabled(false), m_LifeViewPotentiallyStarted(false) {
	CNetworkObserver * pObserver = new CNetworkObserver(this);
	m_pNetworkObserver           = pObserver;
	m_pNetworkObserver->setAutoDelete(true);
}

// -----------------------------------------------------------------------
CMainController::~CMainController() {
	if (m_pNetworkObserver != nullptr && !m_pNetworkObserver->autoDelete()) {
		delete m_pNetworkObserver;
		}
}

// -----------------------------------------------------------------------
void CMainController::updateWifiStatus() {
	EWifiStatus status = (EWifiStatus)m_upWifiStatus->getValue();
	switch (status) {
		case EWifiOlyCameraConnected :
					qDebug("Olympus Camera Wifi found");
                    requestCommandList();
					break;
		default:	qDebug("No Wifi found");
					m_StateMachine.init();
					break;
		}
	m_QMLBackend.wifiStatusChanged(QVariant((int)status));
	emit notifyWifiStatusChanged();
}

// -----------------------------------------------------------------------
// Enqueues LifeView command
void CMainController::enqueueLifeViewCommand(bool start) {
    if (start && m_LifeViewEnabled) {
        m_LifeViewPotentiallyStarted = true;
        m_OlyCameraCommands.push(EOCStartLiveView);
        }
    else if (start && !m_LifeViewEnabled) {
        m_LifeViewPotentiallyStarted = true;
        }
    else { /* start == false */
        m_LifeViewPotentiallyStarted = false;
        m_OlyCameraCommands.push(EOCStopLiveView);
        }
}

// -----------------------------------------------------------------------
// Processes a camera command
void CMainController::processCameraCommand() {
	if (m_pNetworkAccessManager == nullptr || m_pNetworkReply != nullptr)
		return;

	QString      url;
	EOlyCommands cmd = (m_OlyCameraCommands.empty()) ? EOCNoCommand : m_OlyCameraCommands.front();
	switch(cmd) {
		case EOCSetRecMode :
					url = QString::fromLatin1("http://%1/switch_cammode.cgi?mode=rec&lvqty=0320x0240").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOCSetShutterMode :
					url = QString::fromLatin1("http://%1/switch_cammode.cgi?mode=shutter").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOC1stPush :
					url = QString::fromLatin1("http://%1/exec_shutter.cgi?com=1stpush").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOC1stRelease :
					url = QString::fromLatin1("http://%1/exec_shutter.cgi?com=1strelease").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOC2ndPush :
					url = QString::fromLatin1("http://%1/exec_shutter.cgi?com=2ndpush").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOC2ndRelease :
					url = QString::fromLatin1("http://%1/exec_shutter.cgi?com=2ndrelease").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOC1st2ndPush :
					url = QString::fromLatin1("http://%1/exec_shutter.cgi?com=1st2ndpush").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOC2nd1stRelease :
					url = QString::fromLatin1("http://%1/exec_shutter.cgi?com=2nd1strelease").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOCRequestShutterSpeed :
                    url = QString::fromLatin1("http://%1/get_camprop.cgi?com=desc&propname=shutspeedvalue ").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOCRequestFocalValue :
					url = QString::fromLatin1("http://%1/get_camprop.cgi?com=desc&propname=focalvalue").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOCRequestEVValue :
					url = QString::fromLatin1("http://%1/get_camprop.cgi?com=desc&propname=expcomp").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
        case EOCRequestCameraDriveMode :
                    url = QString::fromLatin1("http://%1/get_camprop.cgi?com=desc&propname=cameradrivemode").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
        case EOCRequestISOValue :
                    url = QString::fromLatin1("http://%1/get_camprop.cgi?com=desc&propname=isospeedvalue").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
                    break;
        case EOCStartLiveView :
					if (m_pUDPServerSocket != nullptr && m_pUDPServerSocket->localPort() >= 1024)
						url = QString::fromLatin1("http://%1/exec_takemisc.cgi?com=startliveview&port=%2").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY))
																									  .arg(m_pUDPServerSocket->localPort());
					break;
		case EOCStopLiveView :
					url = QString::fromLatin1("http://%1/exec_takemisc.cgi?com=stopliveview").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOCGetLastImage :
					url = QString::fromLatin1("http://%1/exec_takemisc.cgi?com=getlastjpg").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
		case EOCGetRecView :
					url = QString::fromLatin1("http://%1/exec_takemisc.cgi?com=getrecview").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
					break;
        case EOCRequestCommandList :
                    url = QString::fromLatin1("http://%1/get_commandlist.cgi").arg(QString::fromStdString(OLY_DEFAULT_GATEWAY));
                    break;
        default :	break;
		} // End switch

	if (!url.isEmpty()) {
		QNetworkRequest request(url);
		request.setHeader(QNetworkRequest::UserAgentHeader, "OlympusCameraKit");
		m_pNetworkReply = m_pNetworkAccessManager->get(request);
		connect(m_pNetworkReply, SIGNAL(finished()), this, SLOT(httpFinished()));
		connect(((QIODevice*)m_pNetworkReply), SIGNAL(readyRead()), this, SLOT(httpReadyRead()));
		} // End if url is not empty
	else {
		; // Intentionally left blank
		} // End if url is empty
}

// -----------------------------------------------------------------------
// Invokes "2ndpush" at the camera to take the photo
void CMainController::shutterButtonPressed() {
	qDebug("CMainController::shutterButtonPressed");
	m_StateMachine.shutterButtonPressed();
}

// -----------------------------------------------------------------------
// Invokes "2ndrelease" at the camera to
void CMainController::shutterButtonReleased() {
	qDebug("CMainController::shutterButtonReleased");
	m_StateMachine.shutterButtonReleased();
}

// -----------------------------------------------------------------------
// checked = true : invokes "1stpush" at the camera to focus the object
// checked = false: invokes "1strelease" at the camera
void CMainController::focusButtonClicked(bool checked) {
	if (checked)	m_StateMachine.focusButtonPressed();
	else			m_StateMachine.focusButtonReleased();
}

// -----------------------------------------------------------------------
// Requests LifeView image, invoked by NetworkObserver
void CMainController::requestExposureProperties() {
	if (m_StateMachine.isRecModeAvail() && m_upWifiStatus->getValue() == EWifiOlyCameraConnected)
		// Sends signal to GUI thread. Further processing at CMainController::_requestExposureProperties()
		emit dispatchExposurePropertiesRequest();
}

// -----------------------------------------------------------------------
// Requests LifeView image, invoked by NetworkObserver
void CMainController::requestLifeViewImage() {
	// Sends signal to GUI thread. Further processing at CMainController::_requestLifeViewImage()
	emit dispatchLifeViewImageRequest();
}

// -----------------------------------------------------------------------
// Requests command list of camera
void CMainController::requestCommandList() {
    // Sends signal to GUI thread. Further processing at CMainController::_requestCommandList()
    emit dispatchCommandListRequest();
}

// -----------------------------------------------------------------------
// Analyses the reply of an exposure parameter request
std::string CMainController::analyseReply(EOlyCommands cmd, const std::string & reply) {
    int idx1 = reply.find("<value>");
    int idx2 = reply.find("</value>");
    std::string value;
    if (idx1 < idx2 && idx1 != std::string::npos && idx2 != std::string::npos) {
        value = reply.substr(idx1+7, (idx2-idx1-7)); // 7: "<value>"
        return value;
        }
    else
        return "";
}

// -----------------------------------------------------------------------
// Analyses the reply of an exposure parameter request
void CMainController::analyseEmitReply(EOlyCommands cmd, const std::string & reply) {
    const std::string value = analyseReply(cmd, reply);
    if (!value.empty()) {
		emit notifyCameraValueChanged(QVariant((int)cmd), QVariant(QString::fromStdString(value)));
		}
}

// -----------------------------------------------------------------------
// Analyses the reply of an command list request
void CMainController::analyseCommandList(const std::string & reply) {
    // Currently only the "shutspeedvalue" is evaluated
    if (reply.find("shutspeedvalue") != std::string::npos)
        m_HasShutterSpeedValue = true;
}


// -----------------------------------------------------------------------
void CMainController::setLifeViewEnabled(bool enabled) {
    m_LifeViewEnabled = enabled;
    if (!enabled && m_LifeViewPotentiallyStarted) // Disables running LifeView
        m_OlyCameraCommands.push(EOCStopLiveView);
    else if (enabled && m_LifeViewPotentiallyStarted) // Enables potentially running LifeView
        m_OlyCameraCommands.push(EOCStartLiveView);
}

// -----------------------------------------------------------------------
// Qt slot: requests LifeView image
void CMainController::_requestExposureProperties() {
	if (m_CameraMode != ECM_RecMode) {
		m_OlyCameraCommands.push(EOCSetRecMode);
		m_OlyCameraCommands.push(EOCStopLiveView); // ... from previous session possibly different port
        enqueueLifeViewCommand(true /* start */);
		}
    if (m_HasShutterSpeedValue)
        m_OlyCameraCommands.push(EOCRequestShutterSpeed);
	m_OlyCameraCommands.push(EOCRequestFocalValue);
	m_OlyCameraCommands.push(EOCRequestEVValue);
	m_OlyCameraCommands.push(EOCRequestISOValue);
    m_OlyCameraCommands.push(EOCRequestCameraDriveMode);
    processCameraCommand();
}

// -----------------------------------------------------------------------
// Qt slot: requests LifeView image
void CMainController::_requestCommandList() {
    m_OlyCameraCommands.push(EOCRequestCommandList);
    processCameraCommand();
}

// -----------------------------------------------------------------------
// Qt slot: requests LifeView image
void CMainController::_requestLifeViewImage() {
	; // ToDo:
}

// -----------------------------------------------------------------------
// Qt slot: Wifi status has changed
void CMainController::_notifyWifiStatusChanged() {
	EWifiStatus status = (EWifiStatus)m_upWifiStatus->getValue();
	switch (status) {
		case EWifiConnected :
		case EWifiOlyCameraConnected : {
					if (m_pUDPServerSocket == nullptr) {
						m_pUDPServerSocket = new QUdpSocket(this);
						QHostAddress addr(QString::fromStdString(m_upLocalIpAddress->getValue()));
						//QHostAddress addr("192.168.178.22");
						bool success = m_pUDPServerSocket->bind(addr, 0, QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
						quint16 port = m_pUDPServerSocket->localPort();
						QAbstractSocket::SocketState state = m_pUDPServerSocket->state();
						connect(m_pUDPServerSocket, &QUdpSocket::readyRead, this, &CMainController::udpReadyRead);
						}
					} break;
		default:	if (m_pUDPServerSocket != nullptr) {
						disconnect(m_pUDPServerSocket, &QUdpSocket::readyRead, this, &CMainController::udpReadyRead);
						delete m_pUDPServerSocket;
						m_pUDPServerSocket = nullptr;
						}
					break;
		}
}

// -----------------------------------------------------------------------
// Qt slot
void CMainController::httpFinished() {
	if (m_pNetworkReply == nullptr) return;

	QNetworkReply::NetworkError error = m_pNetworkReply->error();
	if (error != QNetworkReply::NoError) {
		wDebug(QString("NetworkReply error: %1").arg((int)error));
		m_pNetworkReply->deleteLater();
		m_pNetworkReply = nullptr;

		while (!m_OlyCameraCommands.empty()) // Empties the queue
			m_OlyCameraCommands.pop();

		m_StateMachine.error();
		return;
		}

	QByteArray buffer = m_pNetworkReply->readAll();
	std::string reply = buffer.toStdString();
	if (!reply.empty()) wDebug(QString::fromStdString(reply));
	m_pNetworkReply->deleteLater();
	m_pNetworkReply = nullptr;

	const EOlyCommands & cmd = m_OlyCameraCommands.front();
	switch (cmd) {
		case EOCSetRecMode:
					m_CameraMode = ECM_RecMode;
					break;
		case EOCSetShutterMode:
					m_CameraMode = ECM_ShutterMode;
					break;
		case EOCRequestShutterSpeed:
		case EOCRequestFocalValue:
		case EOCRequestEVValue:
		case EOCRequestISOValue:
                    analyseEmitReply(cmd, reply);
                    break;
        case EOCRequestCameraDriveMode: {
                    // Possible values: "normal", "continuous", "selftimer", "customselftimer",
                    // "livetime" (composite mode)
                    const std::string s = analyseReply(cmd, reply);
                    if      (s.find("normal") == 0)  m_ExposureMode = EEM_Normal;
                    else if (s.find("cont")   == 0)  m_ExposureMode = EEM_Continuous;
                    else if (s.find("self")   == 0)  m_ExposureMode = EEM_Self;
                    else if (s.find("livetime") == 0)  m_ExposureMode = EEM_Composite;
                    else                             m_ExposureMode = EEM_Undefined;
                    emit notifyCameraValueChanged(QVariant((int)cmd), QVariant((int)m_ExposureMode));
                    } break;
        case EOCGetRecView :
                    break;
        case EOCStoreImage :
                    m_pRTPDatagramHandler->processDatagram(buffer);
                    break;
        case EOCRequestCommandList :
                    analyseCommandList(reply);
                    break;
		default:	break;
		}

	m_StateMachine.commandsProcessed(m_OlyCameraCommands.front());
	m_OlyCameraCommands.pop();

	if (!m_OlyCameraCommands.empty()) {
		processCameraCommand();
		}
}

// -----------------------------------------------------------------------
// Qt slot
void CMainController::httpReadyRead() {
	// This slot gets called every time the QNetworkReply has new data in its buffer.
	// Here is the possibility to read these data
}

// -----------------------------------------------------------------------
// Qt slot
void CMainController::udpReadyRead() {
	// This slot gets called every time a datagram is in the buffer of QUdpSocket.
	// Here is the possibility to read these data
	while (m_pUDPServerSocket->hasPendingDatagrams()) {
		QNetworkDatagram datagram = m_pUDPServerSocket->receiveDatagram();
		m_pRTPDatagramHandler->processDatagram(datagram.data());
	}
}


// -----------------------------------------------------------------------
// Class CQMLBackend
// -----------------------------------------------------------------------
CQMLBackend::CQMLBackend(QObject * pParent) : QObject(pParent), m_pOwner(nullptr),
	m_pRootWidget(nullptr) {
	// Intentionally left blank
}

// -----------------------------------------------------------------------
CQMLBackend::~CQMLBackend() {
	// Intentionally left blank
}

// -----------------------------------------------------------------------
void CQMLBackend::init(CMainController * pOwner, QWidget * pRootWidget) {
	m_pRootWidget  = pRootWidget;
	m_pOwner       = pOwner;
	MainWindow * pMainWindow = dynamic_cast<MainWindow*>(m_pRootWidget);
	if (pMainWindow == nullptr)
		return;

	connect(this, SIGNAL(notifyWifiStatusChanged(QVariant)), pMainWindow, SLOT(notifyWifiStatusChanged(QVariant)), Qt::QueuedConnection);
	connect(this, SIGNAL(notifyCameraStatusChanged(QVariant)), pMainWindow, SLOT(notifyCameraStatusChanged(QVariant)), Qt::QueuedConnection);
	connect(this, SIGNAL(notifyLifeViewImageChanged(QVariant)), pMainWindow, SLOT(notifyLifeViewImageChanged(QVariant)), Qt::QueuedConnection);
	connect(pMainWindow->getFocusButton(), SIGNAL(clicked(bool)), this, SLOT(onFocusButtonClicked(bool)));
	connect(pMainWindow->getShutterButton(), SIGNAL(pressed()), this, SLOT(onShutterButtonPressed()));
	connect(pMainWindow->getShutterButton(), SIGNAL(released()), this, SLOT(onShutterButtonReleased()));
}

// -----------------------------------------------------------------------
void CQMLBackend::tearDown() {
	// It seems there is no need to disconnect notifyWifiStatusChanged() and notifyLifeViewImageChanged
}

// -----------------------------------------------------------------------
void CQMLBackend::wifiStatusChanged(const QVariant & status) {
	emit notifyWifiStatusChanged(status);
}

// -----------------------------------------------------------------------
void CQMLBackend::lifeviewImageChanged(const QVariant & image) {
	emit notifyLifeViewImageChanged(image);
}

// -----------------------------------------------------------------------
void CQMLBackend::cameraStatusChanged(const QVariant & status) {
	emit notifyCameraStatusChanged(status);
}

// -----------------------------------------------------------------------
void CQMLBackend::onFocusButtonClicked(bool checked) {
	m_pOwner->focusButtonClicked(checked);
}

// -----------------------------------------------------------------------
void CQMLBackend::onShutterButtonPressed() {
	m_pOwner->shutterButtonPressed();
}

// -----------------------------------------------------------------------
void CQMLBackend::onShutterButtonReleased() {
	m_pOwner->shutterButtonReleased();
}


// -----------------------------------------------------------------------
// Class CMainStateMachine
// -----------------------------------------------------------------------
CMainStateMachine::CMainStateMachine() : m_pCurrentState(nullptr) {}

// -----------------------------------------------------------------------
void CMainStateMachine::setCurrentState(CMainStateMachine::IState * pState) {
	m_pCurrentState = pState;
}

// -----------------------------------------------------------------------
void CMainStateMachine::init() {
	m_pCurrentState = CInitState::getInstance();
	CInitState::getInstance()->init(this);
}

// -----------------------------------------------------------------------
void CMainStateMachine::tearDown() { m_StateListeners.clear(); m_pCurrentState->tearDown(this); }

// -----------------------------------------------------------------------
void CMainStateMachine::error() { m_pCurrentState->error(this); }

// -----------------------------------------------------------------------
void CMainStateMachine::timeout() {	m_pCurrentState->tearDown(this); }

// -----------------------------------------------------------------------
void CMainStateMachine::focusButtonPressed() { m_pCurrentState->pressFocusButton(this); }

// -----------------------------------------------------------------------
void CMainStateMachine::focusButtonReleased() { m_pCurrentState->releaseFocusButton(this); }

// -----------------------------------------------------------------------
void CMainStateMachine::shutterButtonPressed() { m_pCurrentState->pressShutterButton(this); }

// -----------------------------------------------------------------------
void CMainStateMachine::shutterButtonReleased() { m_pCurrentState->releaseShutterButton(this); }

// -----------------------------------------------------------------------
void CMainStateMachine::commandsProcessed(EOlyCommands cmd) { m_pCurrentState->commandsProcessed(this, cmd); }

// -----------------------------------------------------------------------
void CMainStateMachine::notifyListeners() {
	for (auto pListener : m_StateListeners)
		pListener->stateEntered(m_pCurrentState->getId());
}
// -----------------------------------------------------------------------
void CMainStateMachine::addListener(IStateListener * pListener) { m_StateListeners.insert(pListener); }
// -----------------------------------------------------------------------
void CMainStateMachine::removeListener(IStateListener * pListener) {
	auto pos = m_StateListeners.find(pListener);
	if (pos != m_StateListeners.end())
		m_StateListeners.erase(pos);
}
// -----------------------------------------------------------------------
bool CMainStateMachine::isRecModeAvail() const {
	return m_pCurrentState == nullptr ||
		   m_pCurrentState == CInitState::getInstance() ||
		   m_pCurrentState == CFocusReleaseState::getInstance();
}


// -----------------------------------------------------------------------
// Class CRTPDatagramHandler
// -----------------------------------------------------------------------
void CRTPDatagramHandler::processDatagram(const QByteArray & data) {
	const u_int8_t *   pData = (u_int8_t*)data.data();
	int                size0 = data.size();
	bool   headerInitialized = false;
	u_int16_t sequenceNumber = 0, m_CSRC_Count = 0;
	u_int8_t         version = 0, payloadType = 0, CSRC_Count = 0;
	bool             padding = false, extension = false, marker = false;

	// Decodes an RTP packet and extracts marker, sequence number, and (partial) payload
	// The payload is part of several RTP packets.
	// Based on: https://en.wikipedia.org/wiki/Real-time_Transport_Protocol
	if (size0 > 0) {
		version		= *(pData + 0) >> 6;
		padding		= (*(pData + 0) & 0x20) >> 5;
		extension	= (*(pData + 0) & 0x10) >> 4;
		CSRC_Count  = (*(pData + 0) & 0x0F);
		}
	if (size0 > 1) {
		marker		= (*(pData + 1) & 0x80) >> 7;
		payloadType = (*(pData + 1) & 0x7F); // @see: https://en.wikipedia.org/wiki/RTP_payload_formats
		}
	if (size0 > 3) {
		sequenceNumber    = (*(pData + 2) << 8) + *(pData + 3);
		headerInitialized = true;
		}

	if (headerInitialized) {
		u_int32_t payloadStart = 12 + 4*CSRC_Count; // Calculated without extension header
		if (extension) {
			u_int16_t  extHeaderID = (*(pData + payloadStart + 0) << 8) + *(pData + payloadStart + 1);
			u_int16_t extHeaderLen = (*(pData + payloadStart + 2) << 8) + *(pData + payloadStart + 3);
			payloadStart += (4 * extHeaderLen + 4 /* fields of header ID and length */);
			}

		u_int16_t amountPadding = (padding) ? data.at(data.size()-1) : 0;
		u_int16_t   payloadSize = data.size() - payloadStart - amountPadding;
		m_PartialPayload.append((const char*)(pData + payloadStart), payloadSize);
		}

	if (headerInitialized && marker) {
		if (m_PayloadNumber > 0 && m_Payloads.size() < 20) { // The first partialPayload may be a fractional payload. Limits the queue.
			m_Payloads.push(m_PartialPayload);
			mvc::Model::setChanged();
			mvc::Model::notifyAll();
			}
		m_PartialPayload.clear();
		m_PayloadNumber++;
		}
}


namespace {
// -----------------------------------------------------------------------
// Anonymous class CNetworkObserver
// -----------------------------------------------------------------------
void CNetworkObserver::run() {
	const unsigned short  TIMEOUT_INTERVAL =   50; // ms
	const unsigned short     WIFI_INTERVAL = 2000; // ms
	const unsigned short PROPERTY_INTERVAL =  500; // ms
	setAutoDelete(true);
	unsigned short wifiCounter = 0;
	while(!m_StopRequest) {
		QThread::msleep(TIMEOUT_INTERVAL);

		if ((wifiCounter % (WIFI_INTERVAL/TIMEOUT_INTERVAL)) == 0) {
			QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
			bool                   anyWifiFound = false;
			bool                   olyWifiFound = false;
			for (auto &interface : interfaces) {
				if (interface.isValid() && (interface.flags() & QNetworkInterface::IsUp) > 0 &&
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
				((interface.type() == QNetworkInterface::Wifi) ||
				 (interface.type() == QNetworkInterface::Unknown && interface.name().indexOf("wlan") >= 0))
#else
				 (interface.name().indexOf("wlan") >= 0)
#endif
					) {
					anyWifiFound       = true;
                    std::string gateway= de::bswalz::network::CNetworkHelper::getDefaultGateway(); // Does not work with 2 different network cards [2024-09-09]
					if (gateway == OLY_DEFAULT_GATEWAY) {
						const auto addresses  = interface.addressEntries();
						for (auto &localIpAddress : addresses) {
							QHostAddress    hostaddr = localIpAddress.ip();
							const std::string ipaddr = hostaddr.toString().toStdString();
							if (ipaddr.find('.') != std::string::npos) // Checks for IPv4 address
								m_pOwner->getLocalIpAddress()->assignValue(ipaddr);
							}
						olyWifiFound = true;
						break;
						}
					} // End if interface ist up and valid
				} // End for all interfaces
			if (anyWifiFound && !olyWifiFound) {
				m_pOwner->getLocalIpAddress()->assignValue(LOCAL_HOST);
				m_pOwner->getWifiStatus()->assignValue(EWifiConnected);
				}
			else if (olyWifiFound) {
				m_pOwner->getWifiStatus()->assignValue(EWifiOlyCameraConnected);
				}
			else {
				m_pOwner->getLocalIpAddress()->assignValue(LOCAL_HOST);
				m_pOwner->getWifiStatus()->assignValue(EWifiNotConnected);
				}
			} // End if (WIFI_INTERVAL/TIMEOUT_INTERVAL)) == 0

		if ((wifiCounter % (PROPERTY_INTERVAL/TIMEOUT_INTERVAL)) == 0 &&
			m_pOwner->getWifiStatus()->getValue() == EWifiOlyCameraConnected) {
			m_pOwner->requestExposureProperties();
			} // End if (PROPERTY_INTERVAL/TIMEOUT_INTERVAL)) == 0

		wifiCounter++;
	} // End while
}

// -----------------------------------------------------------------------
// Anonymous class CInitState
// -----------------------------------------------------------------------
std::unique_ptr<CInitState> CInitState::m_upInstance = std::unique_ptr<CInitState>();
// -----------------------------------------------------------------------
CInitState * CInitState::getInstance() {
	if (m_upInstance.get() == nullptr)
		m_upInstance.reset(new CInitState());
	return m_upInstance.get();
}
// -----------------------------------------------------------------------
void CInitState::pressFocusButton(CMainStateMachine * pStateMachine) {
	pStateMachine->setCurrentState(CFocusRequestState::getInstance());
	pStateMachine->notifyListeners();
}
// -----------------------------------------------------------------------
void CInitState::commandsProcessed(CMainStateMachine *, EOlyCommands) {
	// ToDo:
}
// -----------------------------------------------------------------------
void CInitState::init(CMainStateMachine * pStateMachine) {
	pStateMachine->notifyListeners();
}
// -----------------------------------------------------------------------
void CInitState::tearDown(CMainStateMachine *) {}

// -----------------------------------------------------------------------
// Anonymous class CFocusRequestState
// -----------------------------------------------------------------------
std::unique_ptr<CFocusRequestState> CFocusRequestState::m_upInstance = std::unique_ptr<CFocusRequestState>();
// -----------------------------------------------------------------------
CFocusRequestState * CFocusRequestState::getInstance() {
	if (m_upInstance.get() == nullptr)
		m_upInstance.reset(new CFocusRequestState());
	return m_upInstance.get();
}
// -----------------------------------------------------------------------
void CFocusRequestState::commandsProcessed(CMainStateMachine * pStateMachine, EOlyCommands cmd) {
	if (cmd == EOC1stPush) {
		pStateMachine->setCurrentState(CFocussedState::getInstance());
		pStateMachine->notifyListeners();
		}
}
// -----------------------------------------------------------------------
void CFocusRequestState::timeout(CMainStateMachine *) {
	// ToDo:
}
// -----------------------------------------------------------------------
void CFocusRequestState::tearDown(CMainStateMachine *) {
	// ToDo:
}

// -----------------------------------------------------------------------
// Anonymous class CFocussedState
// -----------------------------------------------------------------------
std::unique_ptr<CFocussedState> CFocussedState::m_upInstance = std::unique_ptr<CFocussedState>();
// -----------------------------------------------------------------------
CFocussedState * CFocussedState::getInstance() {
	if (m_upInstance.get() == nullptr)
		m_upInstance.reset(new CFocussedState());
	return m_upInstance.get();
}
// -----------------------------------------------------------------------
void CFocussedState::releaseFocusButton(CMainStateMachine * pStateMachine) {
	pStateMachine->setCurrentState(CFocusReleaseState::getInstance());
	pStateMachine->notifyListeners();
}
// -----------------------------------------------------------------------
void CFocussedState::pressShutterButton(CMainStateMachine * pStateMachine) {
	pStateMachine->setCurrentState(CTriggerRequestState::getInstance());
	pStateMachine->notifyListeners();
}
// -----------------------------------------------------------------------
void CFocussedState::tearDown(CMainStateMachine *) {
	// ToDo:
}

// -----------------------------------------------------------------------
// Anonymous class CFocusReleaseState
// -----------------------------------------------------------------------
std::unique_ptr<CFocusReleaseState> CFocusReleaseState::m_upInstance = std::unique_ptr<CFocusReleaseState>();
// -----------------------------------------------------------------------
CFocusReleaseState * CFocusReleaseState::getInstance() {
	if (m_upInstance.get() == nullptr)
		m_upInstance.reset(new CFocusReleaseState());
	return m_upInstance.get();
}
// -----------------------------------------------------------------------
void CFocusReleaseState::commandsProcessed(CMainStateMachine * pStateMachine, EOlyCommands cmd) {
	if (cmd == EOC1stRelease) {
		pStateMachine->setCurrentState(CInitState::getInstance());
		pStateMachine->notifyListeners();
		}
}
// -----------------------------------------------------------------------
void CFocusReleaseState::timeout(CMainStateMachine *) {
	// ToDo:
}
// -----------------------------------------------------------------------
void CFocusReleaseState::tearDown(CMainStateMachine *) {
	// ToDo:
}

// -----------------------------------------------------------------------
// Anonymous class CTriggerRequestState
// -----------------------------------------------------------------------
std::unique_ptr<CTriggerRequestState> CTriggerRequestState::m_upInstance = std::unique_ptr<CTriggerRequestState>();
// -----------------------------------------------------------------------
CTriggerRequestState * CTriggerRequestState::getInstance() {
	if (m_upInstance.get() == nullptr)
		m_upInstance.reset(new CTriggerRequestState());
	return m_upInstance.get();
}
// -----------------------------------------------------------------------
void CTriggerRequestState::commandsProcessed(CMainStateMachine * pStateMachine, EOlyCommands cmd) {
	if (cmd == EOC2ndPush) {
		pStateMachine->setCurrentState(CTriggeredState::getInstance());
		pStateMachine->notifyListeners();
		}
}
// -----------------------------------------------------------------------
// Possibly happens when camera ist slowly responding
void CTriggerRequestState::releaseShutterButton(CMainStateMachine * pStateMachine) {
	pStateMachine->setCurrentState(CTriggerReleaseState::getInstance());
	pStateMachine->notifyListeners();
}
// -----------------------------------------------------------------------
void CTriggerRequestState::timeout(CMainStateMachine *) {
	// ToDo:
}
// -----------------------------------------------------------------------
void CTriggerRequestState::tearDown(CMainStateMachine *) {
	// ToDo:
}

// -----------------------------------------------------------------------
// Anonymous class CTriggeredState
// -----------------------------------------------------------------------
std::unique_ptr<CTriggeredState> CTriggeredState::m_upInstance = std::unique_ptr<CTriggeredState>();
// -----------------------------------------------------------------------
CTriggeredState * CTriggeredState::getInstance() {
	if (m_upInstance.get() == nullptr)
		m_upInstance.reset(new CTriggeredState());
	return m_upInstance.get();
}
// -----------------------------------------------------------------------
void CTriggeredState::releaseShutterButton(CMainStateMachine * pStateMachine) {
	pStateMachine->setCurrentState(CTriggerReleaseState::getInstance());
	pStateMachine->notifyListeners();
}
// -----------------------------------------------------------------------
void CTriggeredState::tearDown(CMainStateMachine *) {
	// ToDo:
}

// -----------------------------------------------------------------------
// Anonymous class CTriggerReleaseState
// -----------------------------------------------------------------------
std::unique_ptr<CTriggerReleaseState> CTriggerReleaseState::m_upInstance = std::unique_ptr<CTriggerReleaseState>();
// -----------------------------------------------------------------------
CTriggerReleaseState * CTriggerReleaseState::getInstance() {
	if (m_upInstance.get() == nullptr)
		m_upInstance.reset(new CTriggerReleaseState());
	return m_upInstance.get();
}
// -----------------------------------------------------------------------
void CTriggerReleaseState::commandsProcessed(CMainStateMachine * pStateMachine, EOlyCommands cmd) {
	if (cmd == EOC2ndRelease) {
		pStateMachine->setCurrentState(CFocussedState::getInstance());
		pStateMachine->notifyListeners();
		}
}
// -----------------------------------------------------------------------
void CTriggerReleaseState::timeout(CMainStateMachine *) {
	// ToDo:
}
// -----------------------------------------------------------------------
void CTriggerReleaseState::tearDown(CMainStateMachine *) {
	// ToDo:
}

} // End of anonymous namespace

}}} // End namespaces
