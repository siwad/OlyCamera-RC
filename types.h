#ifndef DE_BSWALZ_OLYCAMERARC_TYPES_H
#define DE_BSWALZ_OLYCAMERARC_TYPES_H

/**
 * OlympusCamera-RemoteControl: data types
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


namespace de { namespace bswalz { namespace olycamerarc {

enum EOlyCommands   { EOCNoCommand, EOCSetRecMode, EOCSetShutterMode, EOC1stPush, EOC1stRelease, EOC2ndPush, EOC2ndRelease, EOC1st2ndPush, EOC2nd1stRelease,
                      EOCRequestShutterSpeed, EOCRequestFocalValue, EOCRequestEVValue, EOCRequestISOValue, EOCRequestCameraDriveMode,
                      EOCStartLiveView, EOCStopLiveView, EOCGetLastImage, EOCGetRecView, EOCStoreImage,
                      EOCRequestCommandList };
enum EState         { Init = 0, FocusRequest = 1, Focussed = 2, FocusRelease = 3, TriggerRequest = 4, Triggered = 5, TriggerRelease = 6 };
enum EWifiStatus    { EWifiNotConnected = 0, EWifiConnected = 1, EWifiOlyCameraConnected = 2 };
enum ECameraMode	{ ECM_Undefined, ECM_RecMode, ECM_ShutterMode };
enum EExposeMode    { EEM_Undefined = 0, EEM_Normal = 1,  EEM_Continuous = 2, EEM_Self = 3, EEM_Composite = 4 };

}}} // End namespaces

#endif // DE_BSWALZ_OLYCAMERARC_TYPES_H
