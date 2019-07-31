/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_siLabsBgApi_module.h"


// ******** includes ********
#ifndef CXA_SILABSBGAPI_MODE_SOC
#include <gecko_bglib.h>
#else
#include "bg_types.h"
#include "native_gecko.h"
#include "infrastructure.h"
#endif

#include <cxa_assert.h>
#include <cxa_ioStream_peekable.h>
#include <cxa_siLabsBgApi_btle_central.h>
#include <cxa_siLabsBgApi_btle_peripheral.h>
#include <cxa_runLoop.h>
#include <cxa_stateMachine.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#ifndef CXA_SILABSBGAPI_MODE_SOC
BGLIB_DEFINE();							// needs to be defined to use bglib functions
#endif

#ifndef CXA_SILABSBGAPI_MAX_NUM_TIMERS
#define CXA_SILABSBGAPI_MAX_NUM_TIMERS				4
#endif


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


typedef struct
{
	bool isUsed;

	cxa_siLabsBgApi_cb_onTimer_t cb;
	void* userVarIn;
}timerCallbackEntry_t;


// ******** local function prototypes ********
static void appHandleEvents(struct gecko_cmd_packet *evt);

#ifndef CXA_SILABSBGAPI_MODE_SOC
static void stateCb_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
#endif
static void stateCb_waitForBoot_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitBoot_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_ready_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static void stateCb_xxx_state(cxa_stateMachine_t *const smIn, void *userVarIn);

#ifndef CXA_SILABSBGAPI_MODE_SOC
static void bglib_cb_output(uint32_t numBytesIn, uint8_t* dataIn);
static int32_t bglib_cb_input(uint32_t numBytesToReadIn, uint8_t* dataOut);
static int32_t bglib_cb_peek(void);
#endif


// ********  local variable declarations *********
#ifndef CXA_SILABSBGAPI_MODE_SOC
static cxa_ioStream_peekable_t ios_usart;
#endif

static bool hasBootFailed;

static cxa_siLabsBgApi_btle_central_t btlec;
static cxa_siLabsBgApi_btle_peripheral_t btlep;

static timerCallbackEntry_t timerCallbackEntries[CXA_SILABSBGAPI_MAX_NUM_TIMERS];

static cxa_logger_t logger;
static cxa_stateMachine_t stateMachine;


// ******** global function implementations ********
#ifndef CXA_SILABSBGAPI_MODE_SOC
void cxa_siLabsBgApi_module_init(cxa_ioStream_t *const ioStreamIn, int threadIdIn)
{
	cxa_assert(ioStreamIn);

	// save our references and setup our internal state
	cxa_ioStream_peekable_init(&ios_usart, ioStreamIn);
	hasBootFailed = false;
	cxa_logger_init(&logger, "bgApiModule");

	memset(timerCallbackEntries, 0, sizeof(timerCallbackEntries));

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

#else

void cxa_siLabsBgApi_module_init(void)
{
	// save our references and setup our internal state
	cxa_logger_init(&logger, "bgApiModule");

	memset(timerCallbackEntries, 0, sizeof(timerCallbackEntries));

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "bgApiBtleC", CXA_RUNLOOP_THREADID_DEFAULT);
	cxa_stateMachine_addState(&stateMachine, RADIOSTATE_WAIT_BOOT, "waitBoot", stateCb_waitForBoot_enter, stateCb_xxx_state, stateCb_waitBoot_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, RADIOSTATE_READY, "ready", stateCb_ready_enter, stateCb_xxx_state, NULL, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, RADIOSTATE_WAIT_BOOT);

	// setup our btle client and peripheral
	cxa_siLabsBgApi_btle_central_init(&btlec, CXA_RUNLOOP_THREADID_DEFAULT);
	cxa_siLabsBgApi_btle_peripheral_init(&btlep, CXA_RUNLOOP_THREADID_DEFAULT);
}
#endif


