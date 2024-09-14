#ifndef DE_BSWALZ_OLYCAMERARC_MAIN_H
#define DE_BSWALZ_OLYCAMERARC_MAIN_H

/**
 * OlympusCamera-RemoteControl: main classes
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
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OlyCamera-RC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OlyCamera-RC. If not, see <http://www.gnu.org/licenses/>.
 *
 * OlyCamera-RC was tested under Android 4.1/5.0 (SDK 16/21) and Linux and
 * was developped using Qt 5.12.12 resp. Qt 5.15.2 .
 */

#include "types.h"
#include "maincontroller.h"
#include <QObject>
#include <QVariant>
#include <memory>
#include <queue>
#include <set>
#include <common/mvc/View.h>		// Separate git-repo
#include <common/model/Parameter.h>	// Separate git-repo

class QRunnable;
class QCoreApplication;
class QNetworkAccessManager;
class QNetworkReply;
class QUdpSocket;
class QNetworkDatagram;

namespace de { namespace bswalz {
namespace mvc {
class Model;
}
namespace model {
class CEnumParameter;
}

namespace olycamerarc {

using de::bswalz::mvc::View;
using de::bswalz::mvc::Model;
using namespace de::bswalz::model;

class CMainController;
class CRTPDatagramHandler;

// -----------------------------------------------------------------------
// Class CQMLBackend
// -----------------------------------------------------------------------
class CQMLBackend : public QObject {
	Q_OBJECT

public:
	CQMLBackend(QObject * pParent = nullptr);
	virtual ~CQMLBackend();
	void init(CMainController *, QWidget*);
	void tearDown();

	void wifiStatusChanged(const QVariant &);
	void cameraStatusChanged(const QVariant &);
	void lifeviewImageChanged(const QVariant &);

signals:
	void notifyWifiStatusChanged(QVariant);
	void notifyCameraStatusChanged(QVariant);
	void notifyLifeViewImageChanged(QVariant);

protected slots:
	void onShutterButtonPressed();
	void onShutterButtonReleased();
	void onFocusButtonClicked(bool);

private:
	QWidget *			m_pRootWidget;
	CMainController *	m_pOwner;
};

// -----------------------------------------------------------------------
// Class CMainStateMachine
// -----------------------------------------------------------------------
class CMainStateMachine {
	friend class CMainController;
public:
	class IState {
	public:
		virtual ~IState() {}
		virtual void pressFocusButton(CMainStateMachine *) {}
		virtual void releaseFocusButton(CMainStateMachine *) {}
		virtual void pressShutterButton(CMainStateMachine *) {}
		virtual void releaseShutterButton(CMainStateMachine *) {}
		virtual void commandsProcessed(CMainStateMachine *, EOlyCommands) {}
		virtual void timeout(CMainStateMachine *) {}
		virtual void error(CMainStateMachine *) {}
		virtual void tearDown(CMainStateMachine *) = 0;
		virtual EState getId() const = 0;
	};
	class IStateListener {
	public:
		virtual ~IStateListener() {}
		virtual void stateEntered(EState) = 0;
	};

public:
	void focusButtonPressed();
	void focusButtonReleased();
	void shutterButtonPressed();
	void shutterButtonReleased();
	void commandsProcessed(EOlyCommands);
	void error();
	void timeout();
	void notifyListeners();
	void addListener(IStateListener *);
	void removeListener(IStateListener *);
	void setCurrentState(CMainStateMachine::IState *);
	bool isRecModeAvail() const;
	virtual ~CMainStateMachine() {}
protected:
	void init();
	void tearDown();
	CMainStateMachine();
private:
	IState *						m_pCurrentState;
	std::set<IStateListener*>		m_StateListeners;
};

// -----------------------------------------------------------------------
// Class CMainController (Singleton)
// -----------------------------------------------------------------------
class CMainController : public QObject, public View, public IMainController, public CMainStateMachine::IStateListener {
	Q_OBJECT
public:
	static	CMainController * getInstance();
	virtual ~CMainController();
	void	init(QCoreApplication *, QWidget *);

	/** Inherited from View */
	virtual void update(const Model * pModel, void * pObject) override;

	/** Inherited from CMainStateMachine::IStateListener */
	virtual void stateEntered(EState) override;

	/** Access to QML backend */
	CQMLBackend * getQMLBackend() { return &m_QMLBackend; }

	/** Callback from QML backend */
	void	shutterButtonPressed();
	/** Callback from QML backend */
	void	shutterButtonReleased();
	/** Callback from QML backend */
	void	focusButtonClicked(bool);

	/** Access to Wifi status */
    virtual CEnumParameter * getWifiStatus() const override { return m_upWifiStatus.get(); }
	/** Access to local IP address */
    virtual CAStringParameter * getLocalIpAddress() const override { return m_upLocalIpAddress.get(); }
    /** Access to property "exposure mode" */
    virtual EExposeMode getExposureMode() const override { return m_ExposureMode; }
    /** Access to property "shutter speed" */
    virtual bool hasShutterSpeedValue() const override { return m_HasShutterSpeedValue; }
    /** Access to property "life view enabled" */
    virtual void setLifeViewEnabled(bool enabled) override;

    /** Requests command list of camera */
    void    requestCommandList();
	/** Requests exposure properties */
	void	requestExposureProperties();
	/** Requests LV image */
	void	requestLifeViewImage();
    /** Potentially starts / stops LifeView */
    void    enqueueLifeViewCommand(bool start);
protected:
	CMainController();
	void	updateWifiStatus();
	void	processCameraCommand();
    void    analyseCommandList(const std::string &);
    void    analyseEmitReply(EOlyCommands, const std::string &);
    std::string analyseReply(EOlyCommands, const std::string &);

protected slots:
	void	tearDown();
	void	httpFinished();
	void	httpReadyRead();
	void	udpReadyRead();
	void	_requestExposureProperties();
	void    _requestLifeViewImage();
    void    _requestCommandList();
	void    _notifyWifiStatusChanged();

signals:
	void    dispatchExposurePropertiesRequest();
	void    dispatchLifeViewImageRequest();
    void    dispatchCommandListRequest();
	void    notifyWifiStatusChanged();
	void	notifyCameraValueChanged(QVariant,QVariant);

private:
	static	std::unique_ptr<CMainController> m_upInstance;
	QRunnable *			m_pNetworkObserver;
	QUdpSocket *		m_pUDPServerSocket;
	QNetworkAccessManager * m_pNetworkAccessManager;
	QNetworkReply *		m_pNetworkReply;
	CRTPDatagramHandler* m_pRTPDatagramHandler;
	CQMLBackend			m_QMLBackend;
	CMainStateMachine	m_StateMachine;
	ECameraMode			m_CameraMode;
    EExposeMode         m_ExposureMode;
    bool                m_HasShutterSpeedValue;
    bool                m_LifeViewEnabled;
    bool                m_LifeViewPotentiallyStarted;
	std::queue<EOlyCommands> m_OlyCameraCommands;

	std::unique_ptr<CEnumParameter> m_upWifiStatus;
	std::unique_ptr<CAStringParameter> m_upLocalIpAddress;
};


}}} // End namespaces

#endif // DE_BSWALZ_OLYCAMERARC_MAIN_H
