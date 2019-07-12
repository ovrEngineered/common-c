/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_ltc2942.h"


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>


#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define I2C_ADDR_7BIT				0x64
#define M_PRESCALER 				64UL


// ******** local type definitions ********


// ******** local function prototypes ********
static void i2cCb_onWriteComplete_shutdownForConfigure(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn);
static void i2cCb_onWriteComplete_setAccumulatedChargeReg(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn);
static void i2cCb_onWriteComplete_startTrackingBattery(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn);

static void i2cCb_onReadComplete_remainingCapacity(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ltc2942_init(cxa_ltc2942_t *const ltcIn, cxa_i2cMaster_t *const i2cIn, uint16_t batteryInitCap_mahIn)
{
	cxa_assert(ltcIn);
	cxa_assert(i2cIn);

	// save our references
	ltcIn->i2c = i2cIn;
	ltcIn->batteryInitCap_mah = batteryInitCap_mahIn;

	cxa_logger_init(&ltcIn->logger, "ltc-2942");

	cxa_array_initStd(&ltcIn->listeners, ltcIn->listeners_raw);

	ltcIn->isInitialized = false;
}


void cxa_ltc2942_addListener(cxa_ltc2942_t *const ltcIn, cxa_ltc2942_cb_startup_t cb_onStartupIn, cxa_ltc2942_cb_updatedValue_t cb_onUpdatedValue, void *userVarIn)
{
	cxa_assert(ltcIn);

	cxa_ltc2942_listener_t newListener = {
			.cb_onStartup = cb_onStartupIn,
			.cb_onUpdatedValue = cb_onUpdatedValue,
			.userVar = userVarIn
	};

	cxa_assert(cxa_array_append(&ltcIn->listeners, &newListener));
}


void cxa_ltc2942_start(cxa_ltc2942_t *const ltcIn)
{
	cxa_assert(ltcIn);

	// setup a temporary buffer
	cxa_fixedByteBuffer_t fbb_tmp;
	uint8_t fbb_tmp_raw[2];
	cxa_fixedByteBuffer_initStd(&fbb_tmp, fbb_tmp_raw);

	//  shutdown the analog section to configure, set ADC mode, prescaler, AL/CC pin
	cxa_fixedByteBuffer_clear(&fbb_tmp);
	cxa_fixedByteBuffer_append_uint8(&fbb_tmp, 0x01);
	cxa_fixedByteBuffer_append_uint8(&fbb_tmp, 0x31);
	cxa_logger_trace(&ltcIn->logger, "shutting down adc");
	cxa_i2cMaster_writeBytes(ltcIn->i2c, I2C_ADDR_7BIT, true, &fbb_tmp, i2cCb_onWriteComplete_shutdownForConfigure, (void*)ltcIn);
}


void cxa_ltc2942_requestRemainingCapacityNow(cxa_ltc2942_t *const ltcIn)
{
	cxa_assert(ltcIn);

	// make sure we're initialized
	if( !ltcIn->isInitialized )
	{
		// notify our listeners
		cxa_array_iterate(&ltcIn->listeners, currListener, cxa_ltc2942_listener_t)
		{
			if( currListener == NULL ) continue;
			if( currListener->cb_onUpdatedValue != NULL) currListener->cb_onUpdatedValue(false, 0, currListener->userVar);
		}
		return;
	}
	// if we made it here, we've been initialized

	// start the read
	cxa_fixedByteBuffer_t fbb_controlBytes;
	uint8_t fbb_controlBytes_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_controlBytes, fbb_controlBytes_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_controlBytes, 0x02);

	cxa_i2cMaster_readBytes_withControlBytes(ltcIn->i2c, I2C_ADDR_7BIT, true, &fbb_controlBytes, 2, i2cCb_onReadComplete_remainingCapacity, (void*)ltcIn);
}


// ******** local function implementations ********
static void i2cCb_onWriteComplete_shutdownForConfigure(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn)
{
	cxa_ltc2942_t *const ltcIn = (cxa_ltc2942_t *const)userVarIn;
	cxa_assert(ltcIn);

	cxa_logger_trace(&ltcIn->logger, "adc shutdown complete: %d", wasSuccessfulIn);

	if( !wasSuccessfulIn )
	{
		cxa_logger_warn(&ltcIn->logger, "adc shutdown failed");

		// notify our listeners
		cxa_array_iterate(&ltcIn->listeners, currListener, cxa_ltc2942_listener_t)
		{
			if( currListener == NULL ) continue;
			if( currListener->cb_onStartup != NULL) currListener->cb_onStartup(false, currListener->userVar);
		}
		return;
	}
	// if we made it here, the sensor is responding correctly...

	// setup a temporary buffer
	cxa_fixedByteBuffer_t fbb_tmp;
	uint8_t fbb_tmp_raw[3];
	cxa_fixedByteBuffer_initStd(&fbb_tmp, fbb_tmp_raw);

	// set the accumulated charge register to our starting battery capacity
	uint16_t initAccumCharge = (((uint32_t)ltcIn->batteryInitCap_mah) * 1505UL / M_PRESCALER);
	cxa_fixedByteBuffer_append_uint8(&fbb_tmp, 0x02);					// internal address of accumulated charge register
	cxa_fixedByteBuffer_append_uint16BE(&fbb_tmp, initAccumCharge);
	cxa_logger_trace(&ltcIn->logger, "setting accumulated charge register to 0x%04X", initAccumCharge);
	cxa_i2cMaster_writeBytes(ltcIn->i2c, I2C_ADDR_7BIT, true, &fbb_tmp, i2cCb_onWriteComplete_setAccumulatedChargeReg, (void*)ltcIn);
}


