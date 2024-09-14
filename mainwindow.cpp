
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

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "maincontroller.h"
#include "types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>

const QSize WIFI_ICON_SIZE = QSize(20,20);
const QSize WIFI_LED_SIZE  = QSize(20,20);
const QSize SHUTTER_BUTTON_SIZE = QSize(100,100);

namespace {
static const QString DFLT_SHUTTERSPEED_TEXT = "T --- s";
static const QString DFLT_FOCALVALUE_TEXT   = "F 1/--";
static const QString DFLT_EV_TEXT           = "EV ---";
static const QString DFLT_ISO_TEXT          = "ISO Auto";
static const QString DFLT_EXP_MODE_TEXT     = "M ---";
}

// -----------------------------------------------------------------------
// Class MainWindow
// -----------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_pMainController(nullptr) {
	ui->setupUi(this);
}

// -----------------------------------------------------------------------
MainWindow::~MainWindow() {
	delete ui;
}

// -----------------------------------------------------------------------
void MainWindow::layoutUI() {
	const QRect & geom = this->geometry();

	// this->ui->main_column_layout; ist the main QVBoxLayout.
	QHBoxLayout * pHeadlineLayout = new QHBoxLayout();
	QVBoxLayout *    pLabelLayout = new QVBoxLayout();
	QLabel *		  pTitleLabel = new QLabel("OlyCamera RemoteControl");
	QDate                   date = QDate(QLocale("en_US").toDate(QString(__DATE__).simplified(), "MMM d yyyy"));
#ifdef QT_DEBUG
	QString            authorText = QString("Siegfried Walz, Version: %1").arg(date.toString("yyyy-MM-dd"));
#else
	QString            authorText = QString("Version: %1").arg(date.toString("yyyy-MM-dd"));
#endif
	QLabel *		 pAuthorLabel = new QLabel(authorText);
	pTitleLabel->setFont(QFont(this->font().family(), 12, 2));
	pAuthorLabel->setFont(QFont(this->font().family(), 8, 1));
	pTitleLabel->setMinimumHeight(23);
	pAuthorLabel->setMinimumHeight(12);
	pLabelLayout->setSpacing(2);
	pLabelLayout->addWidget(pTitleLabel);
	pLabelLayout->addWidget(pAuthorLabel);
	pHeadlineLayout->setSpacing(2);
	pHeadlineLayout->addItem(pLabelLayout);

	m_pWifiLED = new QLabel();
	m_pOlyWifiLED = new QLabel();
	m_pWifiLED->setPixmap(QPixmap(":/res/wifi-disabled.png").scaled(WIFI_ICON_SIZE));
	m_pWifiLED->setMaximumSize(35,35);
	m_pOlyWifiLED->setPixmap(QPixmap(":/res/led-gr.png").scaled(WIFI_LED_SIZE));
	m_pOlyWifiLED->setMaximumSize(35,35);
	pHeadlineLayout->addWidget(m_pWifiLED);
	pHeadlineLayout->addWidget(m_pOlyWifiLED);

	ui->main_column_layout->addItem(pHeadlineLayout);

	m_pLifeView = new QLabel();
	m_pLifeView->setMinimumWidth(360);
	m_pLifeView->setMinimumHeight(240);
	m_pLifeView->setAlignment(Qt::AlignHCenter);
	m_pLifeView->setPixmap(QPixmap(":/res/lifeview-disabled.png").scaled(m_pLifeView->size(), Qt::KeepAspectRatio));

    m_pLifeViewButton = new QPushButton();
    m_pLifeViewButton->setIcon(QIcon(":/res/play_button_released.png"));
    m_pLifeViewButton->setFlat(true);
    m_pLifeViewButton->setCheckable(true);
    m_pLifeViewButton->setAutoRepeat(false);
    m_pLifeViewButton->setFixedSize(60,60);
    m_pLifeViewButton->setIconSize(QSize(60,60));

	m_pFocusButton = new QPushButton();
	m_pFocusButton->setIcon(QIcon(":/res/focus_button_released.png"));
	m_pFocusButton->setFlat(true);
	m_pFocusButton->setCheckable(true);
	m_pFocusButton->setAutoRepeat(false);
	m_pFocusButton->setFixedSize(100,100);
	m_pFocusButton->setIconSize(QSize(100,100));

	m_pShutterButton = new QPushButton();
	m_pShutterButton->setIcon(QIcon(":/res/shutter_button_released.png"));
	m_pShutterButton->setFlat(true);
	m_pShutterButton->setCheckable(false);
	m_pShutterButton->setAutoRepeat(false);
	m_pShutterButton->setFixedSize(100,100);
	m_pShutterButton->setIconSize(QSize(100,100));

	ui->main_column_layout->addWidget(m_pLifeView);
	ui->main_column_layout->addStretch(2);

	QFrame * pPropertiesFrame = new QFrame();
	pPropertiesFrame->setFrameShape(QFrame::StyledPanel);
	QGridLayout *  pPropertiesLayout = new QGridLayout();
	pPropertiesFrame->setLayout(pPropertiesLayout);
	m_pShutterSpeedLabel = new QLabel(DFLT_SHUTTERSPEED_TEXT);
	m_pFocalValueLabel   = new QLabel(DFLT_FOCALVALUE_TEXT);
	m_pEVLabel           = new QLabel(DFLT_EV_TEXT);
	m_pISOLabel          = new QLabel(DFLT_ISO_TEXT);
    m_pExpModeLabel      = new QLabel(DFLT_EXP_MODE_TEXT);
	pPropertiesLayout->addWidget(m_pShutterSpeedLabel, 0, 0);
	pPropertiesLayout->addWidget(m_pFocalValueLabel, 0, 1);
	pPropertiesLayout->addWidget(m_pEVLabel, 0, 2);
	pPropertiesLayout->addWidget(m_pISOLabel, 0, 3);
    pPropertiesLayout->addWidget(m_pExpModeLabel, 0, 4);
	ui->main_column_layout->addWidget(pPropertiesFrame);
	ui->main_column_layout->addStretch(2);

	QHBoxLayout * pButtonLayout = new QHBoxLayout();
	pButtonLayout->setAlignment(Qt::AlignHCenter);
    pButtonLayout->addWidget(m_pLifeViewButton);
    pButtonLayout->addSpacing(50);
    pButtonLayout->addWidget(m_pFocusButton);
	pButtonLayout->addSpacing(50);
	pButtonLayout->addWidget(m_pShutterButton);

	ui->main_column_layout->addItem(pButtonLayout);
	ui->main_column_layout->addStretch(10);

	ui->centralwidget->setLayout(ui->main_column_layout);

    connect(this->m_pLifeViewButton, SIGNAL(toggled(bool)), this, SLOT(notifyLifeViewButtonChecked(bool)));
}

