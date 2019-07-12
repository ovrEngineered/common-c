/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_ioStream_bridge.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ioStream_bridge_init(cxa_ioStream_bridge_t *const bridgeIn,
								cxa_ioStream_t *const stream1In,
								cxa_ioStream_t *const stream2In,
								int threadIdIn)
{
	cxa_assert(bridgeIn);
	cxa_assert(stream1In);
	cxa_assert(stream2In);

	// save our references
	bridgeIn->stream1 = stream1In;
	bridgeIn->stream2 = stream2In;

	// register for run loop execution
	cxa_runLoop_addEntry(threadIdIn, NULL, cb_onRunLoopUpdate, (void*)bridgeIn);
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_ioStream_bridge_t* bridgeIn = (cxa_ioStream_bridge_t*)userVarIn;

	uint8_t rxByte;
	if( cxa_ioStream_readByte(bridgeIn->stream1, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		cxa_ioStream_writeByte(bridgeIn->stream2, rxByte);
	}

	if( cxa_ioStream_readByte(bridgeIn->stream2, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		cxa_ioStream_writeByte(bridgeIn->stream1, rxByte);
	}
}
