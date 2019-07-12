/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_btle_peripheral.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL					CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_btle_peripheral_charEntry_t* getCharEntry(cxa_btle_peripheral_t *const btlepIn,
													 cxa_btle_uuid_t *const serviceUuidIn,
													 cxa_btle_uuid_t *const charUuidIn);

static cxa_btle_peripheral_deferredOperationEntry_t* reserveDeferredOpEntry(cxa_btle_peripheral_t *const btlepIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_btle_peripheral_init(cxa_btle_peripheral_t *const btlepIn,
							  cxa_btle_peripheral_scm_sendNotification_t scm_sendNotificationIn,
							  cxa_btle_peripheral_scm_sendDeferredReadResponse_t scm_sendDeferredReadResponseIn,
							  cxa_btle_peripheral_scm_sendDeferredWriteResponse_t scm_sendDeferredWriteResponseIn,
							  cxa_btle_peripheral_scm_setAdvertisingInfo_t scm_setAdvertisingInfoIn,
							  cxa_btle_peripheral_scm_startAdvertising_t scm_startAdvertisingIn)
{
	cxa_assert(btlepIn);

	// save our references and setup our internal state
	btlepIn->scms.sendNotification = scm_sendNotificationIn;
	btlepIn->scms.sendDeferredReadResponse = scm_sendDeferredReadResponseIn;
	btlepIn->scms.sendDeferredWriteResponse = scm_sendDeferredWriteResponseIn;
	btlepIn->scms.setAdvertisingInfo = scm_setAdvertisingInfoIn;
	btlepIn->scms.startAdvertising = scm_startAdvertisingIn;

	cxa_array_initStd(&btlepIn->charEntries, btlepIn->charEntries_raw);
	cxa_array_initStd(&btlepIn->listeners, btlepIn->listeners_raw);

	// free all of our deferred operations
	for( size_t i = 0; i < sizeof(btlepIn->deferredOperations)/sizeof(*btlepIn->deferredOperations); i++ )
	{
		btlepIn->deferredOperations[i].isInUse = false;
	}

	cxa_logger_init(&btlepIn->logger, "btlePeripheral");
}


void cxa_btle_peripheral_addListener(cxa_btle_peripheral_t *const btlepIn,
									 cxa_btle_peripheral_cb_onReady_t cb_onReadyIn,
									 cxa_btle_peripheral_cb_onFailedInit_t cb_onFailedInitIn,
									 cxa_btle_peripheral_cb_onConnectionOpened_t cb_onConnectionOpenedIn,
									 cxa_btle_peripheral_cb_onConnectionClosed_t cb_onConnectionClosedIn,
									 void* userVarIn)
{
	cxa_assert(btlepIn);

	cxa_btle_peripheral_listener_entry_t newEntry =
	{
			.cb_onReady = cb_onReadyIn,
			.cb_onFailedInit = cb_onFailedInitIn,
			.cb_onConnectionOpened = cb_onConnectionOpenedIn,
			.cb_onConnectionClosed = cb_onConnectionClosedIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&btlepIn->listeners, &newEntry));
}


void cxa_btle_peripheral_registerCharacteristicHandler_read(cxa_btle_peripheral_t *const btlepIn,
															const char *const serviceUuidStrIn,
															const char *const charUuidStrIn,
															cxa_btle_peripheral_cb_onReadRequest_t cb_onReadIn, void *userVarIn)
{
	cxa_assert(btlepIn);
	cxa_assert(serviceUuidStrIn);
	cxa_assert(charUuidStrIn);

	cxa_btle_uuid_t tmpServiceUuid;
	cxa_btle_uuid_t tmpCharUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));
	cxa_assert(cxa_btle_uuid_initFromString(&tmpCharUuid, charUuidStrIn));

	cxa_btle_peripheral_charEntry_t* targetEntry = getCharEntry(btlepIn, &tmpServiceUuid, &tmpCharUuid);
	if( targetEntry == NULL ) targetEntry = cxa_array_append_empty(&btlepIn->charEntries);
	cxa_assert_msg(targetEntry, "increase CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES");

	cxa_btle_uuid_initFromUuid(&targetEntry->serviceUuid, &tmpServiceUuid, false);
	cxa_btle_uuid_initFromUuid(&targetEntry->charUuid, &tmpCharUuid, false);
	targetEntry->cbs.onReadRequest = cb_onReadIn;
	targetEntry->cbs.onDeferredReadRequest = NULL;
	targetEntry->cbs.userVar_read = userVarIn;
}


