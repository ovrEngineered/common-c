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
typedef enum
{
	CXA_NETWORK_WIFISTATE_STARTUP,
	CXA_NETWORK_WIFISTATE_PROVISIONING,
	CXA_NETWORK_WIFISTATE_ASSOCIATING,
	CXA_NETWORK_WIFISTATE_ASSOCIATED,
	CXA_NETWORK_WIFISTATE_MICROAP
}cxa_network_wifiManager_state_t;


typedef void (*cxa_network_wifiManager_cb_t)(void* userVarIn);
typedef void (*cxa_network_wifiManager_ssid_cb_t)(const char *const ssidIn, void* userVarIn);


// ******** global function prototypes ********
void cxa_network_wifiManager_init(void);
void cxa_network_wifiManager_addListener(cxa_network_wifiManager_cb_t cb_provisioniningEnterIn,
										 cxa_network_wifiManager_ssid_cb_t cb_associatingWithSsidIn,
										 cxa_network_wifiManager_ssid_cb_t cb_associatedWithSsidIn,
										 cxa_network_wifiManager_ssid_cb_t cb_lostAssociationWithSsidIn,
										 cxa_network_wifiManager_ssid_cb_t cb_associateWithSsidFailedIn,
										 cxa_network_wifiManager_ssid_cb_t cb_microApEnterIn,
										 void *userVarIn);

void cxa_network_wifiManager_start(void);

cxa_network_wifiManager_state_t cxa_network_wifiManager_getState(void);

bool cxa_network_wifiManager_restart(void);
bool cxa_network_wifiManager_enterProvision(void);
bool cxa_network_wifiManager_enterMicroAp(const char* ssidIn, const char* passphraseIn);


#endif // CXA_NETWORK_WIFI_MANAGER_H_
