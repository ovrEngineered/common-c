/**
 * @copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_btle_central.h"


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_btle_central_init(cxa_btle_central_t *const btlecIn,
						   cxa_btle_central_scm_getState_t scm_getStateIn,
						   cxa_btle_central_scm_startScan_t scm_startScanIn,
						   cxa_btle_central_scm_stopScan_t scm_stopScanIn,
						   cxa_btle_central_scm_startConnection_t scm_startConnectionIn)
{
	cxa_assert(btlecIn);
	cxa_assert(scm_getStateIn);
	cxa_assert(scm_startScanIn);
	cxa_assert(scm_stopScanIn);

	// save our references
	btlecIn->scms.getState = scm_getStateIn;
	btlecIn->scms.startScan = scm_startScanIn;
	btlecIn->scms.stopScan = scm_stopScanIn;
	btlecIn->scms.startConnection = scm_startConnectionIn;
	btlecIn->hasActivityAvailable = false;

	// clear our callbacks and listeners
	memset((void*)&btlecIn->cbs, 0, sizeof(btlecIn->cbs));
	cxa_array_initStd(&btlecIn->listeners, btlecIn->listeners_raw);

	// setup our logger
	cxa_logger_init(&btlecIn->logger, "btleCentral");
}


void cxa_btle_central_addListener(cxa_btle_central_t *const btlecIn,
								 cxa_btle_central_cb_onReady_t cb_onReadyIn,
								 cxa_btle_central_cb_onFailedInit_t cb_onFailedInitIn,
								 void* userVarIn)
{
	cxa_assert(btlecIn);

	cxa_btle_central_listener_entry_t newEntry =
	{
			.cb_onReady = cb_onReadyIn,
			.cb_onFailedInit = cb_onFailedInitIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&btlecIn->listeners, &newEntry));
}


void cxa_btle_central_startScan_passive(cxa_btle_central_t *const btlecIn,
									   cxa_btle_central_cb_onScanStart_t cb_scanStartIn,
									   cxa_btle_central_cb_onAdvertRx_t cb_advIn,
									   void* userVarIn)
{
	cxa_assert(btlecIn);

	// make sure this method isn't already in progress
	if( (btlecIn->cbs.scanning.onAdvert != NULL) ||
		(btlecIn->cbs.scanning.onScanStart != NULL) )
	{
		if(cb_scanStartIn != NULL) cb_scanStartIn(false, userVarIn);
		return;
	}

	// save our callbacks
	btlecIn->cbs.scanning.onAdvert = cb_advIn;
	btlecIn->cbs.scanning.onScanStart = cb_scanStartIn;
	btlecIn->cbs.scanning.userVar = userVarIn;

	// start our scan
	cxa_assert(btlecIn->scms.startScan);
	cxa_logger_info(&btlecIn->logger, "starting passive scan");
	btlecIn->scms.startScan(btlecIn, false);
}


void cxa_btle_central_startScan_active(cxa_btle_central_t *const btlecIn,
									  cxa_btle_central_cb_onScanStart_t cb_scanStartIn,
									  cxa_btle_central_cb_onAdvertRx_t cb_advIn,
									  cxa_btle_central_cb_onScanResponseRx_t cb_scanRespIn,
									  void* userVarIn)
{
	cxa_assert(btlecIn);

	// make sure this method isn't already in progress
	if( (btlecIn->cbs.scanning.onAdvert != NULL) ||
		(btlecIn->cbs.scanning.onScanResp != NULL) ||
		(btlecIn->cbs.scanning.onScanStart != NULL) )
	{
		if(cb_scanStartIn != NULL) cb_scanStartIn(false, userVarIn);
		return;
	}

	// save our callbacks
	btlecIn->cbs.scanning.onAdvert = cb_advIn;
	btlecIn->cbs.scanning.onScanResp = cb_scanRespIn;
	btlecIn->cbs.scanning.onScanStart = cb_scanStartIn;
	btlecIn->cbs.scanning.userVar = userVarIn;

	// start our scan
	cxa_assert(btlecIn->scms.startScan);
	cxa_logger_info(&btlecIn->logger, "starting active scan");
	btlecIn->scms.startScan(btlecIn, false);
}


void cxa_btle_central_stopScan(cxa_btle_central_t *const btlecIn,
							  cxa_btle_central_cb_onScanStop_t cb_scanStopIn,
							  void* userVarIn)
{
	cxa_assert(btlecIn);

	// make sure this method isn't already in progress
	if( (btlecIn->cbs.scanning.onScanStop != NULL) )
	{
		return;
	}

	// save our callbacks
	btlecIn->cbs.scanning.onScanStop = cb_scanStopIn;
	btlecIn->cbs.scanning.userVar = userVarIn;

	// stop our scan
	cxa_assert(btlecIn->scms.stopScan);
	cxa_logger_info(&btlecIn->logger, "stopping scan");
	btlecIn->scms.stopScan(btlecIn);
}


cxa_btle_central_state_t cxa_btle_central_getState(cxa_btle_central_t *const btlecIn)
{
	cxa_assert(btlecIn);

	return (btlecIn->scms.getState != NULL) ? btlecIn->scms.getState(btlecIn) : true;
}


bool cxa_btle_central_hasActivityAvailable(cxa_btle_central_t *const btlecIn)
{
	cxa_assert(btlecIn);

	bool retVal = btlecIn->hasActivityAvailable;
	btlecIn->hasActivityAvailable = false;
	return retVal;
}


void cxa_btle_central_startConnection(cxa_btle_central_t *const btlecIn,
									  cxa_eui48_t *const addrIn,
									  bool isRandomAddrIn,
									  cxa_btle_central_cb_onConnectionOpened_t cb_connectionOpenedIn,
									  void* userVarIn)
{
	cxa_assert(btlecIn);
	cxa_assert(addrIn);

	// make sure this method isn't already in progress
	if( btlecIn->cbs.connecting.onConnectionOpened != NULL )
	{
		if( cb_connectionOpenedIn != NULL ) cb_connectionOpenedIn(false, NULL, userVarIn);
		return;
	}

	// save our callbacks
	btlecIn->cbs.connecting.onConnectionOpened = cb_connectionOpenedIn;
	btlecIn->cbs.connecting.userVar = userVarIn;

	// perform the connection
	cxa_assert(btlecIn->scms.startConnection != NULL);
	cxa_eui48_string_t targetAddr_str;
	cxa_eui48_toString(addrIn, &targetAddr_str);
	cxa_logger_info(&btlecIn->logger, "connecting to '%s'", targetAddr_str.str);
	btlecIn->scms.startConnection(btlecIn, addrIn, isRandomAddrIn);
}


void cxa_btle_central_notify_onBecomesReady(cxa_btle_central_t *const btlecIn)
{
	cxa_assert(btlecIn);

	cxa_array_iterate(&btlecIn->listeners, currListener, cxa_btle_central_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onReady != NULL ) currListener->cb_onReady(btlecIn, currListener->userVar);
	}
}


void cxa_btle_central_notify_onFailedInit(cxa_btle_central_t *const btlecIn,
										  bool willAutoRetryIn)
{
	cxa_assert(btlecIn);

	cxa_array_iterate(&btlecIn->listeners, currListener, cxa_btle_central_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onFailedInit != NULL ) currListener->cb_onFailedInit(btlecIn, willAutoRetryIn, currListener->userVar);
	}
}


void cxa_btle_central_notify_advertRx(cxa_btle_central_t *const btlecIn,
									  cxa_btle_advPacket_t *packetIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.scanning.onAdvert != NULL ) btlecIn->cbs.scanning.onAdvert(packetIn, btlecIn->cbs.scanning.userVar);
}


void cxa_btle_central_notify_scanStart(cxa_btle_central_t *const btlecIn,
									   bool wasSuccessfulIn)
{
	cxa_assert(btlecIn);

	// clear the way for other related callbacks
	btlecIn->cbs.scanning.onScanStop = NULL;

	// call our callback
	if( btlecIn->cbs.scanning.onScanStart != NULL )
	{
		cxa_btle_central_cb_onScanStart_t cb = btlecIn->cbs.scanning.onScanStart;
		btlecIn->cbs.scanning.onScanStart = NULL;
		cb(wasSuccessfulIn, btlecIn->cbs.scanning.userVar);
	}
}


void cxa_btle_central_notify_scanStop(cxa_btle_central_t *const btlecIn)
{
	cxa_assert(btlecIn);

	// clear the way for other related callbacks
	btlecIn->cbs.scanning.onAdvert = NULL;
	btlecIn->cbs.scanning.onScanResp = NULL;
	btlecIn->cbs.scanning.onScanStart = NULL;

	// call our callback
	if( btlecIn->cbs.scanning.onScanStop != NULL )
	{
		cxa_btle_central_cb_onScanStop_t cb = btlecIn->cbs.scanning.onScanStop;
		btlecIn->cbs.scanning.onScanStop = NULL;
		cb(btlecIn->cbs.scanning.userVar);
	}
}


void cxa_btle_central_notify_connectionStarted(cxa_btle_central_t *const btlecIn,
											   bool wasSuccessfulIn,
											   cxa_btle_connection_t *const connIn)
{
	cxa_assert(btlecIn);

	// call our callback
	if( btlecIn->cbs.connecting.onConnectionOpened != NULL )
	{
		cxa_btle_central_cb_onConnectionOpened_t cb = btlecIn->cbs.connecting.onConnectionOpened;
		btlecIn->cbs.connecting.onConnectionOpened = NULL;
		cb(wasSuccessfulIn, connIn, btlecIn->cbs.connecting.userVar);
	}
}


// ******** local function implementations ********

