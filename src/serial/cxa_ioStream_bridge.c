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
	cxa_runLoop_addEntry(threadIdIn, cb_onRunLoopUpdate, (void*)bridgeIn);
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
