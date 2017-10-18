/**
 * @copyright 2017 opencxa.org
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
#include "cxa_i2cMaster.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_i2cMaster_init(cxa_i2cMaster_t *const i2cIn,
					    cxa_i2cMaster_scm_readBytes_t scm_readIn,
						cxa_i2cMaster_scm_readBytesWithControlBytes_t scm_readWithControlBytesIn,
						cxa_i2cMaster_scm_writeBytes_t scm_writeIn,
						cxa_i2cMaster_scm_resetBus_t scm_resetBusIn)
{
	cxa_assert(i2cIn);
	cxa_assert(scm_readIn);
	cxa_assert(scm_writeIn);

	// save our references
	i2cIn->scms.readBytes = scm_readIn;
	i2cIn->scms.readBytesWithControlBytes = scm_readWithControlBytesIn;
	i2cIn->scms.writeBytes = scm_writeIn;
	i2cIn->scms.resetBus = scm_resetBusIn;

	i2cIn->isBusy = false;
}


void cxa_i2cMaster_resetBus(cxa_i2cMaster_t *const i2cIn)
{
	cxa_assert(i2cIn);

	cxa_assert(i2cIn->scms.resetBus);
	i2cIn->scms.resetBus(i2cIn);
}


void cxa_i2cMaster_readBytes(cxa_i2cMaster_t *const i2cIn,
							 uint8_t addressIn, uint8_t sendStopIn, size_t numBytesToReadIn,
							 cxa_i2cMaster_cb_onReadComplete_t cbIn, void* userVarIn)
{
	cxa_assert(i2cIn);
	cxa_assert(numBytesToReadIn > 0);

	// don't do anything if we're still busy
	if( i2cIn->isBusy )
	{
		if( cbIn != NULL ) cbIn(i2cIn, false, NULL, userVarIn);
		return;
	}

	// if we made it here, we weren't busy...now we are
	i2cIn->isBusy = true;
	i2cIn->cbs.readComplete = cbIn;
	i2cIn->cbs.userVar = userVarIn;

	// call our subclass method
	cxa_assert(i2cIn->scms.readBytes);
	i2cIn->scms.readBytes(i2cIn, addressIn, sendStopIn, numBytesToReadIn);
}


void cxa_i2cMaster_readBytes_withControlBytes(cxa_i2cMaster_t *const i2cIn,
											 uint8_t addressIn, uint8_t sendStopIn,
											 cxa_fixedByteBuffer_t *const controlBytesIn,
											 size_t numBytesToReadIn,
											 cxa_i2cMaster_cb_onReadComplete_t cbIn, void* userVarIn)
{
	cxa_assert(i2cIn);
	cxa_assert(controlBytesIn && (cxa_fixedByteBuffer_getSize_bytes(controlBytesIn) > 0) );
	cxa_assert(numBytesToReadIn > 0);

	// don't do anything if we're still busy
	if( i2cIn->isBusy )
	{
		if( cbIn != NULL ) cbIn(i2cIn, false, NULL, userVarIn);
		return;
	}

	// if we made it here, we weren't busy...now we are
	i2cIn->isBusy = true;
	i2cIn->cbs.readComplete = cbIn;
	i2cIn->cbs.userVar = userVarIn;

	// call our subclass method
	cxa_assert(i2cIn->scms.readBytesWithControlBytes);
	i2cIn->scms.readBytesWithControlBytes(i2cIn, addressIn, sendStopIn, controlBytesIn, numBytesToReadIn);
}


void cxa_i2cMaster_writeBytes(cxa_i2cMaster_t *const i2cIn,
							  uint8_t addressIn, uint8_t sendStopIn, cxa_fixedByteBuffer_t *const writeBytesIn,
							  cxa_i2cMaster_cb_onWriteComplete_t cbIn, void* userVarIn)
{
	cxa_assert(i2cIn);
	cxa_assert(writeBytesIn && (cxa_fixedByteBuffer_getSize_bytes(writeBytesIn) > 0) );

	// don't do anything if we're still busy
	if( i2cIn->isBusy )
	{
		if( cbIn != NULL ) cbIn(i2cIn, false, userVarIn);
		return;
	}

	// if we made it here, we weren't busy...now we are
	i2cIn->isBusy = true;
	i2cIn->cbs.writeComplete = cbIn;
	i2cIn->cbs.userVar = userVarIn;

	// call our subclass method
	cxa_assert(i2cIn->scms.readBytes);
	i2cIn->scms.writeBytes(i2cIn, addressIn, sendStopIn, writeBytesIn);
}


void cxa_i2cMaster_notify_readComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn)
{
	cxa_assert(i2cIn);

	if( !i2cIn->isBusy ) return;
	// we _must_ become unbusy so callee can continue i2c operations
	i2cIn->isBusy = false;

	// call our callback
	if( i2cIn->cbs.readComplete ) i2cIn->cbs.readComplete(i2cIn, wasSuccessfulIn, readBytesIn, i2cIn->cbs.userVar);
}


void cxa_i2cMaster_notify_writeComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn)
{
	cxa_assert(i2cIn);

	if( !i2cIn->isBusy ) return;
	// we _must_ become unbusy so callee can continue i2c operations
	i2cIn->isBusy = false;

	// call our callback
	if( i2cIn->cbs.writeComplete ) i2cIn->cbs.writeComplete(i2cIn, wasSuccessfulIn, i2cIn->cbs.userVar);
}


// ******** local function implementations ********
