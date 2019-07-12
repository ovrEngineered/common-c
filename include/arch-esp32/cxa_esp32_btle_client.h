/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ESP32_BTLE_CLIENT_H_
#define CXA_ESP32_BTLE_CLIENT_H_


// ******** includes ********
#include <cxa_btle_central.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
cxa_btle_central_t* cxa_esp32_btle_client_getSingleton(void);

#endif
