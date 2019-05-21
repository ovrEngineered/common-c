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


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_btle_peripheral_init(cxa_btle_peripheral_t *const btlepIn)
{
	cxa_assert(btlepIn);

	// save our references and setup our internal state
	cxa_array_initStd(&btlepIn->charEntries, btlepIn->charEntries_raw);
	cxa_array_initStd(&btlepIn->listeners, btlepIn->listeners_raw);

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


cxa_btle_peripheral_writeRetVal_t cxa_btle_peripheral_notify_writeRequest(cxa_btle_peripheral_t *const btlepIn,
																		  cxa_btle_uuid_t *const serviceUuidIn,
																		  cxa_btle_uuid_t *const charUuidIn,
																		  cxa_fixedByteBuffer_t *const dataIn)
{
	cxa_assert(btlepIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(charUuidIn);
	cxa_assert(dataIn);

	cxa_btle_peripheral_writeRetVal_t retVal = CXA_BTLE_PERIPHERAL_WRITERET_ATTRIBUTE_NOT_FOUND;
	cxa_array_iterate(&btlepIn->charEntries, currEntry, cxa_btle_peripheral_charEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( cxa_btle_uuid_isEqual(&currEntry->serviceUuid, serviceUuidIn) &&
			cxa_btle_uuid_isEqual(&currEntry->charUuid, charUuidIn) )
		{
			if( currEntry->cbs.onWriteRequest != NULL ) retVal = currEntry->cbs.onWriteRequest(dataIn, currEntry->cbs.userVar_write);
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
