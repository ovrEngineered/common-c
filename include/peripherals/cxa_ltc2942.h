/**
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
#ifndef CXA_LTC2942_H_
#define CXA_LTC2942_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_gpio.h>
#include <cxa_i2cMaster.h>
#include <cxa_logger_header.h>


// ******** global macro definitions ********
#ifndef CXA_LTC2942_MAXNUM_LISTENERS
	#define CXA_LTC2942_MAXNUM_LISTENERS		2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_ltc2942 cxa_ltc2942_t;


/**
 * @public
 */
typedef void (*cxa_ltc2942_cb_startup_t)(bool wasSuccessfulIn, void *const userVarIn);


/**
 * @public
 */
typedef void (*cxa_ltc2942_cb_updatedValue_t)(bool wasSuccessfulIn, uint16_t remainingCapacity_mahIn, void *const userVarIn);


/**
 * @public
 */
typedef struct
{
	uint8_t channelIndex;
	uint8_t brightness;
}cxa_ltc2942_channelEntry_t;


/**
 * @private
 */
typedef struct
{
	cxa_ltc2942_cb_startup_t cb_onStartup;
	cxa_ltc2942_cb_updatedValue_t cb_onUpdatedValue;

	void* userVar;
}cxa_ltc2942_listener_t;


/**
 * @private
 */
struct cxa_ltc2942
{
	cxa_i2cMaster_t* i2c;

	uint16_t batteryInitCap_mah;
	bool isInitialized;

	cxa_logger_t logger;

	cxa_array_t listeners;
	cxa_ltc2942_listener_t listeners_raw[CXA_LTC2942_MAXNUM_LISTENERS];
};


// ******** global function prototypes ********
void cxa_ltc2942_init(cxa_ltc2942_t *const ltcIn, cxa_i2cMaster_t *const i2cIn, uint16_t batteryInitCap_mahIn);

void cxa_ltc2942_addListener(cxa_ltc2942_t *const ltcIn, cxa_ltc2942_cb_startup_t cb_onStartupIn, cxa_ltc2942_cb_updatedValue_t cb_onUpdatedValue, void *userVarIn);

void cxa_ltc2942_start(cxa_ltc2942_t *const ltcIn);

void cxa_ltc2942_requestRemainingCapacityNow(cxa_ltc2942_t *const ltcIn);

#endif // CXA_LTC2942_H_
