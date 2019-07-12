/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_esp32_eventManager.h"


// ******** includes ********
#include "esp_event_loop.h"

#include <cxa_array.h>
#include <cxa_assert.h>
#include <cxa_stateMachine.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_WARN
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	cxa_esp32_eventManager_cb_t cb_sta_start;
	cxa_esp32_eventManager_cb_t cb_sta_stop;
	cxa_esp32_eventManager_cb_t cb_sta_connected;
	cxa_esp32_eventManager_cb_t cb_sta_disconnected;
	cxa_esp32_eventManager_cb_t cb_sta_gotIp;
	cxa_esp32_eventManager_cb_t cb_eth_start;
	cxa_esp32_eventManager_cb_t cb_eth_stop;
	cxa_esp32_eventManager_cb_t cb_eth_connected;
	cxa_esp32_eventManager_cb_t cb_eth_disconnected;
	cxa_esp32_eventManager_cb_t cb_eth_gotIp;

	void *const userVarIn;
}listener_t;


// ******** local function prototypes ********
static esp_err_t espCb_eventHandler(void *ctx, system_event_t *event);

static void notifyListeners_sta_start(system_event_t *eventIn);
static void notifyListeners_sta_stop(system_event_t *eventIn);
static void notifyListeners_sta_connected(system_event_t *eventIn);
static void notifyListeners_sta_disconnected(system_event_t *eventIn);
static void notifyListeners_sta_gotIp(system_event_t *eventIn);
static void notifyListeners_eth_start(system_event_t *eventIn);
static void notifyListeners_eth_stop(system_event_t *eventIn);
static void notifyListeners_eth_connected(system_event_t *eventIn);
static void notifyListeners_eth_disconnected(system_event_t *eventIn);
static void notifyListeners_eth_gotIp(system_event_t *eventIn);


// ********  local variable declarations *********
static bool isInit = false;

static cxa_array_t listeners;
static listener_t listeners_raw[CXA_ESP32_EVENTMGR_MAXNUM_LISTENERS];

static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_esp32_eventManager_init(void)
{
	if( isInit ) return;
	isInit = true;

	cxa_logger_init(&logger, "esp32EventManager");

	cxa_array_initStd(&listeners, listeners_raw);

	cxa_assert( esp_event_loop_init(espCb_eventHandler, NULL) == ESP_OK );
}


void cxa_esp32_eventManager_addListener(cxa_esp32_eventManager_cb_t cb_sta_startIn,
									   cxa_esp32_eventManager_cb_t cb_sta_stopIn,
									   cxa_esp32_eventManager_cb_t cb_sta_connectedIn,
									   cxa_esp32_eventManager_cb_t cb_sta_disconnectedIn,
									   cxa_esp32_eventManager_cb_t cb_sta_gotIpIn,
									   cxa_esp32_eventManager_cb_t cb_eth_startIn,
									   cxa_esp32_eventManager_cb_t cb_eth_stopIn,
									   cxa_esp32_eventManager_cb_t cb_eth_connectedIn,
									   cxa_esp32_eventManager_cb_t cb_eth_disconnectedIn,
									   cxa_esp32_eventManager_cb_t cb_eth_gotIpIn,
									   void *const userVarIn)
{
	if( !isInit ) cxa_esp32_eventManager_init();

	listener_t newListener = {
			.cb_sta_start = cb_sta_startIn,
			.cb_sta_stop = cb_sta_stopIn,
			.cb_sta_connected = cb_sta_connectedIn,
			.cb_sta_disconnected = cb_sta_disconnectedIn,
			.cb_sta_gotIp = cb_sta_gotIpIn,
			.cb_eth_start = cb_eth_startIn,
			.cb_eth_stop = cb_eth_stopIn,
			.cb_eth_connected = cb_eth_connectedIn,
			.cb_eth_disconnected = cb_eth_disconnectedIn,
			.cb_eth_gotIp = cb_eth_gotIpIn
	};

	cxa_assert(cxa_array_append(&listeners, &newListener));
}


