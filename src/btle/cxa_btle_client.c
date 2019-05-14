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
#include "cxa_btle_client.h"


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
void cxa_btle_client_init(cxa_btle_client_t *const btlecIn,
						  cxa_btle_client_scm_getState_t scm_getStateIn,
						  cxa_btle_client_scm_startScan_t scm_startScanIn,
						  cxa_btle_client_scm_stopScan_t scm_stopScanIn,
						  cxa_btle_client_scm_startConnection_t scm_startConnectionIn,
						  cxa_btle_client_scm_stopConnection_t scm_stopConnectionIn,
						  cxa_btle_client_scm_readFromCharacteristic_t scm_readFromCharIn,
						  cxa_btle_client_scm_writeToCharacteristic_t scm_writeToCharIn,
						  cxa_btle_client_scm_changeNotifications_t scm_changeNotificationsIn)
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
	btlecIn->scms.stopConnection = scm_stopConnectionIn;
	btlecIn->scms.readFromCharacteristic = scm_readFromCharIn;
	btlecIn->scms.writeToCharacteristic = scm_writeToCharIn;
	btlecIn->scms.changeNotifications = scm_changeNotificationsIn;
	btlecIn->hasActivityAvailable = false;

	// clear our callbacks
	memset((void*)&btlecIn->cbs, 0, sizeof(btlecIn->cbs));
	cxa_array_initStd(&btlecIn->listeners, btlecIn->listeners_raw);
	cxa_array_initStd(&btlecIn->notiIndiSubs, btlecIn->notiIndiSubs_raw);
}


void cxa_btle_client_addListener(cxa_btle_client_t *const btlecIn,
								 cxa_btle_client_cb_onReady_t cb_onReadyIn,
								 cxa_btle_client_cb_onFailedInit_t cb_onFailedInitIn,
								 void* userVarIn)
{
	cxa_assert(btlecIn);

	cxa_btle_client_listener_entry_t newEntry =
	{
			.cb_onReady = cb_onReadyIn,
			.cb_onFailedInit = cb_onFailedInitIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&btlecIn->listeners, &newEntry));
}


void cxa_btle_client_startScan_passive(cxa_btle_client_t *const btlecIn,
									   cxa_btle_client_cb_onScanStart_t cb_scanStartIn,
									   cxa_btle_client_cb_onAdvertRx_t cb_advIn,
									   void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.scanning.onAdvert = cb_advIn;
	btlecIn->cbs.scanning.onScanStart = cb_scanStartIn;
	btlecIn->cbs.scanning.userVar = userVarIn;

	// start our scan
	cxa_assert(btlecIn->scms.startScan);
	btlecIn->scms.startScan(btlecIn, false);
}


void cxa_btle_client_startScan_active(cxa_btle_client_t *const btlecIn,
									  cxa_btle_client_cb_onScanStart_t cb_scanStartIn,
									  cxa_btle_client_cb_onAdvertRx_t cb_advIn,
									  cxa_btle_client_cb_onScanResponseRx_t cb_scanRespIn,
									  void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.scanning.onAdvert = cb_advIn;
	btlecIn->cbs.scanning.onScanResp = cb_scanRespIn;
	btlecIn->cbs.scanning.onScanStart = cb_scanStartIn;
	btlecIn->cbs.scanning.userVar = userVarIn;

	// start our scan
	cxa_assert(btlecIn->scms.startScan);
	btlecIn->scms.startScan(btlecIn, false);
}


void cxa_btle_client_stopScan(cxa_btle_client_t *const btlecIn,
							  cxa_btle_client_cb_onScanStop_t cb_scanStopIn,
							  void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.scanning.onScanStop = cb_scanStopIn;
	btlecIn->cbs.scanning.userVar = userVarIn;

	// stop our scan
	cxa_assert(btlecIn->scms.stopScan);
	btlecIn->scms.stopScan(btlecIn);
}


cxa_btle_client_state_t cxa_btle_client_getState(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	return (btlecIn->scms.getState != NULL) ? btlecIn->scms.getState(btlecIn) : true;
}


bool cxa_btle_client_hasActivityAvailable(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	bool retVal = btlecIn->hasActivityAvailable;
	btlecIn->hasActivityAvailable = false;
	return retVal;
}


