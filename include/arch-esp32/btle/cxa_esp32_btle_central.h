/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ESP32_BTLE_CENTRAL_H_
#define CXA_ESP32_BTLE_CENTRAL_H_


// ******** includes ********
#include <cxa_btle_central.h>
#include <cxa_logger_header.h>


#include <esp_gap_ble_api.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_esp32_btle_central cxa_esp32_btle_central_t;


/**
 * @private
 */
struct cxa_esp32_btle_central
{
	cxa_btle_central_t super;

	bool shouldBeScanning;
	esp_ble_scan_type_t currScanType;

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_esp32_btle_central_init(cxa_esp32_btle_central_t *const btlecIn, int threadIdIn);


/**
 * @protected
 */
void cxa_esp32_btle_central_handleEvent_gap(cxa_esp32_btle_central_t *const btlecIn, esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

#endif
