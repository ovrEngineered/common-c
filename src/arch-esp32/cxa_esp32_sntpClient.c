/**
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
#include "cxa_sntpClient.h"


// ******** includes ********
#include <cxa_array.h>
#include <cxa_assert.h>
#include <cxa_network_wifiManager.h>

#include "apps/sntp/sntp.h"
#include <sys/time.h>
#include <time.h>


// ******** local macro definitions ********
#define SNTP_SERVER									"pool.ntp.org"

#ifndef CXA_SNTPCLIENT_MAXNUM_LISTENERS
	#define CXA_SNTPCLIENT_MAXNUM_LISTENERS		4
#endif


// ******** local type definitions ********
typedef struct
{
	cxa_sntpClient_cb_onInitialTimeSet_t cb_initialTimeSet;
	void *userVar;
}listenerEntry_t;


// ******** local function prototypes ********
static void wifiCb_onConnected(const char *const ssidIn, void* userVarIn);
static void wifiCb_onDisconnected(void* userVarIn);


// ********  local variable declarations *********
static bool isInit = false;

static cxa_array_t listeners;
static listenerEntry_t listeners_raw[CXA_SNTPCLIENT_MAXNUM_LISTENERS];


// ******** global function implementations ********
void cxa_sntpClient_init(void)
{
	cxa_array_initStd(&listeners, listeners_raw);
	cxa_network_wifiManager_addListener(NULL, NULL, NULL, wifiCb_onConnected, wifiCb_onDisconnected, NULL, NULL);

	isInit = true;
}


void cxa_sntpClient_addListener(cxa_sntpClient_cb_onInitialTimeSet_t cbIn, void *const userVarIn)
{
	cxa_assert(isInit);

	listenerEntry_t newEntry =
	{
		.cb_initialTimeSet = cbIn,
		.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&listeners, &newEntry));
}


bool cxa_sntpClient_isClockSet(void)
{
	cxa_assert(isInit);

	time_t now = 0;
	struct tm timeinfo = { 0 };

	time(&now);
	localtime_r(&now, &timeinfo);

	return timeinfo.tm_year >= (2016 - 1900);
}


uint32_t cxa_sntpClient_getUnixTimeStamp(void)
{
	cxa_assert(isInit);
	if( !cxa_sntpClient_isClockSet() ) return 0;

	return time(NULL);
}


// ******** local function implementations ********
static void wifiCb_onConnected(const char *const ssidIn, void* userVarIn)
{
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, SNTP_SERVER);
	sntp_init();
}


static void wifiCb_onDisconnected(void* userVarIn)
{
	sntp_stop();
}