void cxa_btle_client_startConnection(cxa_btle_client_t *const btlecIn, cxa_eui48_t *const addrIn, bool isRandomAddrIn,
									 cxa_btle_client_cb_onConnectionOpened_t cb_connectionOpenedIn,
									 cxa_btle_client_cb_onConnectionClosed_unexpected_t cb_connectionClosed_unexpectedIn,
									 void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.connecting.onConnectionOpened = cb_connectionOpenedIn;
	btlecIn->cbs.connecting.onConnectionClosed_unexpected = cb_connectionClosed_unexpectedIn;
	btlecIn->cbs.connecting.userVar = userVarIn;

	cxa_assert(btlecIn->scms.startConnection != NULL);
	btlecIn->scms.startConnection(btlecIn, addrIn, isRandomAddrIn);
}


void cxa_btle_client_stopConnection(cxa_btle_client_t *const btlecIn,
									cxa_eui48_t *const targetAddrIn,
									cxa_btle_client_cb_onConnectionClosed_expected_t cb_connectionClosed_expectedIn,
									void *userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.connecting.onConnectionClosed_expected = cb_connectionClosed_expectedIn;
	btlecIn->cbs.connecting.userVar = userVarIn;

	cxa_assert(btlecIn->scms.stopConnection != NULL);
	btlecIn->scms.stopConnection(btlecIn, targetAddrIn);
}


void cxa_btle_client_readFromCharacteristic(cxa_btle_client_t *const btlecIn,
										    cxa_eui48_t *const targetAddrIn,
										    const char *const serviceUuidIn,
										    const char *const characteristicUuidIn,
										    cxa_btle_client_cb_onReadComplete_t cb_readCompleteIn,
										    void* userVarIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// save our callbacks
	btlecIn->cbs.reading.onReadComplete = cb_readCompleteIn;
	btlecIn->cbs.reading.userVar = userVarIn;

	cxa_assert(btlecIn->scms.readFromCharacteristic != NULL);


	btlecIn->scms.readFromCharacteristic(btlecIn, targetAddrIn, serviceUuidIn, characteristicUuidIn);
}


void cxa_btle_client_writeToCharacteristic_fbb(cxa_btle_client_t *const btlecIn,
											   cxa_eui48_t *const targetAddrIn,
											   const char *const serviceUuidIn,
											   const char *const characteristicUuidIn,
											   cxa_fixedByteBuffer_t *const dataIn,
											   cxa_btle_client_cb_onWriteComplete_t cb_writeCompleteIn,
											   void* userVarIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// save our callbacks
	btlecIn->cbs.writing.onWriteComplete = cb_writeCompleteIn;
	btlecIn->cbs.writing.userVar = userVarIn;

	cxa_assert(btlecIn->scms.writeToCharacteristic != NULL);
	btlecIn->scms.writeToCharacteristic(btlecIn, targetAddrIn, serviceUuidIn, characteristicUuidIn, dataIn);
}


void cxa_btle_client_writeToCharacteristic(cxa_btle_client_t *const btlecIn,
										   cxa_eui48_t *const targetAddrIn,
										   const char *const serviceUuidIn,
										   const char *const characteristicUuidIn,
										   void *const dataIn,
										   size_t numBytesIn,
										   cxa_btle_client_cb_onWriteComplete_t cb_writeCompleteIn,
										   void* userVarIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// save our callbacks
	btlecIn->cbs.writing.onWriteComplete = cb_writeCompleteIn;
	btlecIn->cbs.writing.userVar = userVarIn;

	cxa_assert(btlecIn->scms.writeToCharacteristic != NULL);

	cxa_fixedByteBuffer_t fbb_data;
	cxa_fixedByteBuffer_init_inPlace(&fbb_data, numBytesIn, dataIn, numBytesIn);

	btlecIn->scms.writeToCharacteristic(btlecIn, targetAddrIn, serviceUuidIn, characteristicUuidIn, &fbb_data);
}


