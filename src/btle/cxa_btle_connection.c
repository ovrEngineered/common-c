/*
 * Copyright 2019 ovrEngineered, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * @author Christopher Armenio
 */
#include "cxa_btle_connection.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_btle_central.h>

#include <string.h>

#define CXA_LOG_LEVEL					CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_btle_connection_init(cxa_btle_connection_t *const connIn,
							  void *const btlecIn,
							  cxa_btle_connection_scm_stopConnection_t scm_stopConnectionIn,
							  cxa_btle_connection_scm_readFromCharacteristic_t scm_readFromCharIn,
							  cxa_btle_connection_scm_writeToCharacteristic_t scm_writeToCharIn,
							  cxa_btle_connection_scm_changeNotifications_t scm_changeNotiIndisIn)
{
	cxa_assert(connIn);
	cxa_assert(btlecIn);

	// save our references
	connIn->btlec = btlecIn;

	// save our subclass methods
	connIn->scms.stopConnection = scm_stopConnectionIn;
	connIn->scms.readFromCharacteristic = scm_readFromCharIn;
	connIn->scms.writeToCharacteristic = scm_writeToCharIn;
	connIn->scms.changeNotifications = scm_changeNotiIndisIn;

	// clear out our callbacks
	memset(&connIn->cbs, 0, sizeof(connIn->cbs));

	// setup our noti/indi subscriptions
	cxa_array_initStd(&connIn->notiIndiSubs, connIn->notiIndiSubs_raw);
}


void cxa_btle_connection_setTargetAddress(cxa_btle_connection_t *const connIn,
										  cxa_eui48_t *const targetAddrIn)
{
	cxa_assert(connIn);
	cxa_assert(targetAddrIn);

	// set our address
	cxa_eui48_initFromEui48(&connIn->targetAddr, targetAddrIn);

	// clear out our callbacks
	memset(&connIn->cbs, 0, sizeof(connIn->cbs));

	// setup our noti/indi subscriptions
	cxa_array_initStd(&connIn->notiIndiSubs, connIn->notiIndiSubs_raw);
}


void cxa_btle_connection_setOnClosedCb(cxa_btle_connection_t *const connIn, cxa_btle_connection_cb_onConnectionClosed_t cbIn, void* userVarIn)
{
	cxa_assert(connIn);

	// save our callback
	connIn->cbs.connectionClosed.func = cbIn;
	connIn->cbs.connectionClosed.userVar = userVarIn;
}


cxa_eui48_t* cxa_btle_connection_getTargetMacAddress(cxa_btle_connection_t *const connIn)
{
	cxa_assert(connIn);

	return &connIn->targetAddr;
}


void cxa_btle_connection_readFromCharacteristic(cxa_btle_connection_t *const connIn,
												const char *const serviceUuidIn,
												const char *const characteristicUuidIn,
												cxa_btle_connection_cb_onReadComplete_t cbIn,
												void* userVarIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure that this method is not already in progress
	if( connIn->cbs.readFromChar.func != NULL )
	{
		if( cbIn != NULL ) cbIn(false, NULL, userVarIn);
		return;
	}

	// save our callback
	connIn->cbs.readFromChar.func = cbIn;
	connIn->cbs.readFromChar.userVar = userVarIn;

	// actually perform the read
	cxa_assert(connIn->scms.readFromCharacteristic);
	connIn->scms.readFromCharacteristic(connIn, serviceUuidIn, characteristicUuidIn);
}


void cxa_btle_connection_writeToCharacteristic(cxa_btle_connection_t *const connIn,
											   const char *const serviceUuidIn,
											   const char *const characteristicUuidIn,
											   cxa_fixedByteBuffer_t *const dataIn,
											   cxa_btle_connection_cb_onWriteComplete_t cbIn,
											   void *userVarIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure that this method is not already in progress
	if( connIn->cbs.writeToChar.func != NULL )
	{
		if( cbIn != NULL ) cbIn(false, userVarIn);
		return;
	}

	// save our callback
	connIn->cbs.writeToChar.func = cbIn;
	connIn->cbs.writeToChar.userVar = userVarIn;

	// actually perform the write
	cxa_assert(connIn->scms.writeToCharacteristic);
	connIn->scms.writeToCharacteristic(connIn, serviceUuidIn, characteristicUuidIn, dataIn);
}


void cxa_btle_connection_writeToCharacteristic_ptr(cxa_btle_connection_t *const connIn,
											   	   const char *const serviceUuidIn,
												   const char *const characteristicUuidIn,
												   void *const dataIn,
												   size_t numBytesIn,
												   cxa_btle_connection_cb_onWriteComplete_t cbIn,
												   void *userVarIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// convert to a fbb
	cxa_fixedByteBuffer_t fbbTmp;
	cxa_fixedByteBuffer_init_inPlace(&fbbTmp, numBytesIn, dataIn, numBytesIn);

	cxa_btle_connection_writeToCharacteristic(connIn, serviceUuidIn, characteristicUuidIn, &fbbTmp, cbIn, userVarIn);
}


