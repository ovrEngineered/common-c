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
#include "cxa_siLabsBgApi_module.h"


// ******** includes ********
#include <gecko_bglib.h>

#include <cxa_assert.h>
#include <cxa_ioStream_peekable.h>
#include <cxa_siLabsBgApi_btle_central.h>
#include <cxa_siLabsBgApi_btle_peripheral.h>
#include <cxa_stateMachine.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
BGLIB_DEFINE();							// needs to be defined to use bglib functions

#define WAIT_BOOT_TIME_MS				4000

#define ADVERT_PERIOD_SLOW_MS			1000
#define ADVERT_WINDOW_SLOW_MS			250


// ******** local type definitions ********
typedef enum
{
	RADIOSTATE_RESET,
	RADIOSTATE_WAIT_BOOT,
	RADIOSTATE_READY,
}radioState_t;


// ******** local function prototypes ********
static void appHandleEvents(struct gecko_cmd_packet *evt);

static void stateCb_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitForBoot_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitBoot_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_ready_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static void stateCb_xxx_state(cxa_stateMachine_t *const smIn, void *userVarIn);

static void bglib_cb_output(uint32_t numBytesIn, uint8_t* dataIn);
static int32_t bglib_cb_input(uint32_t numBytesToReadIn, uint8_t* dataOut);
static int32_t bglib_cb_peek(void);


// ********  local variable declarations *********
static cxa_ioStream_peekable_t ios_usart;
static bool hasBootFailed;

static cxa_siLabsBgApi_btle_central_t btlec;
static cxa_siLabsBgApi_btle_peripheral_t btlep;

static cxa_logger_t logger;
static cxa_stateMachine_t stateMachine;


// ******** global function implementations ********
void cxa_siLabsBgApi_module_init(cxa_ioStream_t *const ioStreamIn, int threadIdIn)
{
	cxa_assert(ioStreamIn);

	// save our references and setup our internal state
	cxa_ioStream_peekable_init(&ios_usart, ioStreamIn);
	hasBootFailed = false;
	cxa_logger_init(&logger, "bgApiModule");

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "bgApiBtleC", threadIdIn);
	cxa_stateMachine_addState(&stateMachine, RADIOSTATE_RESET, "reset", stateCb_reset_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState_timed(&stateMachine, RADIOSTATE_WAIT_BOOT, "reset", RADIOSTATE_RESET, WAIT_BOOT_TIME_MS, stateCb_waitForBoot_enter, stateCb_xxx_state, stateCb_waitBoot_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, RADIOSTATE_READY, "ready", stateCb_ready_enter, stateCb_xxx_state, NULL, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, RADIOSTATE_RESET);

	// setup our BGLib
	BGLIB_INITIALIZE_NONBLOCK(bglib_cb_output, bglib_cb_input, bglib_cb_peek);

	// setup our btle client and peripheral
	cxa_siLabsBgApi_btle_central_init(&btlec, threadIdIn);
	cxa_siLabsBgApi_btle_peripheral_init(&btlep, threadIdIn);
}


cxa_siLabsBgApi_btle_central_t* cxa_siLabsBgApi_module_getBtleCentral(void)
{
	return &btlec;
}


cxa_siLabsBgApi_btle_peripheral_t* cxa_siLabsBgApi_module_getBtlePeripheral(void)
{
	return &btlep;
}


cxa_btle_central_state_t cxa_siLabsBgApi_module_getState(void)
{
	cxa_btle_central_state_t retVal = CXA_BTLE_CENTRAL_STATE_STARTUP;
	switch( cxa_stateMachine_getCurrentState(&stateMachine) )
	{
		case RADIOSTATE_RESET:
		case RADIOSTATE_WAIT_BOOT:
			retVal = hasBootFailed ? CXA_BTLE_CENTRAL_STATE_STARTUPFAILED : CXA_BTLE_CENTRAL_STATE_STARTUP;
			break;

		case RADIOSTATE_READY:
			retVal = CXA_BTLE_CENTRAL_STATE_READY;
			break;
	}
	return retVal;
}