void cxa_btle_peripheral_registerCharacteristicHandler_deferredRead(cxa_btle_peripheral_t *const btlepIn,
																	const char *const serviceUuidStrIn,
																	const char *const charUuidStrIn,
																	cxa_btle_peripheral_cb_onDeferredReadRequest_t cb_onReadIn, void* userVarIn)
{
	cxa_assert(btlepIn);
	cxa_assert(serviceUuidStrIn);
	cxa_assert(charUuidStrIn);

	cxa_btle_uuid_t tmpServiceUuid;
	cxa_btle_uuid_t tmpCharUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));
	cxa_assert(cxa_btle_uuid_initFromString(&tmpCharUuid, charUuidStrIn));

	cxa_btle_peripheral_charEntry_t* targetEntry = getCharEntry(btlepIn, &tmpServiceUuid, &tmpCharUuid);
	if( targetEntry == NULL ) targetEntry = cxa_array_append_empty(&btlepIn->charEntries);
	cxa_assert_msg(targetEntry, "increase CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES");

	cxa_btle_uuid_initFromUuid(&targetEntry->serviceUuid, &tmpServiceUuid, false);
	cxa_btle_uuid_initFromUuid(&targetEntry->charUuid, &tmpCharUuid, false);
	targetEntry->cbs.onReadRequest = NULL;
	targetEntry->cbs.onDeferredReadRequest = cb_onReadIn;
	targetEntry->cbs.userVar_read = userVarIn;
}


void cxa_btle_peripheral_registerCharacteristicHandler_write(cxa_btle_peripheral_t *const btlepIn,
															 const char *const serviceUuidStrIn,
															 const char *const charUuidStrIn,
															 cxa_btle_peripheral_cb_onWriteRequest_t cb_onWriteIn, void *userVarIn)
{
	cxa_assert(btlepIn);
	cxa_assert(serviceUuidStrIn);
	cxa_assert(charUuidStrIn);

	cxa_btle_uuid_t tmpServiceUuid;
	cxa_btle_uuid_t tmpCharUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));
	cxa_assert(cxa_btle_uuid_initFromString(&tmpCharUuid, charUuidStrIn));

	cxa_btle_peripheral_charEntry_t* targetEntry = getCharEntry(btlepIn, &tmpServiceUuid, &tmpCharUuid);
	if( targetEntry == NULL ) targetEntry = cxa_array_append_empty(&btlepIn->charEntries);
	cxa_assert_msg(targetEntry, "increase CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES");

	cxa_btle_uuid_initFromUuid(&targetEntry->serviceUuid, &tmpServiceUuid, false);
	cxa_btle_uuid_initFromUuid(&targetEntry->charUuid, &tmpCharUuid, false);
	targetEntry->cbs.onWriteRequest = cb_onWriteIn;
	targetEntry->cbs.onDeferredWriteRequest = NULL;
	targetEntry->cbs.userVar_write = userVarIn;
}


void cxa_btle_peripheral_registerCharacteristicHandler_deferredWrite(cxa_btle_peripheral_t *const btlepIn,
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 const char *const serviceUuidStrIn,
																	 const char *const charUuidStrIn,
																	 cxa_btle_peripheral_cb_onDeferredWriteRequest_t cb_onWriteIn, void* userVarIn)
{
	cxa_assert(btlepIn);
	cxa_assert(serviceUuidStrIn);
	cxa_assert(charUuidStrIn);

	cxa_btle_uuid_t tmpServiceUuid;
	cxa_btle_uuid_t tmpCharUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));
	cxa_assert(cxa_btle_uuid_initFromString(&tmpCharUuid, charUuidStrIn));

	cxa_btle_peripheral_charEntry_t* targetEntry = getCharEntry(btlepIn, &tmpServiceUuid, &tmpCharUuid);
	if( targetEntry == NULL ) targetEntry = cxa_array_append_empty(&btlepIn->charEntries);
	cxa_assert_msg(targetEntry, "increase CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES");

	cxa_btle_uuid_initFromUuid(&targetEntry->serviceUuid, &tmpServiceUuid, false);
	cxa_btle_uuid_initFromUuid(&targetEntry->charUuid, &tmpCharUuid, false);
	targetEntry->cbs.onWriteRequest = NULL;
	targetEntry->cbs.onDeferredWriteRequest = cb_onWriteIn;
	targetEntry->cbs.userVar_write = userVarIn;
}


