# OlyCameraRC (Application)

The application allows a restricted remote control of Olympus (TM) cameras using the wireless connection.
Unlike the official app this app allows to control focussing with an extra button for the purpose of taking multiple photos with the same focus. Further the LifeView can be activated/deactivated seperately to save energy.

Additionally this application serves as an example of the software design pattern "State" originally published by Erich Gamma.

<strong>Important Note</strong>:
OlyCameraRC is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE; without even any warranty of MISBEHAVIOUR resp. DAMAGE cameras due to faults, misuse of camera API.


## Screenshots of the product
![Screenshot_20240914_051313](https://github.com/user-attachments/assets/c297c1d3-83b2-4fe5-bcfd-40ffd18e0049)



## The software pattern "State"
State transitions according to the design pattern are exemplarily shown for the focus request of the camera.
Complete code for all state transitions is available at main.h and main.cpp.

#### main.h
The main class and interface of the state machine (shortened):
<pre>
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
		...
		virtual void commandsProcessed(CMainStateMachine *, EOlyCommands) {}
		...
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
	...
	void setCurrentState(CMainStateMachine::IState *);
	virtual ~CMainStateMachine() {}
private:
	IState *						m_pCurrentState;
	std::set<IStateListener*>		m_StateListeners;
};
</pre>

#### main.cpp:
The invocation of focusButtonPressed() resp. focusButtonReleased() or commandsProcessed():
<pre>
void CMainStateMachine::focusButtonPressed() { m_pCurrentState->pressFocusButton(this); }
void CMainStateMachine::focusButtonReleased() { m_pCurrentState->releaseFocusButton(this); }
void CMainStateMachine::commandsProcessed(EOlyCommands cmd) { m_pCurrentState->commandsProcessed(this, cmd); }
</pre>

The state transition initated by pressing the "Focus" button is implemented in the anonymous classes CInitState.
The button's event changes the current state to "FocusRequest" state.
<pre>
void CInitState::pressFocusButton(CMainStateMachine * pStateMachine) {
	pStateMachine->setCurrentState(CFocusRequestState::getInstance());
	pStateMachine->notifyListeners();
}
</pre>
The state listener (see: notfyListeners()) sends a "1st push" command via HTTP protocol to the camera, then waiting for the response of the camera. When the camera has responded, commandsProcessed() is invoked.
<pre>
void CFocusRequestState::commandsProcessed(CMainStateMachine * pStateMachine, EOlyCommands cmd) {
	if (cmd == EOC1stPush) {
		pStateMachine->setCurrentState(CFocussedState::getInstance());
		pStateMachine->notifyListeners();
		}
}
</pre>
The state machine has now changed to "Focussed" state.


## Building the product
Checkout of the sources
<pre>
git clone https://github.com/siwad/OlyCameraRC
cd OlyCameraRC
git clone https://github.com/siwad/common
</pre>

I built and tested the product with the following toolchain resp. kits:
* QtCreator 14.0.1
* CMake 3.20.4
* Desktop_Qt-5.15.2_GCC_64_bit (running on [openSuSE](https://www.opensuse.org) Linux)
* Android_Qt-5.15.2_Clang_Multi_Abi (running on Android:tm: emulation of [Sailfish OS](https://sailfishos.org/)), API Level 21
* gcc 12.2.1