// ******** local function implementations ********
static void appHandleEvents(struct gecko_cmd_packet *evt)
{
	if( NULL == evt ) return;

	radioState_t currConnState = cxa_stateMachine_getCurrentState(&stateMachine);

	cxa_logger_trace(&logger, "event: 0x%08X", BGLIB_MSG_ID(evt->header));

	// Handle events
	switch( BGLIB_MSG_ID(evt->header) )
	{
		case gecko_evt_system_boot_id:
		{
			if( currConnState == RADIOSTATE_WAIT_BOOT )
			{
				cxa_logger_debug(&logger, "radio booted");
			}
			else
			{
				cxa_logger_warn(&logger, "unexpected radio boot");
			}
			cxa_stateMachine_transition(&stateMachine, RADIOSTATE_READY);
			break;
		}

		case gecko_evt_le_connection_opened_id:
		case gecko_evt_le_connection_closed_id:
		case gecko_evt_gatt_procedure_completed_id:
		case gecko_evt_gatt_service_id:
		case gecko_evt_gatt_characteristic_id:
		case gecko_evt_gatt_characteristic_value_id:
		case gecko_evt_le_gap_scan_response_id:
			if( !cxa_siLabsBgApi_btle_central_handleBgEvent(&btlec, evt) ) cxa_siLabsBgApi_btle_peripheral_handleBgEvent(&btlep, evt);
			break;

		case gecko_evt_gatt_server_user_read_request_id:
		case gecko_evt_gatt_server_user_write_request_id:
			cxa_siLabsBgApi_btle_peripheral_handleBgEvent(&btlep, evt);
			break;

		case gecko_evt_le_connection_parameters_id:
		case gecko_evt_le_connection_phy_status_id:
		case gecko_evt_gatt_mtu_exchanged_id:
			break;

		default:
			cxa_logger_debug(&logger, "unhandled event: 0x%08X", BGLIB_MSG_ID(evt->header));
			break;
	}
}


static void stateCb_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "resetting radio");

	// tell the radio to reboot
	gecko_cmd_system_reset(0);

	// start waiting for the boot event
	cxa_stateMachine_transition(&stateMachine, RADIOSTATE_WAIT_BOOT);
}


static void stateCb_waitForBoot_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_debug(&logger, "waiting for radio boot");
}


static void stateCb_waitBoot_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	// failed to receive our boot message
	if( nextStateIdIn == RADIOSTATE_RESET )
	{
		hasBootFailed = true;
		cxa_btle_central_notify_onFailedInit(&btlec.super, true);
		cxa_btle_peripheral_notify_onFailedInit(&btlep.super, true);
	}
	else
	{
		// booted successfully
		hasBootFailed = false;
	}
}


static void stateCb_ready_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "radio is ready");

	cxa_btle_central_notify_onBecomesReady(&btlec.super);
	cxa_btle_peripheral_notify_onBecomesReady(&btlep.super);
}


static void stateCb_xxx_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	appHandleEvents(gecko_peek_event());
}


static void bglib_cb_output(uint32_t numBytesIn, uint8_t* dataIn)
{
	cxa_logger_trace_memDump(&logger, "write to bgm121: ", dataIn, numBytesIn, NULL);
	cxa_ioStream_writeBytes(&ios_usart.super, dataIn, numBytesIn);
}


static int32_t bglib_cb_input(uint32_t numBytesToReadIn, uint8_t* dataOut)
{
	uint8_t rxByte;

	cxa_logger_trace(&logger, "waiting for %d bytes", numBytesToReadIn);
	for( uint32_t numBytesRead = 0; numBytesRead < numBytesToReadIn; numBytesRead++ )
	{
		while(1)
		{
			cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(&ios_usart.super, &rxByte);
			if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
			{
				// got our byte...continue
				dataOut[numBytesRead] = rxByte;
				break;
			}
			else if( readStat == CXA_IOSTREAM_READSTAT_ERROR )
			{
				// return non-zero on failure
				cxa_logger_error(&logger, "error during read");
				return 1;
			}
		}
	}
	cxa_logger_trace(&logger, "read complete (%d bytes)", numBytesToReadIn);

	// return 0 on success
	return 0;
}


static int32_t bglib_cb_peek(void)
{
	return cxa_ioStream_peekable_hasBytesAvailable(&ios_usart);
}