void cxa_btle_client_subscribeToNotifications(cxa_btle_client_t *const btlecIn,
	    									  cxa_eui48_t *const targetAddrIn,
											  const char *const serviceUuidIn,
											  const char *const characteristicUuidIn,
											  cxa_btle_client_cb_onNotiIndiSubscriptionChanged_t cb_onSubscribedIn,
											  cxa_btle_client_cb_onNotiIndiRx_t cb_onRxIn,
											  void* userVarIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure we have room to store the subscription
	cxa_btle_client_notiIndiSubscription_t* newSub = cxa_array_append_empty(&btlecIn->notiIndiSubs);
	if( newSub == NULL )
	{
		if( cb_onSubscribedIn != NULL ) cb_onSubscribedIn(targetAddrIn, serviceUuidIn, characteristicUuidIn, false, userVarIn);
		return;
	}

	// record our info
	newSub->cb_onSubscriptionChanged = cb_onSubscribedIn;
	newSub->cb_onRx = cb_onRxIn;
	newSub->userVar = userVarIn;
	cxa_eui48_initFromEui48(&newSub->address, targetAddrIn);
	if( !cxa_btle_uuid_initFromString(&newSub->uuid_service, serviceUuidIn) )
	{
		cxa_array_remove(&btlecIn->notiIndiSubs, newSub);
		if( cb_onSubscribedIn != NULL ) cb_onSubscribedIn(targetAddrIn, serviceUuidIn, characteristicUuidIn, false, userVarIn);
		return;
	}
	if( !cxa_btle_uuid_initFromString(&newSub->uuid_characteristic, characteristicUuidIn) )
	{
		cxa_array_remove(&btlecIn->notiIndiSubs, newSub);
		if( cb_onSubscribedIn != NULL ) cb_onSubscribedIn(targetAddrIn, serviceUuidIn, characteristicUuidIn, false, userVarIn);
		return;
	}

	cxa_assert(btlecIn->scms.changeNotifications != NULL);
	btlecIn->scms.changeNotifications(btlecIn, targetAddrIn, serviceUuidIn, characteristicUuidIn, true);
}


void cxa_btle_client_unsubscribeToNotifications(cxa_btle_client_t *const btlecIn,
												cxa_eui48_t *const targetAddrIn,
												const char *const serviceUuidIn,
												const char *const characteristicUuidIn,
												cxa_btle_client_cb_onNotiIndiSubscriptionChanged_t cb_onUnsubscribedIn,
												void* userVarIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// save our callback
	btlecIn->cbs.unsubscribing.onUnsubscribed = cb_onUnsubscribedIn;
	btlecIn->cbs.unsubscribing.userVar = userVarIn;

	// make sure to remove our notification entry (if we have one)
	cxa_array_iterate(&btlecIn->notiIndiSubs, currSub, cxa_btle_client_notiIndiSubscription_t)
	{
		if( currSub == NULL ) continue;

		if( cxa_eui48_isEqual(&currSub->address, targetAddrIn) &&
			cxa_btle_uuid_isEqualToString(&currSub->uuid_service, serviceUuidIn) &&
			cxa_btle_uuid_isEqualToString(&currSub->uuid_characteristic, characteristicUuidIn) )
		{
			cxa_array_remove(&btlecIn->notiIndiSubs, currSub);
		}
	}

	// actually perform the change on the device
	btlecIn->scms.changeNotifications(btlecIn, targetAddrIn, serviceUuidIn, characteristicUuidIn, false);
}


void cxa_btle_client_notify_onBecomesReady(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	cxa_array_iterate(&btlecIn->listeners, currListener, cxa_btle_client_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onReady != NULL ) currListener->cb_onReady(btlecIn, currListener->userVar);
	}
}


void cxa_btle_client_notify_onFailedInit(cxa_btle_client_t *const btlecIn, bool willAutoRetryIn)
{
	cxa_assert(btlecIn);

	cxa_array_iterate(&btlecIn->listeners, currListener, cxa_btle_client_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onFailedInit != NULL ) currListener->cb_onFailedInit(btlecIn, willAutoRetryIn, currListener->userVar);
	}
}


void cxa_btle_client_notify_advertRx(cxa_btle_client_t *const btlecIn, cxa_btle_advPacket_t *packetIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.scanning.onAdvert != NULL ) btlecIn->cbs.scanning.onAdvert(packetIn, btlecIn->cbs.scanning.userVar);
}


void cxa_btle_client_notify_scanStart(cxa_btle_client_t *const btlecIn, bool wasSuccessfulIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.scanning.onScanStart != NULL ) btlecIn->cbs.scanning.onScanStart(wasSuccessfulIn, btlecIn->cbs.scanning.userVar);
}


void cxa_btle_client_notify_scanStop(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.scanning.onScanStop != NULL ) btlecIn->cbs.scanning.onScanStop(btlecIn->cbs.scanning.userVar);
}


void cxa_btle_client_notify_connectionStarted(cxa_btle_client_t *const btlecIn, cxa_eui48_t *const targetAddrIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.connecting.onConnectionOpened != NULL ) btlecIn->cbs.connecting.onConnectionOpened(targetAddrIn, btlecIn->cbs.connecting.userVar);
}