static void i2cCb_onWriteComplete_setAccumulatedChargeReg(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn)
{
	cxa_ltc2942_t *const ltcIn = (cxa_ltc2942_t *const)userVarIn;
	cxa_assert(ltcIn);

	cxa_logger_trace(&ltcIn->logger, "accumulated charge register set complete: %d", wasSuccessfulIn);

	if( !wasSuccessfulIn )
	{
		cxa_logger_warn(&ltcIn->logger, "accumulated charge register set failed");

		// notify our listeners
		cxa_array_iterate(&ltcIn->listeners, currListener, cxa_ltc2942_listener_t)
		{
			if( currListener == NULL ) continue;
			if( currListener->cb_onStartup != NULL) currListener->cb_onStartup(false, currListener->userVar);
		}
		return;
	}
	// if we made it here, the sensor is responding correctly...

	// setup a temporary buffer
	cxa_fixedByteBuffer_t fbb_tmp;
	uint8_t fbb_tmp_raw[2];
	cxa_fixedByteBuffer_initStd(&fbb_tmp, fbb_tmp_raw);

	//  power up the analog section and start tracking battery level
	cxa_fixedByteBuffer_clear(&fbb_tmp);
	cxa_fixedByteBuffer_append_uint8(&fbb_tmp, 0x01);
	cxa_fixedByteBuffer_append_uint8(&fbb_tmp, 0x30);
	cxa_logger_trace(&ltcIn->logger, "powering up adc");
	cxa_i2cMaster_writeBytes(ltcIn->i2c, I2C_ADDR_7BIT, true, &fbb_tmp, i2cCb_onWriteComplete_startTrackingBattery, (void*)ltcIn);
}


static void i2cCb_onWriteComplete_startTrackingBattery(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn)
{
	cxa_ltc2942_t *const ltcIn = (cxa_ltc2942_t *const)userVarIn;
	cxa_assert(ltcIn);

	cxa_logger_trace(&ltcIn->logger, "adc power-up complete: %d", wasSuccessfulIn);

	if( !wasSuccessfulIn )
	{
		cxa_logger_warn(&ltcIn->logger, "adc power-up failed");

		// notify our listeners
		cxa_array_iterate(&ltcIn->listeners, currListener, cxa_ltc2942_listener_t)
		{
			if( currListener == NULL ) continue;
			if( currListener->cb_onStartup != NULL) currListener->cb_onStartup(false, currListener->userVar);
		}
		return;
	}
	// if we made it here, the sensor is responding correctly...

	ltcIn->isInitialized = true;

	// notify our listeners
	cxa_array_iterate(&ltcIn->listeners, currListener, cxa_ltc2942_listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onStartup != NULL) currListener->cb_onStartup(true, currListener->userVar);
	}
}


static void i2cCb_onReadComplete_remainingCapacity(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn)
{
	cxa_ltc2942_t *const ltcIn = (cxa_ltc2942_t *const)userVarIn;
	cxa_assert(ltcIn);

	uint16_t remainingCapacity_mah = 0;
	if( wasSuccessfulIn )
	{
		uint16_t remainingCapacity_raw;
		cxa_fixedByteBuffer_get_uint16BE(readBytesIn, 0, remainingCapacity_raw);
		remainingCapacity_mah = ((uint32_t)remainingCapacity_raw) * M_PRESCALER / 1505UL;

		cxa_logger_trace(&ltcIn->logger, "read 0x%04X  %d mAh", remainingCapacity_raw, remainingCapacity_mah);
	}
	else
	{
		cxa_logger_warn(&ltcIn->logger, "read failed");
	}

	// notify our listeners
	cxa_array_iterate(&ltcIn->listeners, currListener, cxa_ltc2942_listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onUpdatedValue != NULL) currListener->cb_onUpdatedValue(wasSuccessfulIn, remainingCapacity_mah, currListener->userVar);
	}
}
