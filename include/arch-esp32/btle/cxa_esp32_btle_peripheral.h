/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ESP32_BTLE_PERIPHERAL_H_
#define CXA_ESP32_BTLE_PERIPHERAL_H_


// ******** includes ********
#include <cxa_btle_peripheral.h>
#include <cxa_fixedByteBuffer.h>


#include <esp_gap_ble_api.h>
#include "esp_gatts_api.h"


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_esp32_btle_peripheral cxa_esp32_btle_peripheral_t;


/**
 * @private
 */
typedef struct
{
	cxa_btle_peripheral_charEntry_t *charEntry;
	uint16_t handle_service;
	uint16_t handle_char;
}cxa_esp32_btle_handleCharMapEntry_t;


/**
 * @private
 */
struct cxa_esp32_btle_peripheral
{
	cxa_btle_peripheral_t super;

	esp_ble_adv_params_t advParams;
	esp_ble_adv_data_t advData;

	cxa_fixedByteBuffer_t fbb_advertData_manSpecific;
	uint8_t fbb_advertData_manSpecific_raw[CXA_BTLE_PERIPHERAL_ADVERT_MAX_SIZE_BYTES];

	cxa_array_t handleCharMap;
	cxa_esp32_btle_handleCharMapEntry_t handleCharMap_raw[CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES];

	size_t currRegisteringCharEntryIndex;

	bool isConnected;
	esp_gatt_if_t currGattsIf;
	uint16_t currConnId;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_esp32_btle_peripheral_init(cxa_esp32_btle_peripheral_t *const btlepIn, int threadIdIn);


/**
 * @public
 */
void cxa_esp32_btle_peripheral_setDeviceName(cxa_esp32_btle_peripheral_t *const btlepIn, const char *const nameIn);


/**
 * @protected
 */
void cxa_esp32_btle_peripheral_handleEvent_gap(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);


/**
 * @protected
 */
void cxa_esp32_btle_peripheral_handleEvent_gatts(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);


#endif
