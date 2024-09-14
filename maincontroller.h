#ifndef DE_BSWALZ_OLYCAMERARC_MAINCONTROLLER_H
#define DE_BSWALZ_OLYCAMERARC_MAINCONTROLLER_H

/**
 * OlympusCamera-RemoteControl: main controller interfaces
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
#include <common/mvc/View.h>		// Separate git-repo
#include <common/model/Parameter.h>	// Separate git-repo

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

// -----------------------------------------------------------------------
// Class IMainController (Interface of a Singleton)
// -----------------------------------------------------------------------
class IMainController  {
public:
    IMainController() {}
    virtual ~IMainController() {}

    /** Access to Wifi status */
    virtual CEnumParameter * getWifiStatus() const = 0;
	/** Access to local IP address */
    virtual CAStringParameter * getLocalIpAddress() const = 0;
    /** Access to property "exposure mode" */
    virtual EExposeMode getExposureMode() const = 0;
    /** Access to property "shutter speed" */
    virtual bool hasShutterSpeedValue() const = 0;
    /** Access to property "life view enabled" */
    virtual void setLifeViewEnabled(bool) = 0;
};


}}} // End namespaces

#endif // DE_BSWALZ_OLYCAMERARC_MAIN_H
