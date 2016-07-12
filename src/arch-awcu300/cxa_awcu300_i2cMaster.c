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
#include "cxa_awcu300_i2cMaster.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>


// ******** local macro definitions ********
#define I2C_MOD_CLK_DIV 3
#define TIMEOUT_DEFAULT_MS			3000
#define INTERWRITE_DELAY_MS			25


// ******** local type definitions ********


// ******** local function prototypes ********
static void resetI2c(cxa_awcu300_i2cMaster_t *const i2cIn);
static bool writebytes(cxa_awcu300_i2cMaster_t *const i2cIn, uint8_t *bytesIn, size_t numBytesIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_awcu300_i2cMaster_init(cxa_awcu300_i2cMaster_t *const i2cIn, I2C_ID_Type i2cIdIn)
{
	cxa_assert(i2cIn);
	cxa_assert( (i2cIdIn == I2C0_PORT) ||
				(i2cIdIn == I2C1_PORT) );
	
	// save our references
	i2cIn->i2cId = i2cIdIn;
	cxa_timeDiff_init(&i2cIn->td_interWriteDelay, true);

	I2C_CFG_Type i2c_cfg =
			{
					.masterAddrBitMode = I2C_ADDR_BITS_7,
					.masterSlaveMode = I2C_MASTER,
					.restartEnable = ENABLE,
					.speedMode = I2C_SPEED_STANDARD
			};
	I2C_FIFO_Type i2c_fifo_cfg =
			{
					.recvFifoThr = 0,
					.transmitFifoThr = 1
			};

	// setup the pin muxes and enable clock to the i2c module
	switch( i2cIn->i2cId )
	{
		case I2C0_PORT:
			// configure our pin muxes
			GPIO_PinMuxFun(GPIO_4, GPIO4_I2C0_SDA);
			GPIO_PinMuxFun(GPIO_5, GPIO5_I2C0_SCL);

			// enable the clock to our module
			CLK_ModuleClkEnable(CLK_I2C0);
			CLK_ModuleClkDivider(CLK_I2C0, I2C_MOD_CLK_DIV);
			break;

		case I2C1_PORT:
			// configure our pin muxes
			GPIO_PinMuxFun(GPIO_25, GPIO25_I2C1_SDA);
			GPIO_PinMuxFun(GPIO_26, GPIO26_I2C1_SCL);

			// enable the clock to our module
			CLK_ModuleClkEnable(CLK_I2C1);
			CLK_ModuleClkDivider(CLK_I2C1, I2C_MOD_CLK_DIV);
			break;
	}

	// Configure the I2C port
	I2C_Disable(i2cIn->i2cId);
	I2C_Init(i2cIn->i2cId, &i2c_cfg);
	I2C_FIFOConfig(i2cIn->i2cId, &i2c_fifo_cfg);
	I2C_Enable(i2cIn->i2cId);
}


bool cxa_i2cMaster_writeBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn,
							  uint8_t* ctrlBytesIn, size_t numCtrlBytesIn,
							  uint8_t* dataBytesIn, size_t numDataBytesIn)
{
	cxa_awcu300_i2cMaster_t* i2cIn = (cxa_awcu300_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);
	if( numCtrlBytesIn > 0 ) cxa_assert(ctrlBytesIn);
	if( numDataBytesIn > 0 ) cxa_assert(dataBytesIn);
	
	while( !cxa_timeDiff_isElapsed_ms(&i2cIn->td_interWriteDelay, INTERWRITE_DELAY_MS) );

	// set our target slave address
	I2C_SetSlaveAddr(i2cIn->i2cId, addressIn);

	// make sure we weren't performing another transaction
	cxa_timeDiff_t td_timeout;
	cxa_timeDiff_setStartTime_now(&td_timeout);
	while( I2C_GetStatus(i2cIn->i2cId, I2C_STATUS_ACTIVITY) == SET )
	{
		if( cxa_timeDiff_isElapsed_ms(&td_timeout, TIMEOUT_DEFAULT_MS) ) return false;
	}

	// queue up our "control" bytes (if they exist)
	if( !writebytes(i2cIn, ctrlBytesIn, numCtrlBytesIn) ) return false;

	// queue up our control bytes
	if( !writebytes(i2cIn, dataBytesIn, numDataBytesIn) ) return false;

	// wait for our transaction to complete
	cxa_timeDiff_setStartTime_now(&td_timeout);
	while( I2C_GetStatus(i2cIn->i2cId, I2C_STATUS_ACTIVITY) == SET )
	{
		if( cxa_timeDiff_isElapsed_ms(&td_timeout, TIMEOUT_DEFAULT_MS) ) return false;
	}

	// make sure we don't write too fast...
	cxa_timeDiff_setStartTime_now(&i2cIn->td_interWriteDelay);

	return true;
}


