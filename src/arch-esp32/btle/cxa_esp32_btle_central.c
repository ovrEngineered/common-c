/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_esp32_btle_central.h"

#include <sdkconfig.h>
#ifdef CONFIG_BT_ENABLED


// ******** includes ********
#include <cxa_runLoop.h>

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <string.h>

#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
#define SCANPERIOD_S				60


// ******** local function prototypes ********
static cxa_btle_central_state_t scm_getState(cxa_btle_central_t *const superIn);
static void scm_startScan(cxa_btle_central_t *const superIn, bool isActiveIn);
static void scm_stopScan(cxa_btle_central_t *const superIn);
static void scm_startConnection(cxa_btle_central_t *const superIn, cxa_eui48_t *const addrIn, bool isRandomAddrIn);


// ********  local variable declarations *********



// ******** global function implementations ********
void cxa_esp32_btle_central_init(cxa_esp32_btle_central_t *const btlecIn, int threadIdIn)
{
	cxa_assert(btlecIn);

	btlecIn->shouldBeScanning = false;

	// and our logger
	cxa_logger_init(&btlecIn->logger, "btleClient");

	// initialize our superclass
	cxa_btle_central_init(&btlecIn->super, scm_getState, scm_startScan, scm_stopScan, scm_startConnection);
}


void cxa_esp32_btle_central_handleEvent_gap(cxa_esp32_btle_central_t *const btlecIn, esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	cxa_assert(btlecIn);

	esp_ble_gap_cb_param_t* scanResult = (esp_ble_gap_cb_param_t *)param;

	if( (event == ESP_GAP_BLE_SCAN_RESULT_EVT) && (scanResult->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) )
	{
		// advertisement received
		cxa_btle_advPacket_t rxPacket;
		if( !cxa_btle_advPacket_init(&rxPacket,
									 scanResult->scan_rst.ble_adv,
									 scanResult->scan_rst.ble_addr_type == BLE_ADDR_TYPE_RANDOM ,
									 scanResult->scan_rst.rssi,
									 scanResult->scan_rst.ble_adv,
									 scanResult->scan_rst.adv_data_len) )

		{
			cxa_logger_warn(&btlecIn->logger, "malformed field, discarding");
			return;
		}

		// now the actual advert fields..first we must count them
		size_t numAdvFields = 0;
		cxa_btle_advPacket_getNumFields(&rxPacket, &numAdvFields);

		cxa_eui48_string_t eui48_str;
		cxa_eui48_toString(&rxPacket.addr, &eui48_str);
		cxa_logger_debug(&btlecIn->logger, "adv from %s(%s)  %ddBm  %d fields",
				eui48_str.str,
				rxPacket.isRandomAddress ? "r" : "p",
				rxPacket.rssi, numAdvFields);

		// notify our listeners
		cxa_btle_central_notify_advertRx(&btlecIn->super, &rxPacket);

	}
	else if( event == ESP_GAP_BLE_SCAN_START_COMPLETE_EVT )
	{
		bool startSuccessful = param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS;
		if( !startSuccessful )
		{
			cxa_logger_warn(&btlecIn->super.logger, "scan start failed: %d", param->scan_start_cmpl.status);
		}

		cxa_btle_central_notify_scanStart(&btlecIn->super, startSuccessful);
	}
	else if( (event == ESP_GAP_BLE_SCAN_RESULT_EVT) && (scanResult->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) )
	{
		// scanning done...this might be a timeout OR requested by the user
		if( !btlecIn->shouldBeScanning ) return;

		// if we made it here, it was a timeout...restart the scan
		scm_startScan(&btlecIn->super, (btlecIn->currScanType == BLE_SCAN_TYPE_ACTIVE));
	}
}


// ******** local function implementations ********
static cxa_btle_central_state_t scm_getState(cxa_btle_central_t *const superIn)
{
	cxa_esp32_btle_central_t *const btlecIn = (cxa_esp32_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	return CXA_BTLE_CENTRAL_STATE_READY;
}


static void scm_startScan(cxa_btle_central_t *const superIn, bool isActiveIn)
{
	cxa_esp32_btle_central_t *const btlecIn = (cxa_esp32_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	// save our scan type
	btlecIn->currScanType = (isActiveIn) ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE;

	esp_ble_scan_params_t scanParams =
	{
			.scan_type = btlecIn->currScanType,
			.own_addr_type = BLE_ADDR_TYPE_RANDOM,
			.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
			.scan_interval = 0x10,
			.scan_window = 0x10
	};
	esp_ble_gap_set_scan_params(&scanParams);
	btlecIn->shouldBeScanning = true;
	esp_ble_gap_start_scanning(SCANPERIOD_S);
}



static void scm_stopScan(cxa_btle_central_t *const superIn)
{
	cxa_esp32_btle_central_t *const btlecIn = (cxa_esp32_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	btlecIn->shouldBeScanning = false;
	esp_ble_gap_stop_scanning();
}


static void scm_startConnection(cxa_btle_central_t *const superIn, cxa_eui48_t *const addrIn, bool isRandomAddrIn)
{
	cxa_esp32_btle_central_t *const btlecIn = (cxa_esp32_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	cxa_assert_failWithMsg("not implemented");
}

#endif
