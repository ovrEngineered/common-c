/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_bgm_i2cMaster.h"


// ******** includes ********
#include <bsp.h>
#include <i2cspm.h>
#include <stdio.h>

#include <cxa_assert.h>
#include <cxa_delay.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define BUS_RESET_TOGGLE_PERIOD_MS			1


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_readBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, size_t numBytesToReadIn);
static void scm_readBytesWithControlBytes(cxa_i2cMaster_t *const i2cIn,
		uint8_t addressIn, uint8_t sendStopIn,
		cxa_fixedByteBuffer_t *const controlBytesIn,
		size_t numBytesToReadIn);
static void scm_writeBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, cxa_fixedByteBuffer_t *const writeBytesIn);
static void scm_resetBus(cxa_i2cMaster_t *const superIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_bgm_i2cMaster_init(cxa_bgm_i2cMaster_t *const i2cIn, I2C_TypeDef *const i2cPortIn,
		const GPIO_Port_TypeDef sdaPortNumIn, const unsigned int sdaPinNumIn, uint32_t sdaLocIn,
		const GPIO_Port_TypeDef sclPortNumIn, const unsigned int sclPinNumIn, uint32_t sclLocIn)
{
	cxa_assert(i2cIn);

	// save our references
	i2cIn->i2cPort = i2cPortIn;
	i2cIn->sda.port = sdaPortNumIn;
	i2cIn->sda.pinNum = sdaPinNumIn;
	i2cIn->sda.loc = sdaLocIn;
	i2cIn->scl.port = sclPortNumIn;
	i2cIn->scl.pinNum = sclPinNumIn;
	i2cIn->scl.loc = sclLocIn;

	// enable the clock to the peripheral
#if defined(_SILICON_LABS_32B_SERIES_2)
	if( i2cIn->i2cPort == I2C0 )
	{
		CMU_ClockEnable(cmuClock_I2C0, true);
	}
#ifdef I2C1
	else if( i2cIn->i2cPort == I2C1 )
	{
		CMU_ClockEnable(cmuClock_I2C1, true);
	}
#endif
#endif

	// make sure the pins are setup correctly
	GPIO_PinModeSet(i2cIn->sda.port, i2cIn->sda.pinNum, gpioModeWiredAndPullUpFilter, 1);
	GPIO_PinModeSet(i2cIn->scl.port, i2cIn->scl.pinNum, gpioModeWiredAndPullUpFilter, 1);

	// route the peripheral to the pins
	i2cIn->i2cPort->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
	i2cIn->i2cPort->ROUTELOC0 = (i2cIn->i2cPort->ROUTELOC0 & (~_I2C_ROUTELOC0_SDALOC_MASK)) | i2cIn->sda.loc;
	i2cIn->i2cPort->ROUTELOC0 = (i2cIn->i2cPort->ROUTELOC0 & (~_I2C_ROUTELOC0_SCLLOC_MASK)) | i2cIn->scl.loc;

	// initialize our hardware
	I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
	i2cInit.freq = I2C_FREQ_STANDARD_MAX;
	I2C_Init(I2C0, &i2cInit);

	i2cIn->i2cPort->CTRL = I2C_CTRL_EN;

	// initialize our super class
	cxa_i2cMaster_init(&i2cIn->super, scm_readBytes, scm_readBytesWithControlBytes, scm_writeBytes, scm_resetBus);
}


// ******** local function implementations ********
static void scm_readBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, size_t numBytesToReadIn)
{
	cxa_bgm_i2cMaster_t* i2cIn = (cxa_bgm_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	// handle an edge case
	if( numBytesToReadIn == 0 )
	{
		cxa_i2cMaster_notify_readComplete(&i2cIn->super, true, NULL);
		return;
	}
	// if we made it here, we're actually reading data

	I2C_TransferSeq_TypeDef    seq;
	seq.addr  = addressIn;
	seq.flags = I2C_FLAG_READ;

	// read buffer
	uint8_t fbb_readBytes_raw[numBytesToReadIn];
	seq.buf[0].data = fbb_readBytes_raw;
	seq.buf[0].len  = numBytesToReadIn;

	// nothing else
	seq.buf[1].len  = 0;

	// commit the read to the bus
	I2C_TransferReturn_TypeDef ret = I2CSPM_Transfer(i2cIn->i2cPort, &seq);

	// create our return buffer
	cxa_fixedByteBuffer_t fbb_readBytes;
	cxa_fixedByteBuffer_init_inPlace(&fbb_readBytes, numBytesToReadIn, fbb_readBytes_raw, sizeof(fbb_readBytes_raw));

	// return the read values
	cxa_i2cMaster_notify_readComplete(&i2cIn->super, (ret == i2cTransferDone), (ret == i2cTransferDone) ? &fbb_readBytes : NULL);
}


static void scm_readBytesWithControlBytes(cxa_i2cMaster_t *const superIn,
		uint8_t addressIn, uint8_t sendStopIn,
		cxa_fixedByteBuffer_t *const controlBytesIn,
		size_t numBytesToReadIn)
{
	cxa_bgm_i2cMaster_t* i2cIn = (cxa_bgm_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	// handle an edge case
	if( numBytesToReadIn == 0 )
	{
		cxa_i2cMaster_notify_readComplete(&i2cIn->super, true, NULL);
		return;
	}
	// if we made it here, we're actually reading data

	I2C_TransferSeq_TypeDef seq;
	seq.addr  = addressIn << 1;
	seq.flags = I2C_FLAG_WRITE_READ;

	// control bytes first
	seq.buf[0].data = (controlBytesIn != NULL) ? cxa_fixedByteBuffer_get_pointerToIndex(controlBytesIn, 0) : NULL;
	seq.buf[0].len  = (controlBytesIn != NULL) ? cxa_fixedByteBuffer_getSize_bytes(controlBytesIn) : 0;

	// read buffer
	uint8_t fbb_readBytes_raw[numBytesToReadIn];
	seq.buf[1].data = fbb_readBytes_raw;
	seq.buf[1].len  = numBytesToReadIn;

	// commit the read to the bus
	I2C_TransferReturn_TypeDef ret = I2CSPM_Transfer(i2cIn->i2cPort, &seq);

	// create our return buffer
	cxa_fixedByteBuffer_t fbb_readBytes;
	cxa_fixedByteBuffer_init_inPlace(&fbb_readBytes, numBytesToReadIn, fbb_readBytes_raw, sizeof(fbb_readBytes_raw));

	// return the read values
	cxa_i2cMaster_notify_readComplete(&i2cIn->super, (ret == i2cTransferDone), (ret == i2cTransferDone) ? &fbb_readBytes : NULL);
}


static void scm_writeBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, cxa_fixedByteBuffer_t *const writeBytesIn)
{
	cxa_bgm_i2cMaster_t* i2cIn = (cxa_bgm_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	// handle an edge case
	if( (writeBytesIn == NULL) || (cxa_fixedByteBuffer_getSize_bytes(writeBytesIn) == 0) )
	{
		cxa_i2cMaster_notify_writeComplete(&i2cIn->super, true);
		return;
	}
	// if we made it here, we're actually writing data

	I2C_TransferSeq_TypeDef    seq;
	seq.addr  = addressIn << 1;
	seq.flags = I2C_FLAG_WRITE;

	// control bytes first
	seq.buf[0].data = cxa_fixedByteBuffer_get_pointerToIndex(writeBytesIn, 0);
	seq.buf[0].len  = cxa_fixedByteBuffer_getSize_bytes(writeBytesIn);

	// nothing else
	seq.buf[1].len  = 0;

	// commit the read to the bus
	I2C_TransferReturn_TypeDef ret = I2CSPM_Transfer(i2cIn->i2cPort, &seq);

	// return our success or failure
	cxa_i2cMaster_notify_writeComplete(&i2cIn->super, (ret == i2cTransferDone));
}


static void scm_resetBus(cxa_i2cMaster_t *const superIn)
{
	cxa_bgm_i2cMaster_t* i2cIn = (cxa_bgm_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	// disconnect the peripheral from the pins
	i2cIn->i2cPort->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
	i2cIn->i2cPort->ROUTELOC0 = (i2cIn->i2cPort->ROUTELOC0 & (~_I2C_ROUTELOC0_SDALOC_MASK));
	i2cIn->i2cPort->ROUTELOC0 = (i2cIn->i2cPort->ROUTELOC0 & (~_I2C_ROUTELOC0_SCLLOC_MASK));

	// configure for output mode so we can manually clock
	GPIO_PinModeSet(i2cIn->sda.port, i2cIn->sda.pinNum, gpioModeInputPull, 1);
	GPIO_PinModeSet(i2cIn->scl.port, i2cIn->scl.pinNum, gpioModeWiredAndPullUpFilter, 1);

	// cycle up to 9 times...per I2C specifications
	for( int i = 0; i < 9; i++ )
	{
		cxa_delay_ms(BUS_RESET_TOGGLE_PERIOD_MS);
		GPIO_PinOutClear(i2cIn->scl.port, i2cIn->scl.pinNum);
		cxa_delay_ms(BUS_RESET_TOGGLE_PERIOD_MS);
		GPIO_PinOutSet(i2cIn->scl.port, i2cIn->scl.pinNum);
		cxa_delay_ms(BUS_RESET_TOGGLE_PERIOD_MS);

		// check to see if it's been released
		if( GPIO_PinInGet(i2cIn->sda.port, i2cIn->sda.pinNum) ) break;
	}

	// reconfigure pins for for i2c
	GPIO_PinModeSet(i2cIn->sda.port, i2cIn->sda.pinNum, gpioModeWiredAndPullUpFilter, 1);
	GPIO_PinModeSet(i2cIn->scl.port, i2cIn->scl.pinNum, gpioModeWiredAndPullUpFilter, 1);

	// reconnect the peripheral to the pins
	i2cIn->i2cPort->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
	i2cIn->i2cPort->ROUTELOC0 = (i2cIn->i2cPort->ROUTELOC0 & (~_I2C_ROUTELOC0_SDALOC_MASK)) | i2cIn->sda.loc;
	i2cIn->i2cPort->ROUTELOC0 = (i2cIn->i2cPort->ROUTELOC0 & (~_I2C_ROUTELOC0_SCLLOC_MASK)) | i2cIn->scl.loc;

	cxa_delay_ms(500);
}
