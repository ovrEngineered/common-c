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

static cxa_btle_peripheral_centralProxy_service_entry_t* getCentralProxyServiceEntry(cxa_btle_peripheral_t *const btlepIn,
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 cxa_btle_uuid_t *const serviceUuidIn);

static void btleCb_centralProxyService_onReadComplete(cxa_eui48_t *const targetAddrIn,
		 	 	 	 	 	 	 	 	 	 	 	  const char *const serviceUuidStrIn,
													  const char *const characteristicUuidStrIn,
													  bool wasSuccessfulIn,
													  cxa_fixedByteBuffer_t *fbb_readDataIn,
													  void* userVarIn);

static void btleCb_centralProxyService_onWriteComplete(cxa_eui48_t *const targetAddrIn,
		  	  	  	  	  	  	  	  	  	  	  	   const char *const serviceUuidIn,
													   const char *const characteristicUuidIn,
													   bool wasSuccessfulIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_btle_peripheral_init(cxa_btle_peripheral_t *const btlepIn,
							  cxa_btle_peripheral_scm_sendNotification_t scm_sendNotificationIn,
							  cxa_btle_peripheral_scm_sendDeferredReadResponse_t scm_sendDeferredReadResponseIn,
							  cxa_btle_peripheral_scm_sendDeferredWriteResponse_t scm_sendDeferredWriteResponseIn)
{
	cxa_assert(btlepIn);

	// save our references and setup our internal state
	btlepIn->scms.sendNotification = scm_sendNotificationIn;
	btlepIn->scms.sendDeferredReadResponseIn = scm_sendDeferredReadResponseIn;
	btlepIn->scms.sendDeferredWriteResponseIn = scm_sendDeferredWriteResponseIn;

	cxa_array_initStd(&btlepIn->charEntries, btlepIn->charEntries_raw);
	cxa_array_initStd(&btlepIn->listeners, btlepIn->listeners_raw);
	cxa_array_initStd(&btlepIn->centralProxy_services, btlepIn->centralProxy_services_raw);

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
	targetEntry->cbs.userVar_write = userVarIn;
}


void cxa_btle_peripheral_registerCentralProxy_service(cxa_btle_peripheral_t *const btlepIn,
													  const char *const serviceUuidStrIn,
													  cxa_btle_central_t *const btlecIn,
													  cxa_eui48_t *const targetMacIn)
{
	cxa_assert(btlepIn);
	cxa_assert(serviceUuidStrIn);
	cxa_assert(btlecIn);
	cxa_assert(targetMacIn);

	cxa_btle_uuid_t tmpServiceUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));

	cxa_btle_peripheral_centralProxy_service_entry_t* cpsEntry = getCentralProxyServiceEntry(btlepIn, &tmpServiceUuid);
	if( cpsEntry == NULL )
	{
		cpsEntry = cxa_array_append_empty(&btlepIn->centralProxy_services);
		cxa_assert(cpsEntry);
	}
	cxa_btle_uuid_initFromUuid(&cpsEntry->serviceUuid, &tmpServiceUuid, false);
	cxa_eui48_initFromEui48(&cpsEntry->targetMac, targetMacIn);
	cpsEntry->btlec = btlecIn;
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


void cxa_btle_peripheral_notify_onBecomesReady(cxa_btle_peripheral_t *const btlepIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->listeners, currListener, cxa_btle_peripheral_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onReady != NULL ) currListener->cb_onReady(btlepIn, currListener->userVar);
	}
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
}