// -----------------------------------------------------------------------
void MainWindow::notifyWifiStatusChanged(QVariant arg) {
	unsigned int status = arg.toUInt();
	qDebug("Wifi status has changed: %i", status);
	switch (status) {
		case de::bswalz::olycamerarc::EWifiConnected :
                m_pLifeViewButton->setEnabled(false /*false*/);
                m_pFocusButton->setEnabled(false /*false*/);
				m_pShutterButton->setEnabled(false /*false*/);
				m_pWifiLED->setPixmap(QPixmap(":/res/wifi-enabled.png").scaled(WIFI_ICON_SIZE));
				m_pOlyWifiLED->setPixmap(QPixmap(":/res/led-rt.png").scaled(WIFI_LED_SIZE));
				m_pLifeView->setPixmap(QPixmap(":/res/lifeview-disabled.png").scaled(m_pLifeView->size(), Qt::KeepAspectRatio));
				break;
		case de::bswalz::olycamerarc::EWifiOlyCameraConnected :
                m_pLifeViewButton->setEnabled(true);
                m_pFocusButton->setEnabled(true);
				//m_pShutterButton->setEnabled(true); // Depends on state
				m_pWifiLED->setPixmap(QPixmap(":/res/wifi-enabled.png").scaled(WIFI_ICON_SIZE));
				m_pOlyWifiLED->setPixmap(QPixmap(":/res/led-gn.png").scaled(WIFI_LED_SIZE));
				break;
		default:
                m_pLifeViewButton->setEnabled(false);
                m_pFocusButton->setEnabled(false);
				m_pShutterButton->setEnabled(false);
				m_pWifiLED->setPixmap(QPixmap(":/res/wifi-disabled.png").scaled(WIFI_ICON_SIZE));
				m_pOlyWifiLED->setPixmap(QPixmap(":/res/led-gr.png").scaled(WIFI_LED_SIZE));
				m_pLifeView->setPixmap(QPixmap(":/res/lifeview-disabled.png").scaled(m_pLifeView->size(), Qt::KeepAspectRatio));
				break;
		} // End switch
}

