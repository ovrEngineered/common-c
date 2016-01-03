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
#ifndef CXA_GPIO_DEBOUNCER_H_
#define CXA_GPIO_DEBOUNCER_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_gpio.h>
#include <cxa_timeDiff.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_GPIO_DEBOUNCER_MAXNUM_LISTENERS
	#define CXA_GPIO_DEBOUNCER_MAXNUM_LISTENERS			1
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_gpio_debouncer cxa_gpio_debouncer_t;


/**
 * @public
 */
typedef void (*cxa_gpio_debouncer_cb_onTransition_t)(bool newVal, void *userVarIn);


typedef struct
{
	cxa_gpio_debouncer_cb_onTransition_t cb_onTransition;
	void *userVar;
}cxa_gpio_debouncer_listenerEntry_t;


/**
 * @private
 */
struct cxa_gpio_debouncer
{
	cxa_timeDiff_t td_debounce;
	cxa_gpio_t* gpio;

	bool prevVal;
	bool isInTimeoutPeriod;

	cxa_array_t listeners;
	cxa_gpio_debouncer_listenerEntry_t listeners_raw[CXA_GPIO_DEBOUNCER_MAXNUM_LISTENERS];
};


// ******** global function prototypes ********
void cxa_gpio_debouncer_init(cxa_gpio_debouncer_t *const debounceIn, cxa_gpio_t *const gpioIn);

void cxa_gpio_debouncer_addListener(cxa_gpio_debouncer_t *const debouncerIn, cxa_gpio_debouncer_cb_onTransition_t cb_onTransition, void *userVarIn);

void cxa_gpio_debouncer_update(cxa_gpio_debouncer_t *const debouncerIn);

#endif // CXA_GPIO_DEBOUNCER_H_