void cxa_btle_peripheral_setAdvertisingInfo(cxa_btle_peripheral_t *const btlepIn,
										  	uint32_t advertPeriod_msIn,
											cxa_fixedByteBuffer_t *const fbbAdvertDataIn)
{
	cxa_assert(btlepIn);

	cxa_assert(btlepIn->scms.setAdvertisingInfo);
	btlepIn->scms.setAdvertisingInfo(btlepIn, advertPeriod_msIn, fbbAdvertDataIn);
}


void cxa_btle_peripheral_sendNotification(cxa_btle_peripheral_t *const btlepIn,
										  const char *const serviceUuidStrIn,
										  const char *const charUuidStrIn,
										  void *const dataIn,
										  size_t numBytesIn)
{
	cxa_assert(btlepIn);
	cxa_assert(serviceUuidStrIn);
	cxa_assert(charUuidStrIn);
	cxa_assert(dataIn);

	cxa_fixedByteBuffer_t fbb_data;
	cxa_fixedByteBuffer_init_inPlace(&fbb_data, numBytesIn, dataIn, numBytesIn);

	cxa_btle_peripheral_sendNotification_fbb(btlepIn, serviceUuidStrIn, charUuidStrIn, &fbb_data);
}


void cxa_btle_peripheral_sendNotification_fbb(cxa_btle_peripheral_t *const btlepIn,
											  const char *const serviceUuidStrIn,
											  const char *const charUuidStrIn,
											  cxa_fixedByteBuffer_t *const fbb_dataIn)
{
	cxa_assert(btlepIn);
	cxa_assert(serviceUuidStrIn);
	cxa_assert(charUuidStrIn);
	cxa_assert(fbb_dataIn);

	cxa_assert(btlepIn->scms.sendNotification);
	btlepIn->scms.sendNotification(btlepIn, serviceUuidStrIn, charUuidStrIn, fbb_dataIn);
}


void cxa_btle_peripheral_completeDeferredRead(cxa_btle_peripheral_t *const btlepIn,
											  cxa_btle_peripheral_deferredOperationEntry_t* doeIn,
											  cxa_btle_peripheral_readRetVal_t retValIn,
											  cxa_fixedByteBuffer_t *const fbbReadDataIn)
{
	cxa_assert(btlepIn);
	cxa_assert(doeIn);

	cxa_btle_uuid_string_t serviceUuidStr, charUuidStr;
	cxa_btle_uuid_toString(&doeIn->serviceUuid, &serviceUuidStr);
	cxa_btle_uuid_toString(&doeIn->charUuid, &charUuidStr);

	btlepIn->scms.sendDeferredReadResponse(btlepIn, &doeIn->sourceMac, serviceUuidStr.str, charUuidStr.str, retValIn, fbbReadDataIn);

	// free our deferred operation entry
	doeIn->isInUse = false;
}


void cxa_btle_peripheral_completeDeferredWrite(cxa_btle_peripheral_t *const btlepIn,
											   cxa_btle_peripheral_deferredOperationEntry_t* doeIn,
											   cxa_btle_peripheral_writeRetVal_t retValIn)
{
	cxa_assert(btlepIn);
	cxa_assert(doeIn);

	cxa_btle_uuid_string_t serviceUuidStr, charUuidStr;
	cxa_btle_uuid_toString(&doeIn->serviceUuid, &serviceUuidStr);
	cxa_btle_uuid_toString(&doeIn->charUuid, &charUuidStr);

	btlepIn->scms.sendDeferredWriteResponse(btlepIn, &doeIn->sourceMac, serviceUuidStr.str, charUuidStr.str, retValIn);

	// free our deferred operation entry
	doeIn->isInUse = false;
}