// ******** local function implementations ********
static esp_err_t espCb_eventHandler(void *ctx, system_event_t *event)
{
	switch( event->event_id )
	{
		case SYSTEM_EVENT_STA_START:
			cxa_logger_debug(&logger, "new event: %s", "STA_START");
			notifyListeners_sta_start(event);
			break;

		case SYSTEM_EVENT_STA_STOP:
			cxa_logger_debug(&logger, "new event: %s", "STA_STOP");
			notifyListeners_sta_stop(event);
			break;

		case SYSTEM_EVENT_STA_CONNECTED:
			cxa_logger_debug(&logger, "new event: %s", "STA_CONNECTED");
			notifyListeners_sta_connected(event);
			break;

		case SYSTEM_EVENT_STA_DISCONNECTED:
			cxa_logger_debug(&logger, "new event: %s", "STA_DISCONNECTED");
			notifyListeners_sta_disconnected(event);
			break;

		case SYSTEM_EVENT_STA_GOT_IP:
			cxa_logger_debug(&logger, "new event: %s", "STA_GOT_IP");
			notifyListeners_sta_gotIp(event);
			break;

		case SYSTEM_EVENT_ETH_START:
			cxa_logger_debug(&logger, "new event: %s", "ETH_START");
			notifyListeners_eth_start(event);
			break;

		case SYSTEM_EVENT_ETH_STOP:
			cxa_logger_debug(&logger, "new event: %s", "ETH_STOP");
			notifyListeners_eth_stop(event);
			break;

		case SYSTEM_EVENT_ETH_CONNECTED:
			cxa_logger_debug(&logger, "new event: %s", "ETH_CONNECTED");
			notifyListeners_eth_connected(event);
			break;

		case SYSTEM_EVENT_ETH_DISCONNECTED:
			cxa_logger_debug(&logger, "new event: %s", "ETH_DISCONNECTED");
			notifyListeners_eth_disconnected(event);
			break;

		case SYSTEM_EVENT_ETH_GOT_IP:
			cxa_logger_debug(&logger, "new event: %s", "ETH_GOT_IP");
			notifyListeners_eth_gotIp(event);
			break;

		default:
			cxa_logger_debug(&logger, "unhandled event: %d", event->event_id);
			break;
	}

	return ESP_OK;
}


static void notifyListeners_sta_start(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_sta_start != NULL) ) currListener->cb_sta_start(eventIn, currListener->userVarIn);
	}
}


static void notifyListeners_sta_stop(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_sta_stop != NULL) ) currListener->cb_sta_stop(eventIn, currListener->userVarIn);
	}
}


static void notifyListeners_sta_connected(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_sta_connected != NULL) ) currListener->cb_sta_connected(eventIn, currListener->userVarIn);
	}
}


static void notifyListeners_sta_disconnected(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_sta_disconnected != NULL) ) currListener->cb_sta_disconnected(eventIn, currListener->userVarIn);
	}
}


static void notifyListeners_sta_gotIp(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_sta_gotIp != NULL) ) currListener->cb_sta_gotIp(eventIn, currListener->userVarIn);
	}
}


static void notifyListeners_eth_start(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_eth_start != NULL) ) currListener->cb_eth_start(eventIn, currListener->userVarIn);
	}
}


static void notifyListeners_eth_stop(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_eth_stop != NULL) ) currListener->cb_eth_stop(eventIn, currListener->userVarIn);
	}
}


static void notifyListeners_eth_connected(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_eth_connected != NULL) ) currListener->cb_eth_connected(eventIn, currListener->userVarIn);
	}
}


static void notifyListeners_eth_disconnected(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_eth_disconnected != NULL) ) currListener->cb_eth_disconnected(eventIn, currListener->userVarIn);
	}
}


static void notifyListeners_eth_gotIp(system_event_t *eventIn)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_eth_gotIp != NULL) ) currListener->cb_eth_gotIp(eventIn, currListener->userVarIn);
	}
}
