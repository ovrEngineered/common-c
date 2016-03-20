/**
 * @file
 * @copyright 2015 opencxa.org
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
#ifndef CXA_ESP8266_WIFI_MANAGER_H_
#define CXA_ESP8266_WIFI_MANAGER_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>


// ******** global macro definitions ********
#ifndef CXA_ESP8266_WIFIMGR_MAX_NUM_LISTENERS
	#define CXA_ESP8266_WIFIMGR_MAX_NUM_LISTENERS		1
#endif


// ******** global type definitions *********
typedef void (*cxa_esp8266_wifiManager_ssid_cb_t)(const char *const ssidIn, void* userVarIn);


// ******** global function prototypes ********
void cxa_esp8266_wifiManager_init(const char* ssidIn, const char* passphraseIn);
void cxa_esp8266_wifiManager_addListener(cxa_esp8266_wifiManager_ssid_cb_t cb_associatingWithSsid,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_associatedWithSsid,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_lostAssociationWithSsid,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_associateWithSsidFailed,
										 void *userVarIn);

bool cxa_esp8266_wifiManager_isAssociated(void);

void cxa_esp8266_wifiManager_start(void);
void cxa_esp8266_wifiManager_update(void);


#endif // CXA_ESP8266_WIFI_MANAGER_H_