void cxa_btle_peripheral_startAdvertising(cxa_btle_peripheral_t *const btlepIn,
										  uint32_t advertPeriod_msIn,
										  cxa_fixedByteBuffer_t *const fbbAdvertDataIn)
{
	cxa_assert(btlepIn);


}


void cxa_btle_peripheral_notify_onBecomesReady(cxa_btle_peripheral_t *const btlepIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->listeners, currListener, cxa_btle_peripheral_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onReady != NULL ) currListener->cb_onReady(btlepIn, currListener->userVar);
	}

	if( btlepIn->scms.startAdvertising != NULL ) btlepIn->scms.startAdvertising(btlepIn);
}


void cxa_btle_peripheral_notify_onFailedInit(cxa_btle_peripheral_t *const btlepIn, bool willAutoRetryIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->listeners, currListener, cxa_btle_peripheral_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onFailedInit != NULL ) currListener->cb_onFailedInit(btlepIn, willAutoRetryIn, currListener->userVar);
	}
}


void cxa_btle_peripheral_notify_connectionOpened(cxa_btle_peripheral_t *const btlepIn, cxa_eui48_t *const targetAddrIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->listeners, currListener, cxa_btle_peripheral_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onConnectionOpened != NULL ) currListener->cb_onConnectionOpened(targetAddrIn, currListener->userVar);
	}
}


void cxa_btle_peripheral_notify_connectionClosed(cxa_btle_peripheral_t *const btlepIn, cxa_eui48_t *const targetAddrIn)
{
	cxa_assert(btlepIn);
	cxa_assert(targetAddrIn);

	cxa_array_iterate(&btlepIn->listeners, currListener, cxa_btle_peripheral_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onConnectionClosed != NULL ) currListener->cb_onConnectionClosed(targetAddrIn, currListener->userVar);
	}

	if( btlepIn->scms.startAdvertising != NULL ) btlepIn->scms.startAdvertising(btlepIn);
}