bool cxa_i2cMaster_readBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn,
							 uint8_t* ctrlBytesIn, size_t numCtrlBytesIn,
							 uint8_t* dataBytesOut, size_t numDataBytesIn)
{
	cxa_awcu300_i2cMaster_t* i2cIn = (cxa_awcu300_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);
	if( numCtrlBytesIn > 0 ) cxa_assert(ctrlBytesIn);

	// set our target slave address
	I2C_SetSlaveAddr(i2cIn->i2cId, addressIn);

	// make sure we weren't performing another transaction
	cxa_timeDiff_t td_timeout;
	cxa_timeDiff_setStartTime_now(&td_timeout);
	while( I2C_GetStatus(i2cIn->i2cId, I2C_STATUS_ACTIVITY) == SET )
	{
		if( cxa_timeDiff_isElapsed_ms(&td_timeout, TIMEOUT_DEFAULT_MS) ) return false;
	}

	// queue up our "control" bytes (if they exist)
	if( !writebytes(i2cIn, ctrlBytesIn, numCtrlBytesIn) ) return false;

	// now queue up our reads (and store bytes if we get them)
	size_t numBytesRead = 0;
	for( size_t i = 0; i < numDataBytesIn; i++ )
	{
		cxa_timeDiff_setStartTime_now(&td_timeout);
		while( I2C_GetStatus(i2cIn->i2cId, I2C_STATUS_TFNF) != SET )
		if( cxa_timeDiff_isElapsed_ms(&td_timeout, TIMEOUT_DEFAULT_MS) )
		{
			resetI2c(i2cIn);
			return false;
		}
		I2C_MasterReadCmd(i2cIn->i2cId);

		if( I2C_GetStatus(i2cIn->i2cId, I2C_STATUS_RFNE) == SET )
		{
			uint8_t rxByte = I2C_ReceiveByte(i2cIn->i2cId);
			if( dataBytesOut != NULL ) dataBytesOut[numBytesRead] = rxByte;
			numBytesRead++;
		}
	}

	// wait for the rest of our data
	cxa_timeDiff_setStartTime_now(&td_timeout);
	while( numBytesRead < numDataBytesIn )
	{
		// if we become inactive, we didn't get enough data bytes back
//		if( I2C_GetStatus(i2cIn->i2cId, I2C_STATUS_ACTIVITY) != SET )
//		{
//			cxa_logger_trace(cxa_logger_getSysLog(), "d");
//			return false;
//		}

		// make sure we haven't timed out
		if( cxa_timeDiff_isElapsed_ms(&td_timeout, TIMEOUT_DEFAULT_MS) )
		{
			resetI2c(i2cIn);
			return false;
		}

		if( I2C_GetStatus(i2cIn->i2cId, I2C_STATUS_RFNE) == SET )
		{
			uint8_t rxByte = I2C_ReceiveByte(i2cIn->i2cId);
			if( dataBytesOut != NULL ) dataBytesOut[numBytesRead] = rxByte;
			numBytesRead++;

			// reset our timeout
			cxa_timeDiff_setStartTime_now(&td_timeout);
		}
	}

	return true;
}


// ******** local function implementations ********
static void resetI2c(cxa_awcu300_i2cMaster_t *const i2cIn)
{
	cxa_assert(i2cIn);

	I2C_Disable(i2cIn->i2cId);
	I2C_Enable(i2cIn->i2cId);
}


static bool writebytes(cxa_awcu300_i2cMaster_t *const i2cIn, uint8_t *bytesIn, size_t numBytesIn)
{
	cxa_assert(i2cIn);
	if( numBytesIn > 0 ) cxa_assert(bytesIn);

	// queue up our control bytes
	cxa_timeDiff_t td_timeout;
	for( size_t i = 0 ; i < numBytesIn; i++ )
	{
		cxa_timeDiff_setStartTime_now(&td_timeout);
		while( I2C_GetStatus(i2cIn->i2cId, I2C_STATUS_TFNF) != SET )
		{
			if( cxa_timeDiff_isElapsed_ms(&td_timeout, TIMEOUT_DEFAULT_MS) )
			{
				resetI2c(i2cIn);
				return false;
			}
		}
		I2C_SendByte(i2cIn->i2cId, bytesIn[i]);
	}

	return true;
}

