/**
 * @file
 * @copyright 2017 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
