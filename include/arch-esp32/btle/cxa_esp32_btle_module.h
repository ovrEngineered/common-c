/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ESP32_BTLE_MODULE_H_
#define CXA_ESP32_BTLE_MODULE_H_


// ******** includes ********
#include <cxa_esp32_btle_central.h>
#include <cxa_esp32_btle_peripheral.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 */
void cxa_esp32_btle_module_init(int threadIdIn);


/**
 * @public
 */
cxa_esp32_btle_central_t* cxa_esp32_btle_module_getBtleCentral(void);


/**
 * @public
 */
cxa_esp32_btle_peripheral_t* cxa_esp32_btle_module_getBtlePeripheral(void);


#endif