void cxa_btle_connection_subscribeToNotifications(cxa_btle_connection_t *const connIn,
												  const char *const serviceUuidIn,
												  const char *const characteristicUuidIn,
												  cxa_btle_connection_cb_onNotiIndiSubscriptionChanged_t cb_onSubscribedIn,
												  cxa_btle_connection_cb_onNotiIndiRx_t cb_onRxIn,
												  void* userVarIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure that this method is not already in progress
	if( connIn->cbs.subscribeToChar.func != NULL )
	{
		if( cb_onSubscribedIn != NULL ) cb_onSubscribedIn(serviceUuidIn, characteristicUuidIn, false, userVarIn);
		return;
	}

	// save our callback
	connIn->cbs.subscribeToChar.func = cb_onSubscribedIn;
	connIn->cbs.subscribeToChar.userVar = userVarIn;

	// make sure the UUIDs checkout
	cxa_btle_uuid_t tmpServiceUuid, tmpCharUuid;
	if( !cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidIn) ||
		!cxa_btle_uuid_initFromString(&tmpCharUuid, characteristicUuidIn) )
	{
		if( cb_onSubscribedIn != NULL ) cb_onSubscribedIn(serviceUuidIn, characteristicUuidIn, false, userVarIn);
		return;
	}

	// don't allow duplicate subscriptions
	cxa_array_iterate(&connIn->notiIndiSubs, currSub, cxa_btle_connection_notiIndiSubscription_t)
	{
		if( currSub == NULL ) continue;

		if( cxa_btle_uuid_isEqual(&currSub->uuid_service, &tmpServiceUuid) &&
			cxa_btle_uuid_isEqual(&currSub->uuid_characteristic, &tmpCharUuid) )
		{
			if( cb_onSubscribedIn != NULL ) cb_onSubscribedIn(serviceUuidIn, characteristicUuidIn, false, userVarIn);
			return;
		}
	}

	// create a new subscription
	cxa_btle_connection_notiIndiSubscription_t* newSub = cxa_array_append_empty(&connIn->notiIndiSubs);
	if( newSub == NULL )
	{
		if( cb_onSubscribedIn != NULL ) cb_onSubscribedIn(serviceUuidIn, characteristicUuidIn, false, userVarIn);
		return;
	}
	cxa_btle_uuid_initFromUuid(&newSub->uuid_service, &tmpServiceUuid, false);
	cxa_btle_uuid_initFromUuid(&newSub->uuid_characteristic, &tmpCharUuid, false);
	newSub->cb_onRx = cb_onRxIn;
	newSub->userVar = userVarIn;

	// now actually do the subscribing
	cxa_assert(connIn->scms.changeNotifications);
	connIn->scms.changeNotifications(connIn, serviceUuidIn, characteristicUuidIn, true);
}


void cxa_btle_connection_unsubscribeToNotifications(cxa_btle_connection_t *const connIn,
													const char *const serviceUuidIn,
													const char *const characteristicUuidIn,
													cxa_btle_connection_cb_onNotiIndiSubscriptionChanged_t cb_onUnsubscribedIn,
													void* userVarIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure that this method is not already in progress
	if( connIn->cbs.unsubscribeFromChar.func != NULL )
	{
		if( cb_onUnsubscribedIn != NULL ) cb_onUnsubscribedIn(serviceUuidIn, characteristicUuidIn, false, userVarIn);
		return;
	}

	// save our callback
	connIn->cbs.unsubscribeFromChar.func = cb_onUnsubscribedIn;
	connIn->cbs.unsubscribeFromChar.userVar = userVarIn;

	// make sure the UUIDs checkout
	cxa_btle_uuid_t tmpServiceUuid, tmpCharUuid;
	if( !cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidIn) ||
		!cxa_btle_uuid_initFromString(&tmpCharUuid, characteristicUuidIn) )
	{
		if( cb_onUnsubscribedIn != NULL ) cb_onUnsubscribedIn(serviceUuidIn, characteristicUuidIn, false, userVarIn);
		return;
	}

	// find our subscription entry
	for( size_t i = 0; i < cxa_array_getSize_elems(&connIn->notiIndiSubs); i++ )
	{
		cxa_btle_connection_notiIndiSubscription_t* currSub = cxa_array_get(&connIn->notiIndiSubs, i);
		if( currSub == NULL ) continue;

		if( cxa_btle_uuid_isEqual(&currSub->uuid_service, &tmpServiceUuid) &&
					cxa_btle_uuid_isEqual(&currSub->uuid_characteristic, &tmpCharUuid) )
		{
			cxa_array_remove_atIndex(&connIn->notiIndiSubs, i);
			i--;
		}
	}

	// now actually do the unsubscribing
	cxa_assert(connIn->scms.changeNotifications);
	connIn->scms.changeNotifications(connIn, serviceUuidIn, characteristicUuidIn, false);
}