bool cxa_btle_peripheral_notify_readRequest(cxa_btle_peripheral_t *const btlepIn,
											cxa_eui48_t *const sourceMacAddrIn,
											cxa_btle_uuid_t *const serviceUuidIn,
											cxa_btle_uuid_t *const charUuidIn,
											cxa_fixedByteBuffer_t *const dataOut,
											cxa_btle_peripheral_readRetVal_t *const retValOut)
{
	cxa_assert(btlepIn);
	cxa_assert(sourceMacAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(charUuidIn);
	cxa_assert(dataOut);
	cxa_assert(retValOut);

	bool shouldSendResponse = false;
	cxa_btle_peripheral_readRetVal_t retVal = CXA_BTLE_PERIPHERAL_READRET_ATTRIBUTE_NOT_FOUND;

	// check our registered read requests first
	cxa_btle_peripheral_charEntry_t* charEntry = getCharEntry(btlepIn, serviceUuidIn, charUuidIn);
	if( charEntry != NULL )
	{
		if( charEntry->cbs.onReadRequest != NULL )
		{
			shouldSendResponse = true;
			retVal = charEntry->cbs.onReadRequest(dataOut, charEntry->cbs.userVar_write);
		}
		else if( charEntry->cbs.onDeferredReadRequest != NULL )
		{
			cxa_btle_peripheral_deferredOperationEntry_t* doe = reserveDeferredOpEntry(btlepIn);
			if( doe != NULL )
			{
				// save our deferred information
				cxa_eui48_initFromEui48(&doe->sourceMac, sourceMacAddrIn);
				cxa_btle_uuid_initFromUuid(&doe->serviceUuid, serviceUuidIn, false);
				cxa_btle_uuid_initFromUuid(&doe->charUuid, charUuidIn, false);

				// call our callback
				retVal = charEntry->cbs.onDeferredReadRequest(doe, charEntry->cbs.userVar_read);

				// if it didn't start successfully, complete immediately
				if( retVal != CXA_BTLE_PERIPHERAL_READRET_SUCCESS )
				{
					cxa_btle_peripheral_completeDeferredRead(btlepIn, doe, retVal, NULL);
				}
			}
			else
			{
				retVal = CXA_BTLE_PERIPHERAL_READRET_UNLIKELY;
			}
		}
	}

	if( retValOut != NULL ) *retValOut = retVal;
	return shouldSendResponse;
}


bool cxa_btle_peripheral_notify_writeRequest(cxa_btle_peripheral_t *const btlepIn,
											 cxa_eui48_t *const sourceMacAddrIn,
											 cxa_btle_uuid_t *const serviceUuidIn,
											 cxa_btle_uuid_t *const charUuidIn,
											 cxa_fixedByteBuffer_t *const dataIn,
											 cxa_btle_peripheral_writeRetVal_t *const retValOut)
{
	cxa_assert(btlepIn);
	cxa_assert(sourceMacAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(charUuidIn);
	cxa_assert(dataIn);
	cxa_assert(retValOut);

	bool shouldSendResponse = false;
	cxa_btle_peripheral_writeRetVal_t retVal = CXA_BTLE_PERIPHERAL_WRITERET_ATTRIBUTE_NOT_FOUND;

	// check our registered write requests first
	cxa_btle_peripheral_charEntry_t* charEntry = getCharEntry(btlepIn, serviceUuidIn, charUuidIn);
	if( charEntry != NULL )
	{
		if( charEntry->cbs.onWriteRequest != NULL )
		{
			shouldSendResponse = true;
			retVal = charEntry->cbs.onWriteRequest(dataIn, charEntry->cbs.userVar_write);
		}
		else if( charEntry->cbs.onDeferredWriteRequest != NULL )
		{
			cxa_btle_peripheral_deferredOperationEntry_t* doe = reserveDeferredOpEntry(btlepIn);
			if( doe != NULL )
			{
				// save our deferred information
				cxa_eui48_initFromEui48(&doe->sourceMac, sourceMacAddrIn);
				cxa_btle_uuid_initFromUuid(&doe->serviceUuid, serviceUuidIn, false);
				cxa_btle_uuid_initFromUuid(&doe->charUuid, charUuidIn, false);

				// call our callback
				retVal = charEntry->cbs.onDeferredWriteRequest(doe, dataIn, charEntry->cbs.userVar_write);

				// if it didn't start successfully, complete immediately
				if( retVal != CXA_BTLE_PERIPHERAL_WRITERET_SUCCESS )
				{
					cxa_btle_peripheral_completeDeferredWrite(btlepIn, doe, retVal);
				}
			}
			else
			{
				retVal = CXA_BTLE_PERIPHERAL_WRITERET_UNLIKELY;
			}
		}
	}

	if( retValOut != NULL ) *retValOut = retVal;
	return shouldSendResponse;
}


// ******** local function implementations ********
static cxa_btle_peripheral_charEntry_t* getCharEntry(cxa_btle_peripheral_t *const btlepIn,
													 cxa_btle_uuid_t *const serviceUuidIn,
													 cxa_btle_uuid_t *const charUuidIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->charEntries, currEntry, cxa_btle_peripheral_charEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( cxa_btle_uuid_isEqual(&currEntry->serviceUuid, serviceUuidIn) &&
			cxa_btle_uuid_isEqual(&currEntry->charUuid, charUuidIn) )
		{
			return currEntry;
		}
	}
	// if we made it here, we couldn't find the target entry
	return NULL;
}


static cxa_btle_peripheral_deferredOperationEntry_t* reserveDeferredOpEntry(cxa_btle_peripheral_t *const btlepIn)
{
	cxa_assert(btlepIn);

	for( size_t i = 0; i < sizeof(btlepIn->deferredOperations)/sizeof(*btlepIn->deferredOperations); i++ )
	{
		if( !btlepIn->deferredOperations[i].isInUse )
		{
			btlepIn->deferredOperations[i].isInUse = true;
			return &btlepIn->deferredOperations[i];
		}
	}

	// if we made it here, we don't have any free entries
	return NULL;
}
