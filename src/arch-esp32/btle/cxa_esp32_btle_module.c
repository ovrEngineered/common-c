/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_esp32_btle_module.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gatt_common_api.h>
#include <esp_gatts_api.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define CXA_ESP32_GATTS_APP_ID					0


// ******** local type definitions ********


// ******** local function prototypes ********
static void runLoopCb_onStart(void* userVarIn);

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gatts_cb(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);


// ********  local variable declarations *********
static cxa_esp32_btle_central_t btlec;
static cxa_esp32_btle_peripheral_t btlep;

static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_esp32_btle_module_init(int threadIdIn)
{
	cxa_logger_init(&logger, "esp32BtleModule");

	// initialize the btle subsystem
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	cxa_assert( esp_bt_controller_init(&bt_cfg) == ESP_OK );
	cxa_assert( esp_bt_controller_enable(ESP_BT_MODE_BLE) == ESP_OK );

	cxa_assert( esp_bluedroid_init() == ESP_OK );
	cxa_assert( esp_bluedroid_enable() == ESP_OK );

	cxa_eui48_t randEui48;
	cxa_eui48_initRandom(&randEui48);
	randEui48.bytes[0] |= 0xC0;
	cxa_assert( esp_ble_gap_set_rand_addr(randEui48.bytes) == ESP_OK );
	cxa_assert( esp_ble_gap_register_callback(esp_gap_cb) == ESP_OK );
	cxa_assert( esp_ble_gatts_register_callback(esp_gatts_cb) == ESP_OK );
	cxa_assert( esp_ble_gatt_set_local_mtu(500) == ESP_OK );

	// intialize our central and peripherals
	cxa_esp32_btle_central_init(&btlec, threadIdIn);
	cxa_esp32_btle_peripheral_init(&btlep, threadIdIn);

	// register for runLoop
	cxa_runLoop_addEntry(threadIdIn, runLoopCb_onStart, NULL, NULL);
}


cxa_esp32_btle_central_t* cxa_esp32_btle_module_getBtleCentral(void)
{
	return &btlec;
}


cxa_esp32_btle_peripheral_t* cxa_esp32_btle_module_getBtlePeripheral(void)
{
	return &btlep;
}


// ******** local function implementations ********
static void runLoopCb_onStart(void* userVarIn)
{
	cxa_assert( esp_ble_gatts_app_register(CXA_ESP32_GATTS_APP_ID) == ESP_OK );

	cxa_btle_central_notify_onBecomesReady(&btlec.super);
	// notify the peripheral once the APPID is registered
}


static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	switch( event )
	{
		case ESP_GAP_BLE_SCAN_RESULT_EVT:
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
		case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
			cxa_esp32_btle_central_handleEvent_gap(&btlec, event, param);
			break;

		case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
		case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
			cxa_esp32_btle_peripheral_handleEvent_gap(&btlep, event, param);
			break;

		default:
			cxa_logger_debug(&logger, "unhandled gapEvt: %d", event);
			break;
	}
}


static void esp_gatts_cb(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	switch( event )
	{
		case ESP_GATTS_REG_EVT:
			cxa_btle_peripheral_notify_onBecomesReady(&btlep.super);
			cxa_esp32_btle_peripheral_handleEvent_gatts(&btlep, event, gatts_if, param);
			break;

		case ESP_GATTS_CREATE_EVT:
		case ESP_GATTS_ADD_CHAR_EVT:
		case ESP_GATTS_START_EVT:
		case ESP_GATTS_ADD_CHAR_DESCR_EVT:
		case ESP_GATTS_READ_EVT:
		case ESP_GATTS_WRITE_EVT:
			cxa_esp32_btle_peripheral_handleEvent_gatts(&btlep, event, gatts_if, param);
			break;

		case ESP_GATTS_CONNECT_EVT:
		case ESP_GATTS_MTU_EVT:
		case ESP_GATTS_DISCONNECT_EVT:
		case ESP_GATTS_RESPONSE_EVT:
			// nothing to do here
			break;

		default:
			cxa_logger_debug(&logger, "unhandled gattsEvt: %d  on  %d", event, gatts_if);
			break;
	}
}
