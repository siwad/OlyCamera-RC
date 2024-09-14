#ifndef _DE_BSWALZ_MAINWINDOW_H
#define _DE_BSWALZ_MAINWINDOW_H

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

#include <QMainWindow>
class QLabel;
class QPushButton;

namespace de { namespace bswalz { namespace olycamerarc {
class IMainController;
}}}

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	void layoutUI();
    void setMainController(de::bswalz::olycamerarc::IMainController * pMainController) { m_pMainController = pMainController; }
	QPushButton * getFocusButton()   { return m_pFocusButton; }
	QPushButton * getShutterButton() { return m_pShutterButton; }
    QPushButton * getLifeViewButton() { return m_pLifeViewButton; }

protected:
	QLabel * m_pWifiLED;
	QLabel * m_pOlyWifiLED;
	QLabel * m_pLifeView;
	QPushButton * m_pFocusButton;
	QPushButton * m_pShutterButton;
    QPushButton * m_pLifeViewButton;
	QLabel * m_pShutterSpeedLabel;
	QLabel * m_pFocalValueLabel;
	QLabel * m_pEVLabel;
	QLabel * m_pISOLabel;
    QLabel * m_pExpModeLabel;
    de::bswalz::olycamerarc::IMainController * m_pMainController;

protected slots:
	void notifyWifiStatusChanged(QVariant);
	void notifyCameraStatusChanged(QVariant);
	void notifyCameraValueChanged(QVariant, QVariant);
	void notifyLifeViewImageChanged(QVariant);
    void notifyLifeViewButtonChecked(bool);

private:
	Ui::MainWindow *ui;
};
#endif // _DE_BSWALZ_MAINWINDOW_H
