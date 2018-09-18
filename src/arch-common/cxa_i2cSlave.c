/**
 * @copyright 2018 opencxa.org
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
#include "cxa_i2cSlave.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_i2cSlave_init(cxa_i2cSlave_t *const i2cIn)
{
	cxa_assert(i2cIn);

	cxa_array_initStd(&i2cIn->commands, i2cIn->commands_raw);

	cxa_logger_init(&i2cIn->logger, "i2cSlave");
}


void cxa_i2cSlave_addCommand(cxa_i2cSlave_t *const i2cIn, const uint8_t commandIn,
							 cxa_i2cSlave_cb_commandRead_t cb_readIn,
							 cxa_i2cSlave_cb_commandWrite_t cb_writeIn,
							 const ssize_t expectedNumBytesIn,
							 void *const userVarIn)
{
	cxa_assert(i2cIn);

	cxa_i2cSlave_commandEntry_t newEntry = {
			.command = commandIn,
			.cb_read = cb_readIn,
			.cb_write = cb_writeIn,
			.expectedNumBytes = expectedNumBytesIn,
			.userVar = userVarIn
	};
	cxa_assert_msg(cxa_array_append(&i2cIn->commands, &newEntry), "increase CXA_I2CSLAVE_MAXNUM_COMMANDS");
}


ssize_t cxa_i2cSlave_validatePreWrite(cxa_i2cSlave_t *const i2cIn, const uint8_t commandIn)
{
	cxa_assert(i2cIn);

	ssize_t retVal = -1;
	cxa_array_iterate(&i2cIn->commands, currEntry, cxa_i2cSlave_commandEntry_t)
	{
		if( (currEntry != NULL) &&
			(currEntry->command == commandIn) &&
			(currEntry->cb_write != NULL) )
		{
			retVal = currEntry->expectedNumBytes;
			break;
		}
	}
	return retVal;
}


bool cxa_i2cSlave_notifyRead(cxa_i2cSlave_t *const i2cIn, const uint8_t commandIn, cxa_fixedByteBuffer_t *const bytesOut)
{
	cxa_assert(i2cIn);

	bool retVal = false;
	cxa_array_iterate(&i2cIn->commands, currEntry, cxa_i2cSlave_commandEntry_t)
	{
		if( (currEntry != NULL) && (currEntry->command == commandIn) && (currEntry->cb_read != NULL) )
		{
			retVal = currEntry->cb_read(bytesOut, currEntry->userVar);
			break;
		}
	}
	return retVal;
}


void cxa_i2cSlave_notifyWrite(cxa_i2cSlave_t *const i2cIn, const uint8_t commandIn, cxa_fixedByteBuffer_t *const bytesIn)
{
	cxa_assert(i2cIn);

	cxa_array_iterate(&i2cIn->commands, currEntry, cxa_i2cSlave_commandEntry_t)
	{
		if( (currEntry != NULL) &&
			(currEntry->command == commandIn) &&
			(currEntry->cb_write != NULL) &&
			(cxa_fixedByteBuffer_getSize_bytes(bytesIn) == currEntry->expectedNumBytes) )
		{
			currEntry->cb_write(bytesIn, currEntry->userVar);
			break;
		}
	}
}


// ******** local function implementations ********
