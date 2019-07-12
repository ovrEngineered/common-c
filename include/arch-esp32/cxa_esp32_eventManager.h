/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ESP32_EVENTMANAGER_H_
#define CXA_ESP32_EVENTMANAGER_H_


// ******** includes ********
#include "esp_event.h"


// ******** global macro definitions ********
#ifndef CXA_ESP32_EVENTMGR_MAXNUM_LISTENERS
	#define CXA_ESP32_EVENTMGR_MAXNUM_LISTENERS		2
#endif


// ******** global type definitions *********
typedef void (*cxa_esp32_eventManager_cb_t)(system_event_t *eventIn, void *const userVarIn);


// ******** global function prototypes ********
void cxa_esp32_eventManager_init(void);

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
									   void *const userVarIn);



#endif
