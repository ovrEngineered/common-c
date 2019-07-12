/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_esp32_btle_client.h"
#ifdef CONFIG_BT_ENABLED


// ******** includes ********
#include <bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <string.h>

#include <cxa_assert.h>
#include <cxa_console.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define SCANPERIOD_S			24*60*60


// ******** local type definitions ********


// ******** local function prototypes ********
static void init(void);

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

static void scm_startScan(cxa_btle_client_t *const superIn, bool isActiveIn);
static void scm_stopScan(cxa_btle_client_t *const superIn);
static bool scm_isScanning(cxa_btle_client_t *const superIn);


static void consoleCb_startScan(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_stopScan(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);


// ********  local variable declarations *********
static bool isInit = false;
static cxa_btle_client_t singleton;

static bool isScanning = false;
static esp_ble_scan_type_t currScanType;

static cxa_logger_t logger;


// ******** global function implementations ********
cxa_btle_client_t* cxa_esp32_btle_client_getSingleton(void)
{
	if( !isInit ) init();

	return &singleton;
}


// ******** local function implementations ********
static void init(void)
{
	// initialize our superclass
	cxa_btle_client_init(&singleton, NULL, scm_startScan, scm_stopScan, scm_isScanning, NULL, NULL, NULL, NULL);

	// and our logger
	cxa_logger_init(&logger, "btleClient");

	// initialize the btle subsystem
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	cxa_assert( esp_bt_controller_init(&bt_cfg) == ESP_OK );
	cxa_assert( esp_bt_controller_enable(ESP_BT_MODE_BTDM) == ESP_OK );

	cxa_assert( esp_bluedroid_init() == ESP_OK );
	cxa_assert( esp_bluedroid_enable() == ESP_OK );

	cxa_assert( esp_ble_gap_register_callback(esp_gap_cb) == ESP_OK );

	// setup our console commands
	cxa_console_addCommand("btle.startScan", "start scan for adverts", NULL, 0, consoleCb_startScan, NULL);
	cxa_console_addCommand("btle.stopScan", "stop scan for adverts", NULL, 0, consoleCb_stopScan, NULL);

	isInit = true;
	isScanning = false;
}


static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	esp_ble_gap_cb_param_t* scanResult = (esp_ble_gap_cb_param_t *)param;

//	cxa_logger_stepDebug_msg("bleEvt: %d  %d", event, scanResult->scan_rst.search_evt);
	if( (event == ESP_GAP_BLE_SCAN_RESULT_EVT) && (scanResult->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) )
	{
		// advertisement received
		cxa_btle_advPacket_t rxPacket;

		// simple stuff first
		cxa_eui48_init(&rxPacket.addr, scanResult->scan_rst.bda);
		rxPacket.isRandomAddress = (scanResult->scan_rst.ble_addr_type == BLE_ADDR_TYPE_RANDOM);
		rxPacket.rssi = scanResult->scan_rst.rssi;

		// now the actual advert fields..first we must count them
		size_t numAdvFields = 0;
		if( !cxa_btle_client_countAdvFields(scanResult->scan_rst.ble_adv, sizeof(scanResult->scan_rst.ble_adv), &numAdvFields) )
		{
			cxa_logger_warn(&logger, "malformed field, discarding");
			return;
		}

		// now that we've counted them, we can parse them pretty safely (index-wise)
		cxa_btle_advField_t fields_raw[numAdvFields];
		cxa_array_initStd(&rxPacket.advFields, fields_raw);
		if( !cxa_btle_client_parseAdvFieldsForPacket(&rxPacket, numAdvFields,
				scanResult->scan_rst.ble_adv, sizeof(scanResult->scan_rst.ble_adv)) )
		{
			cxa_logger_warn(&logger, "malformed field, discarding");
						return;
		}

		cxa_eui48_string_t eui48_str;
		cxa_eui48_toString(&rxPacket.addr, &eui48_str);
		cxa_logger_debug(&logger, "adv from %s(%s)  %ddBm  %d fields",
				eui48_str.str,
				rxPacket.isRandomAddress ? "r" : "p",
				rxPacket.rssi, cxa_array_getSize_elems(&rxPacket.advFields));

		// notify our listeners
		cxa_btle_client_notify_advertRx(&singleton, &rxPacket);

	}
	else if( (event == ESP_GAP_BLE_SCAN_RESULT_EVT) && (scanResult->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) )
	{
		// scanning done...this might be a timeout OR requested by the user
		if( !isScanning ) return;

		// if we made it here, it was a timeout...restart the scan
		scm_startScan(&singleton, (currScanType == BLE_SCAN_TYPE_ACTIVE));
	}
}


static void scm_startScan(cxa_btle_client_t *const superIn, bool isActiveIn)
{
	// save our scan type
	currScanType = (isActiveIn) ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE;

	esp_ble_scan_params_t scanParams =
	{
			.scan_type = currScanType,
			.own_addr_type = BLE_ADDR_TYPE_RANDOM,
			.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
			.scan_interval = 0x10,
			.scan_window = 0x10
	};
	esp_ble_gap_set_scan_params(&scanParams);
	isScanning = true;
	esp_ble_gap_start_scanning(SCANPERIOD_S);
}


static void scm_stopScan(cxa_btle_client_t *const superIn)
{
	isScanning = false;
	esp_ble_gap_stop_scanning();
}


static bool scm_isScanning(cxa_btle_client_t *const superIn)
{
	return isScanning;
}


static void consoleCb_startScan(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	scm_startScan(&singleton, false);
}


static void consoleCb_stopScan(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	scm_stopScan(&singleton);
}

#endif