// -----------------------------------------------------------------------
void MainWindow::notifyCameraStatusChanged(QVariant status) {
	switch (status.toUInt()) {
		case de::bswalz::olycamerarc::FocusRequest :
				 m_pFocusButton->setEnabled(false);
				 m_pFocusButton->setIcon(QIcon(":/res/focus_button_released.png"));
				 m_pShutterButton->setEnabled(false);
				 break;
		case de::bswalz::olycamerarc::Focussed :
				 m_pFocusButton->setEnabled(true);
				 m_pFocusButton->setIcon(QIcon(":/res/focus_button_pressed.png"));
				 m_pShutterButton->setEnabled(true);
				 break;
		case de::bswalz::olycamerarc::FocusRelease :
				 m_pFocusButton->setEnabled(false);
				 m_pFocusButton->setIcon(QIcon(":/res/focus_button_pressed.png"));
                 m_pShutterButton->setEnabled(false);
				 break;
		case de::bswalz::olycamerarc::TriggerRequest :
				 m_pFocusButton->setEnabled(true /*false*/);
				 m_pFocusButton->setIcon(QIcon(":/res/focus_button_pressed.png"));
                 m_pShutterButton->setEnabled((m_pMainController->getExposureMode() == de::bswalz::olycamerarc::EEM_Composite) ? true : false);
                 m_pLifeViewButton->setChecked(false);
				 break;
		case de::bswalz::olycamerarc::Triggered :
				 m_pFocusButton->setEnabled(true /*false*/);
				 m_pFocusButton->setIcon(QIcon(":/res/focus_button_pressed.png"));
				 m_pShutterButton->setEnabled(true);
				 break;
		case de::bswalz::olycamerarc::TriggerRelease :
				 m_pFocusButton->setEnabled(true);
				 m_pFocusButton->setIcon(QIcon(":/res/focus_button_pressed.png"));
				 m_pShutterButton->setEnabled(true);
				 break;
		default: m_pFocusButton->setEnabled(true);
				 m_pFocusButton->setIcon(QIcon(":/res/focus_button_released.png"));
				 m_pShutterButton->setEnabled(false);
				 break;
	}
}

// -----------------------------------------------------------------------
void MainWindow::notifyLifeViewImageChanged(QVariant variant) {
	qDebug("LifeView image changed");
	QByteArray data = variant.toByteArray();
	QImage    image = QImage::fromData(data);
	QPixmap  pixmap;
	pixmap.convertFromImage(image);
	m_pLifeView->setPixmap(pixmap);
}

// -----------------------------------------------------------------------
void MainWindow::notifyCameraValueChanged(QVariant key, QVariant value) {
	switch (key.toInt()) {
		case de::bswalz::olycamerarc::EOCRequestShutterSpeed:
					m_pShutterSpeedLabel->setText(QString("T %1").arg(value.toString()));
					break;
		case de::bswalz::olycamerarc::EOCRequestFocalValue:
					m_pFocalValueLabel->setText(QString("F 1/%1").arg(value.toString()));
					break;
		case de::bswalz::olycamerarc::EOCRequestEVValue:
					m_pEVLabel->setText(QString("EV %1").arg(value.toString()));
					break;
		case de::bswalz::olycamerarc::EOCRequestISOValue:
					m_pISOLabel->setText(QString("ISO %1").arg(value.toString()));
					break;
        case de::bswalz::olycamerarc::EOCRequestCameraDriveMode: {
                    bool ok = false;
                    de::bswalz::olycamerarc::EExposeMode mode = (de::bswalz::olycamerarc::EExposeMode)value.toInt(&ok);
                    if      (ok && mode == de::bswalz::olycamerarc::EEM_Normal) m_pExpModeLabel->setText("M Norm");
                    else if (ok && mode == de::bswalz::olycamerarc::EEM_Self)   m_pExpModeLabel->setText("M Self");
                    else if (ok && mode == de::bswalz::olycamerarc::EEM_Continuous) m_pExpModeLabel->setText("M Cont");
                    else if (ok && mode == de::bswalz::olycamerarc::EEM_Composite)  m_pExpModeLabel->setText("M Comp");
                    else    m_pExpModeLabel->setText(DFLT_EXP_MODE_TEXT);
                    } break;
		default:	break;
	}
}

// -----------------------------------------------------------------------
void MainWindow::notifyLifeViewButtonChecked(bool checked) {
    if (checked) m_pLifeViewButton->setIcon(QIcon(":/res/play_button_pressed.png"));
    else         m_pLifeViewButton->setIcon(QIcon(":/res/play_button_released.png"));
    m_pMainController->setLifeViewEnabled(checked);
}
