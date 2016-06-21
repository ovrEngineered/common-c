/**
 * @file
 * @copyright 2016 opencxa.org
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
#ifndef CXA_NETWORK_WIFI_MANAGER_H_
#define CXA_NETWORK_WIFI_MANAGER_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>


// ******** global macro definitions ********
#ifndef CXA_NETWORK_WIFIMGR_MAX_NUM_LISTENERS
	#define CXA_NETWORK_WIFIMGR_MAX_NUM_LISTENERS		2
#endif


// ******** global type definitions *********
typedef void (*cxa_network_wifiManager_ssid_cb_t)(const char *const ssidIn, void* userVarIn);


// ******** global function prototypes ********
void cxa_network_wifiManager_init(const char* ssidIn, const char* passphraseIn);
void cxa_network_wifiManager_addListener(cxa_network_wifiManager_ssid_cb_t cb_associatingWithSsid,
										 cxa_network_wifiManager_ssid_cb_t cb_associatedWithSsid,
										 cxa_network_wifiManager_ssid_cb_t cb_associateWithSsidFailed,
										 void *userVarIn);

bool cxa_network_wifiManager_isAssociated(void);

void cxa_network_wifiManager_start(void);


#endif // CXA_NETWORK_WIFI_MANAGER_H_