void cxa_btle_connection_stop(cxa_btle_connection_t *const connIn)
{
	cxa_assert(connIn);

	// actually do the stopping
	cxa_assert(connIn->scms.stopConnection);
	connIn->scms.stopConnection(connIn);
}


void cxa_btle_connection_notify_connectionClose(cxa_btle_connection_t *const connIn, cxa_btle_connection_disconnectReason_t reasonIn)
{
	cxa_assert(connIn);

	// notify our callback
	if( connIn->cbs.connectionClosed.func != NULL )
	{
		cxa_btle_connection_cb_onConnectionClosed_t cb = connIn->cbs.connectionClosed.func;
		void* userVar = connIn->cbs.connectionClosed.userVar;

		connIn->cbs.connectionClosed.func = NULL;
		connIn->cbs.connectionClosed.userVar = NULL;

		cb(reasonIn, userVar);
	}
}


void cxa_btle_connection_notify_writeComplete(cxa_btle_connection_t *const connIn,
											  const char *const serviceUuidIn,
											  const char *const characteristicUuidIn,
											  bool wasSuccessfulIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// notify our callback
	if( connIn->cbs.writeToChar.func != NULL )
	{
		cxa_btle_connection_cb_onWriteComplete_t cb = connIn->cbs.writeToChar.func;
		void* userVar = connIn->cbs.writeToChar.userVar;

		connIn->cbs.writeToChar.func = NULL;
		connIn->cbs.writeToChar.userVar = NULL;

		cb(wasSuccessfulIn, userVar);
	}
}


void cxa_btle_connection_notify_readComplete(cxa_btle_connection_t *const connIn,
											 const char *const serviceUuidIn,
											 const char *const characteristicUuidIn,
											 bool wasSuccessfulIn,
											 cxa_fixedByteBuffer_t *fbb_readDataIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// notify our callback
	if( connIn->cbs.readFromChar.func != NULL )
	{
		cxa_btle_connection_cb_onReadComplete_t cb = connIn->cbs.readFromChar.func;
		void* userVar = connIn->cbs.readFromChar.userVar;

		connIn->cbs.readFromChar.func = NULL;
		connIn->cbs.readFromChar.userVar = NULL;

		cb(wasSuccessfulIn, fbb_readDataIn, userVar);
	}
}


void cxa_btle_connection_notify_notiIndiSubscriptionChanged(cxa_btle_connection_t *const connIn,
															const char *const serviceUuidIn,
															const char *const characteristicUuidIn,
															bool wasSuccessfulIn,
															bool notificationsEnableIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// notify our callback
	if( notificationsEnableIn && (connIn->cbs.subscribeToChar.func != NULL) )
	{
		cxa_btle_connection_cb_onNotiIndiSubscriptionChanged_t cb = connIn->cbs.subscribeToChar.func;
		void* userVar = connIn->cbs.subscribeToChar.userVar;

		connIn->cbs.subscribeToChar.func = NULL;
		connIn->cbs.subscribeToChar.userVar = NULL;

		cb(serviceUuidIn, characteristicUuidIn, wasSuccessfulIn, userVar);
	}
	else if( !notificationsEnableIn && (connIn->cbs.unsubscribeFromChar.func != NULL) )
	{
		cxa_btle_connection_cb_onNotiIndiSubscriptionChanged_t cb = connIn->cbs.unsubscribeFromChar.func;
		void* userVar = connIn->cbs.unsubscribeFromChar.userVar;

		connIn->cbs.unsubscribeFromChar.func = NULL;
		connIn->cbs.unsubscribeFromChar.userVar = NULL;

		cb(serviceUuidIn, characteristicUuidIn, wasSuccessfulIn, userVar);
	}
}


void cxa_btle_connection_notify_notiIndiRx(cxa_btle_connection_t *const connIn,
										   const char *const serviceUuidIn,
										   const char *const characteristicUuidIn,
										   cxa_fixedByteBuffer_t *fbb_dataIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure the UUIDs checkout
	cxa_btle_uuid_t tmpServiceUuid, tmpCharUuid;
	if( !cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidIn) ||
		!cxa_btle_uuid_initFromString(&tmpCharUuid, characteristicUuidIn) )
	{
		return;
	}

	// find our subscription entry
	for( size_t i = 0; i < cxa_array_getSize_elems(&connIn->notiIndiSubs); i++ )
	{
		cxa_btle_connection_notiIndiSubscription_t* currSub = cxa_array_get(&connIn->notiIndiSubs, i);
		if( currSub == NULL ) continue;

		if( cxa_btle_uuid_isEqual(&currSub->uuid_service, &tmpServiceUuid) &&
			cxa_btle_uuid_isEqual(&currSub->uuid_characteristic, &tmpCharUuid) )
		{
			if( currSub->cb_onRx != NULL ) currSub->cb_onRx(serviceUuidIn, characteristicUuidIn, fbb_dataIn, currSub->userVar);
		}
	}
}


// ******** local function implementations ********