cxa_siLabsBgApi_btle_central_t* cxa_siLabsBgApi_module_getBtleCentral(void)
{
	return &btlec;
}


cxa_siLabsBgApi_btle_peripheral_t* cxa_siLabsBgApi_module_getBtlePeripheral(void)
{
	return &btlep;
}


void cxa_siLabsBgApi_module_startSoftTimer_repeat(float period_msIn, cxa_siLabsBgApi_cb_onTimer_t cbIn, void* userVarIn)
{
	for( size_t i = 0; i < sizeof(timerCallbackEntries)/sizeof(*timerCallbackEntries); i++ )
	{
		if( !timerCallbackEntries[i].isUsed )
		{
			timerCallbackEntries[i].isUsed = true;
			timerCallbackEntries[i].cb = cbIn;
			timerCallbackEntries[i].userVarIn = userVarIn;

			gecko_cmd_hardware_set_soft_timer((period_msIn / 1000.0) * 32768, i, false);
			return;
		}
	}

	// if we made it here, we don't have any free timers
	cxa_assert(0);
}


void cxa_siLabsBgApi_module_stopSoftTimer(cxa_siLabsBgApi_cb_onTimer_t cbIn, void* userVarIn)
{
	for( size_t i = 0; i < sizeof(timerCallbackEntries)/sizeof(*timerCallbackEntries); i++ )
	{
		if( timerCallbackEntries[i].isUsed &&
			(timerCallbackEntries[i].cb == cbIn) &&
			(timerCallbackEntries[i].userVarIn == userVarIn) )
		{
			// found our target entry
			timerCallbackEntries[i].isUsed = false;
			gecko_cmd_hardware_set_soft_timer(0, i, false);
			return;
		}
	}
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
		case gecko_evt_gatt_server_characteristic_status_id:
			if( !cxa_siLabsBgApi_btle_central_handleBgEvent(&btlec, evt) ) cxa_siLabsBgApi_btle_peripheral_handleBgEvent(&btlep, evt);
			break;

		case gecko_evt_gatt_server_user_read_request_id:
		case gecko_evt_gatt_server_user_write_request_id:
			cxa_siLabsBgApi_btle_peripheral_handleBgEvent(&btlep, evt);
			break;


			cxa_logger_debug(&logger, "chst: %X  %X", evt->data.evt_gatt_server_characteristic_status.client_config_flags, evt->data.evt_gatt_server_characteristic_status.status_flags);
			break;

		case gecko_evt_le_connection_parameters_id:
		case gecko_evt_le_connection_phy_status_id:
		case gecko_evt_gatt_mtu_exchanged_id:
			break;

		case gecko_evt_hardware_soft_timer_id:
			for( size_t i = 0; i < sizeof(timerCallbackEntries)/sizeof(*timerCallbackEntries); i++ )
			{
				if( (i == evt->data.evt_hardware_soft_timer.handle) &&
					timerCallbackEntries[i].isUsed )
				{
					if( timerCallbackEntries[i].cb != NULL ) timerCallbackEntries[i].cb(timerCallbackEntries[i].userVarIn);
				}
			}
			break;

		default:
			cxa_logger_debug(&logger, "unhandled event: 0x%08X", BGLIB_MSG_ID(evt->header));
			break;
	}
}


#ifndef CXA_SILABSBGAPI_MODE_SOC
static void stateCb_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "resetting radio");

	// tell the radio to reboot
	gecko_cmd_system_reset(0);

	// start waiting for the boot event
	cxa_stateMachine_transition(&stateMachine, RADIOSTATE_WAIT_BOOT);
}
#endif


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
#ifndef CXA_SILABSBGAPI_MODE_SOC
	appHandleEvents(gecko_peek_event());
#else
	appHandleEvents(gecko_wait_event());
#endif
}


#ifndef CXA_SILABSBGAPI_MODE_SOC
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
#endif
