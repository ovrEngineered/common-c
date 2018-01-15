/**
 * Copyright 2015 opencxa.org
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
#include "cxa_protocolParser.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_protocolParser_init(cxa_protocolParser_t *const ppIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn,
							 cxa_protocolParser_scm_isInErrorState_t scm_isInErrorIn, cxa_protocolParser_scm_canSetBuffer_t scm_canSetBufferIn,
							 cxa_protocolParser_scm_gotoIdle_t scm_gotoIdleIn, cxa_protocolParser_scm_reset_t scm_resetIn,
							 cxa_protocolParser_scm_writeBytes_t scm_writeBytesIn)
{
	cxa_assert(ppIn);
	cxa_assert(ioStreamIn);
	cxa_assert(scm_isInErrorIn);
	cxa_assert(scm_canSetBufferIn);
	cxa_assert(scm_gotoIdleIn);
	cxa_assert(scm_resetIn);
	cxa_assert(scm_writeBytesIn);
	
	// save our references
	ppIn->ioStream = ioStreamIn;
	ppIn->currBuffer = buffIn;
	ppIn->scm_canSetBuffer = scm_canSetBufferIn;
	ppIn->scm_gotoIdle = scm_gotoIdleIn;
	ppIn->scm_isInError = scm_isInErrorIn;
	ppIn->scm_reset = scm_resetIn;
	ppIn->scm_writeBytes = scm_writeBytesIn;

	// setup our timediff
	cxa_timeDiff_init(&ppIn->td_timeout);

	// setup our logger
	cxa_logger_init(&ppIn->logger, "protocolParser");

	// setup our listeners
	cxa_array_initStd(&ppIn->protocolListeners, ppIn->protocolListeners_raw);
	cxa_array_initStd(&ppIn->packetListeners, ppIn->packetListeners_raw);
}


void cxa_protocolParser_addProtocolListener(cxa_protocolParser_t *const ppIn,
		cxa_protocolParser_cb_ioExceptionOccurred_t cb_exceptionIn,
		cxa_protocolParser_cb_receptionTimeout_t cb_receptionTimeoutIn,
		void *const userVarIn)
{
	cxa_assert(ppIn);

	// create and add our new entry
	cxa_protocolParser_protocolListener_entry_t newEntry = {.cb_exception=cb_exceptionIn, .cb_receptionTimeout=cb_receptionTimeoutIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&ppIn->protocolListeners, &newEntry) );
}


void cxa_protocolParser_addPacketListener(cxa_protocolParser_t *const ppIn,
	cxa_protocolParser_cb_packetReceived_t cb_msgRxIn,
	void *const userVarIn)
{
	cxa_assert(ppIn);

	// create and add our new entry
	cxa_protocolParser_packetListener_entry_t newEntry = {.cb=cb_msgRxIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&ppIn->packetListeners, &newEntry) );
}


void cxa_protocolParser_reset(cxa_protocolParser_t *const ppIn)
{
	cxa_assert(ppIn);

	ppIn->scm_reset(ppIn);
}


void cxa_protocolParser_setBuffer(cxa_protocolParser_t *const ppIn, cxa_fixedByteBuffer_t* buffIn)
{
	cxa_assert(ppIn);

	// handle our special cases
	if( buffIn == NULL)
	{
		ppIn->currBuffer = NULL;
		ppIn->scm_gotoIdle(ppIn);
	}
	else if( ppIn->scm_canSetBuffer(ppIn) )
	{
		// set the buffer
		// (state machine will take care of starting automatically if needed)
		ppIn->currBuffer = buffIn;
	}
	else
	{
		// get to the idle state first
		ppIn->scm_gotoIdle(ppIn);

		// set the buffer
		// (state machine will take care of starting automatically)
		ppIn->currBuffer = buffIn;
	}
}


cxa_fixedByteBuffer_t* cxa_protocolParser_getBuffer(cxa_protocolParser_t *const ppIn)
{
	cxa_assert(ppIn);
	return ppIn->currBuffer;
}


bool cxa_protocolParser_writePacket(cxa_protocolParser_t *const ppIn, cxa_fixedByteBuffer_t *const dataIn)
{
	cxa_assert(ppIn);
	return ppIn->scm_writeBytes(ppIn, dataIn);
}


bool cxa_protocolParser_writePacket_bytes(cxa_protocolParser_t *const ppIn, void* bytesIn, size_t numBytesIn)
{
	cxa_assert(ppIn);

	cxa_fixedByteBuffer_t tmpFbb;
	cxa_fixedByteBuffer_init_inPlace(&tmpFbb, numBytesIn, bytesIn, numBytesIn);

	return cxa_protocolParser_writePacket(ppIn, &tmpFbb);
}


void cxa_protocolParser_resetError(cxa_protocolParser_t *const ppIn)
{
	cxa_assert(ppIn);

	// get to the idle state...the state machine
	// should take care of the rest
	ppIn->scm_gotoIdle(ppIn);
}


void cxa_protocolParser_notify_ioException(cxa_protocolParser_t *const ppIn)
{
	cxa_assert(ppIn);
	
	cxa_logger_error(&ppIn->logger, "underlying serial device is broken, protocol parser is inoperable");

	// notify our protocol listeners
	cxa_array_iterate(&ppIn->protocolListeners, currEntry, cxa_protocolParser_protocolListener_entry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->cb_exception != NULL ) currEntry->cb_exception(currEntry->userVar);
	}
}


void cxa_protocolParser_notify_receptionTimeout(cxa_protocolParser_t *const ppIn)
{
	cxa_assert(ppIn);
	
	cxa_logger_warn(&ppIn->logger, "reception timeout");

	// notify our protocol listeners
	cxa_array_iterate(&ppIn->protocolListeners, currEntry, cxa_protocolParser_protocolListener_entry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->cb_receptionTimeout != NULL ) currEntry->cb_receptionTimeout(ppIn->currBuffer, currEntry->userVar);
	}
}


void cxa_protocolParser_notify_packetReceived(cxa_protocolParser_t *const ppIn, cxa_fixedByteBuffer_t *const packetIn)
{
	cxa_assert(ppIn);

	cxa_array_iterate(&ppIn->packetListeners, currEntry, cxa_protocolParser_packetListener_entry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->cb != NULL )
		{
			currEntry->cb(ppIn->currBuffer, currEntry->userVar);
		}
	}
}


// ******** local function implementations ********