cxa_btle_peripheral_readRetVal_t cxa_btle_peripheral_notify_readRequest(cxa_btle_peripheral_t *const btlepIn,
																		cxa_eui48_t *const sourceMacAddrIn,
																		cxa_btle_uuid_t *const serviceUuidIn,
																		cxa_btle_uuid_t *const charUuidIn,
																		cxa_fixedByteBuffer_t *const dataOut)
{
	cxa_assert(btlepIn);
	cxa_assert(sourceMacAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(charUuidIn);
	cxa_assert(dataOut);

	cxa_btle_peripheral_readRetVal_t retVal = CXA_BTLE_PERIPHERAL_READRET_ATTRIBUTE_NOT_FOUND;

	// check our registered read requests first
	cxa_btle_peripheral_charEntry_t* charEntry = getCharEntry(btlepIn, serviceUuidIn, charUuidIn);
	if( charEntry != NULL )
	{
		if( charEntry->cbs.onReadRequest != NULL ) retVal = charEntry->cbs.onReadRequest(dataOut, charEntry->cbs.userVar_write);
	}

	// now check our centralProxy services
	cxa_btle_peripheral_centralProxy_service_entry_t* cpsEntry = getCentralProxyServiceEntry(btlepIn, serviceUuidIn);
	if( cpsEntry != NULL )
	{
		// make sure we have the ability to support a deferred read
		if( btlepIn->scms.sendDeferredReadResponseIn != NULL )
		{
			// save the source mac address
			cxa_eui48_initFromEui48(&cpsEntry->sourceMac, sourceMacAddrIn);

			// perform the read using our central
			cxa_btle_uuid_string_t serviceUuidStr, charUuidStr;
			cxa_btle_uuid_toString(serviceUuidIn, &serviceUuidStr);
			cxa_btle_uuid_toString(charUuidIn, &charUuidStr);

			// defer the sending of the response until we get a callback from the central
			cxa_btle_central_readFromCharacteristic(cpsEntry->btlec, &cpsEntry->targetMac, serviceUuidStr.str, charUuidStr.str, btleCb_centralProxyService_onReadComplete, (void*)btlepIn);
			retVal = CXA_BTLE_PERIPHERAL_READRET_NOT_YET_COMPLETE;
		}
	}

	return retVal;
}


cxa_btle_peripheral_writeRetVal_t cxa_btle_peripheral_notify_writeRequest(cxa_btle_peripheral_t *const btlepIn,
																		  cxa_eui48_t *const sourceMacAddrIn,
																		  cxa_btle_uuid_t *const serviceUuidIn,
																		  cxa_btle_uuid_t *const charUuidIn,
																		  cxa_fixedByteBuffer_t *const dataIn)
{
	cxa_assert(btlepIn);
	cxa_assert(sourceMacAddrIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(charUuidIn);
	cxa_assert(dataIn);

	cxa_btle_peripheral_writeRetVal_t retVal = CXA_BTLE_PERIPHERAL_WRITERET_ATTRIBUTE_NOT_FOUND;

	// check our registered write requests first
	cxa_btle_peripheral_charEntry_t* charEntry = getCharEntry(btlepIn, serviceUuidIn, charUuidIn);
	if( charEntry != NULL )
	{
		if( charEntry->cbs.onWriteRequest != NULL ) retVal = charEntry->cbs.onWriteRequest(dataIn, charEntry->cbs.userVar_write);
	}

	// now check our centralProxy services
	cxa_btle_peripheral_centralProxy_service_entry_t* cpsEntry = getCentralProxyServiceEntry(btlepIn, serviceUuidIn);
	if( cpsEntry != NULL )
	{
		// make sure we have the ability to support a deferred write
		if( btlepIn->scms.sendDeferredWriteResponseIn != NULL )
		{
			// save the source mac address
			cxa_eui48_initFromEui48(&cpsEntry->sourceMac, sourceMacAddrIn);

			// perform the write using our central
			cxa_btle_uuid_string_t serviceUuidStr, charUuidStr;
			cxa_btle_uuid_toString(serviceUuidIn, &serviceUuidStr);
			cxa_btle_uuid_toString(charUuidIn, &charUuidStr);

			// defer the sending of the response until we get a callback from the central
			cxa_btle_central_writeToCharacteristic_fbb(cpsEntry->btlec, &cpsEntry->targetMac, serviceUuidStr.str, charUuidStr.str, dataIn, btleCb_centralProxyService_onWriteComplete, (void*)btlepIn);
			retVal = CXA_BTLE_PERIPHERAL_READRET_NOT_YET_COMPLETE;
		}
	}

	return retVal;
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


static cxa_btle_peripheral_centralProxy_service_entry_t* getCentralProxyServiceEntry(cxa_btle_peripheral_t *const btlepIn,
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 cxa_btle_uuid_t *const serviceUuidIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->centralProxy_services, currEntry, cxa_btle_peripheral_centralProxy_service_entry_t)
	{
		if( currEntry == NULL ) continue;

		if( cxa_btle_uuid_isEqual(&currEntry->serviceUuid, serviceUuidIn) )
		{
			return currEntry;
		}
	}

	return NULL;
}


static void btleCb_centralProxyService_onReadComplete(cxa_eui48_t *const targetAddrIn,
		 	 	 	 	 	 	 	 	 	 	 	  const char *const serviceUuidStrIn,
													  const char *const characteristicUuidStrIn,
													  bool wasSuccessfulIn,
													  cxa_fixedByteBuffer_t *fbb_readDataIn,
													  void* userVarIn)
{
	cxa_btle_peripheral_t *const btlepIn = (cxa_btle_peripheral_t *const)userVarIn;
	cxa_assert(btlepIn);

	cxa_btle_uuid_t tmpServiceUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));

	cxa_btle_peripheral_centralProxy_service_entry_t* cpsEntry = getCentralProxyServiceEntry(btlepIn, &tmpServiceUuid);
	if( cpsEntry != NULL )
	{
		cxa_btle_peripheral_readRetVal_t retVal = wasSuccessfulIn ? CXA_BTLE_PERIPHERAL_READRET_SUCCESS : CXA_BTLE_PERIPHERAL_READRET_UNLIKELY;

		btlepIn->scms.sendDeferredReadResponseIn(btlepIn, &cpsEntry->sourceMac, serviceUuidStrIn, characteristicUuidStrIn, retVal, fbb_readDataIn);
	}
}


static void btleCb_centralProxyService_onWriteComplete(cxa_eui48_t *const targetAddrIn,
		  	  	  	  	  	  	  	  	  	  	  	   const char *const serviceUuidStrIn,
													   const char *const characteristicUuidStrIn,
													   bool wasSuccessfulIn, void* userVarIn)
{
	cxa_btle_peripheral_t *const btlepIn = (cxa_btle_peripheral_t *const)userVarIn;
	cxa_assert(btlepIn);

	cxa_btle_uuid_t tmpServiceUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));

	cxa_btle_peripheral_centralProxy_service_entry_t* cpsEntry = getCentralProxyServiceEntry(btlepIn, &tmpServiceUuid);
	if( cpsEntry != NULL )
	{
		cxa_btle_peripheral_readRetVal_t retVal = wasSuccessfulIn ? CXA_BTLE_PERIPHERAL_READRET_SUCCESS : CXA_BTLE_PERIPHERAL_READRET_UNLIKELY;

		btlepIn->scms.sendDeferredWriteResponseIn(btlepIn, &cpsEntry->sourceMac, serviceUuidStrIn, characteristicUuidStrIn, retVal);
	}
}
