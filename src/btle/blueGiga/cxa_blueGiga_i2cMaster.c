/**
 * Copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_blueGiga_i2cMaster.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>
#include <cxa_blueGiga_btle_client.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define TIMEOUT_DEFAULT_MS			3000


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_readBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, size_t numBytesToReadIn);
static void scm_writeBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, cxa_fixedByteBuffer_t *const writeBytesIn);

static void responseCb_i2cRead(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_i2cWrite(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_blueGiga_i2cMaster_init(cxa_blueGiga_i2cMaster_t *const i2cIn, cxa_blueGiga_btle_client_t* btlecIn)
{
	cxa_assert(i2cIn);
	cxa_assert(btlecIn);
	
	// save our references
	i2cIn->btlec = btlecIn;

	// initialize our super class
	cxa_i2cMaster_init(&i2cIn->super, scm_readBytes, scm_writeBytes);
}


// ******** local function implementations ********
static void scm_readBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, size_t numBytesToReadIn)
{
	cxa_blueGiga_i2cMaster_t* i2cIn = (cxa_blueGiga_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	// need our radio to be ready
	if( !cxa_btle_client_isReady(&i2cIn->btlec->super) )
	{
		cxa_i2cMaster_notify_readComplete(&i2cIn->super, false, NULL);
		return;
	}

	// create our payload
	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t payload_raw[3];
	cxa_fixedByteBuffer_initStd(&fbb_payload, payload_raw);
	if( !cxa_fixedByteBuffer_append_uint8(&fbb_payload, addressIn) ) return;
	if( !cxa_fixedByteBuffer_append_uint8(&fbb_payload, sendStopIn) ) return;
	if( !cxa_fixedByteBuffer_append_uint8(&fbb_payload, numBytesToReadIn) ) return;

	if( !cxa_blueGiga_btle_client_sendCommand(i2cIn->btlec, CXA_BLUEGIGA_CLASSID_HARDWARE, CXA_BLUEGIGA_METHODID_HW_I2CREAD, &fbb_payload, responseCb_i2cRead, (void*)i2cIn) )
	{
		cxa_i2cMaster_notify_readComplete(&i2cIn->super, false, NULL);
	}
}


static void scm_writeBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, cxa_fixedByteBuffer_t *const writeBytesIn)
{
	cxa_blueGiga_i2cMaster_t* i2cIn = (cxa_blueGiga_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	// need our radio to be ready
	if( !cxa_btle_client_isReady(&i2cIn->btlec->super) )
	{
		cxa_i2cMaster_notify_writeComplete(&i2cIn->super, false);
		return;
	}


	i2cIn->expectedNumBytesToWrite = (writeBytesIn != NULL) ? cxa_fixedByteBuffer_getSize_bytes(writeBytesIn) : 0;

	// create our payload
	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t payload_raw[3 + i2cIn->expectedNumBytesToWrite];
	cxa_fixedByteBuffer_initStd(&fbb_payload, payload_raw);
	if( !cxa_fixedByteBuffer_append_uint8(&fbb_payload, addressIn) ) return;
	if( !cxa_fixedByteBuffer_append_uint8(&fbb_payload, sendStopIn) ) return;
	if( !cxa_fixedByteBuffer_append_uint8(&fbb_payload, i2cIn->expectedNumBytesToWrite) ) return;
	if( (writeBytesIn != NULL) && !cxa_fixedByteBuffer_append_fbb(&fbb_payload, writeBytesIn) ) return;

	if( !cxa_blueGiga_btle_client_sendCommand(i2cIn->btlec, CXA_BLUEGIGA_CLASSID_HARDWARE, CXA_BLUEGIGA_METHODID_HW_I2CWRITE, &fbb_payload, responseCb_i2cWrite, (void*)i2cIn) )
	{
		cxa_i2cMaster_notify_writeComplete(&i2cIn->super, false);
	}
}


static void responseCb_i2cRead(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_blueGiga_i2cMaster_t* i2cIn = (cxa_blueGiga_i2cMaster_t*)userVarIn;
	cxa_assert(i2cIn);
	cxa_assert(payloadIn);

	// look for our response
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 0, response) || (response != 0) )
	{
		cxa_i2cMaster_notify_readComplete(&i2cIn->super, false, NULL);
		return;
	}

	// if we made it here, we were successful
	cxa_fixedByteBuffer_t fbb_readBytes;
	cxa_fixedByteBuffer_t* fbb_readBytes_ptr;
	if( cxa_fixedByteBuffer_getSize_bytes(payloadIn) > 2 )
	{
		// start at index 3 since index 2 is length of byte array
		cxa_fixedByteBuffer_init_subBufferRemainingElems(&fbb_readBytes, payloadIn, 3);
		fbb_readBytes_ptr = &fbb_readBytes;
	}

	cxa_i2cMaster_notify_readComplete(&i2cIn->super, true, fbb_readBytes_ptr);
}


static void responseCb_i2cWrite(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_blueGiga_i2cMaster_t* i2cIn = (cxa_blueGiga_i2cMaster_t*)userVarIn;
	cxa_assert(i2cIn);

	// figure out how many bytes were written
	if( wasSuccessfulIn )
	{
		uint8_t numBytesWritten;
		if( !cxa_fixedByteBuffer_get_uint8(payloadIn, 0, numBytesWritten) || (numBytesWritten != i2cIn->expectedNumBytesToWrite) )
		{
			wasSuccessfulIn = false;
		}
	}

	cxa_i2cMaster_notify_writeComplete(&i2cIn->super, wasSuccessfulIn);
}