void cxa_btle_client_notify_connectionClose_expected(cxa_btle_client_t *const btlecIn, cxa_eui48_t *const targetAddrIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.connecting.onConnectionClosed_expected != NULL ) btlecIn->cbs.connecting.onConnectionClosed_expected(targetAddrIn, btlecIn->cbs.connecting.userVar);
}


void cxa_btle_client_notify_connectionClose_unexpected(cxa_btle_client_t *const btlecIn, cxa_eui48_t *const targetAddrIn, cxa_btle_client_disconnectReason_t reasonIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.connecting.onConnectionClosed_unexpected != NULL ) btlecIn->cbs.connecting.onConnectionClosed_unexpected(targetAddrIn, reasonIn, btlecIn->cbs.connecting.userVar);
}


void cxa_btle_client_notify_writeComplete(cxa_btle_client_t *const btlecIn,
										  cxa_eui48_t *const targetAddrIn,
										  const char *const serviceUuidIn,
										  const char *const characteristicUuidIn,
										  bool wasSuccessfulIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	if( btlecIn->cbs.writing.onWriteComplete != NULL )
	{
		btlecIn->cbs.writing.onWriteComplete(targetAddrIn, serviceUuidIn, characteristicUuidIn, wasSuccessfulIn, btlecIn->cbs.writing.userVar);
	}
}


void cxa_btle_client_notify_readComplete(cxa_btle_client_t *const btlecIn,
										 cxa_eui48_t *const targetAddrIn,
										 const char *const serviceUuidIn,
										 const char *const characteristicUuidIn,
										 bool wasSuccessfulIn,
										 cxa_fixedByteBuffer_t *fbb_readDataIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	if( btlecIn->cbs.reading.onReadComplete != NULL )
	{
		btlecIn->cbs.reading.onReadComplete(targetAddrIn, serviceUuidIn, characteristicUuidIn, wasSuccessfulIn, fbb_readDataIn, btlecIn->cbs.reading.userVar);
	}
}


void cxa_btle_client_notify_notiIndiSubscriptionChanged(cxa_btle_client_t *const btlecIn,
														cxa_eui48_t *const targetAddrIn,
														const char *const serviceUuidIn,
														const char *const characteristicUuidIn,
														bool wasSuccessfulIn,
														bool notificationsEnabledIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	if( notificationsEnabledIn )
	{
		cxa_array_iterate(&btlecIn->notiIndiSubs, currSub, cxa_btle_client_notiIndiSubscription_t)
		{
			if( currSub == NULL ) continue;

			if( cxa_eui48_isEqual(&currSub->address, targetAddrIn) &&
				cxa_btle_uuid_isEqualToString(&currSub->uuid_service, serviceUuidIn) &&
				cxa_btle_uuid_isEqualToString(&currSub->uuid_characteristic, characteristicUuidIn) )
			{
				if( currSub->cb_onSubscriptionChanged != NULL ) currSub->cb_onSubscriptionChanged(targetAddrIn, serviceUuidIn, characteristicUuidIn, wasSuccessfulIn, currSub->userVar);
			}
		}
	}
	else
	{
		if( btlecIn->cbs.unsubscribing.onUnsubscribed != NULL ) btlecIn->cbs.unsubscribing.onUnsubscribed(targetAddrIn, serviceUuidIn, characteristicUuidIn, wasSuccessfulIn, btlecIn->cbs.unsubscribing.userVar);
	}
}


void cxa_btle_client_notify_notiIndiRx(cxa_btle_client_t *const btlecIn,
									   cxa_eui48_t *const targetAddrIn,
									   const char *const serviceUuidIn,
									   const char *const characteristicUuidIn,
									   cxa_fixedByteBuffer_t *fbb_dataIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	cxa_array_iterate(&btlecIn->notiIndiSubs, currSub, cxa_btle_client_notiIndiSubscription_t)
	{
		if( currSub == NULL ) continue;

		if( cxa_eui48_isEqual(&currSub->address, targetAddrIn) &&
			cxa_btle_uuid_isEqualToString(&currSub->uuid_service, serviceUuidIn) &&
			cxa_btle_uuid_isEqualToString(&currSub->uuid_characteristic, characteristicUuidIn) )
		{
			if( currSub->cb_onRx != NULL ) currSub->cb_onRx(targetAddrIn, serviceUuidIn, characteristicUuidIn, fbb_dataIn, currSub->userVar);
		}
	}
}


// ******** local function implementations ********

