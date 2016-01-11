/**
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
#include "cxa_led.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_led_init(cxa_led_t *const ledIn, cxa_gpio_t *const gpioIn)
{
	cxa_assert(ledIn);
	cxa_assert(gpioIn);

	// save our refences
	ledIn->gpio = gpioIn;

	// setup our internal state
	cxa_timeDiff_init(&ledIn->td_blink, true);
	cxa_led_turnOff(ledIn);
}

void cxa_led_turnOn(cxa_led_t *const ledIn)
{
	cxa_assert(ledIn);

	cxa_gpio_setValue(ledIn->gpio, true);
	ledIn->currState = CXA_LED_STATE_ON;
}


void cxa_led_turnOff(cxa_led_t *const ledIn)
{
	cxa_assert(ledIn);

	cxa_gpio_setValue(ledIn->gpio, false);
	ledIn->currState = CXA_LED_STATE_OFF;
}


void cxa_led_blink(cxa_led_t *const ledIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn)
{
	cxa_assert(ledIn);

	ledIn->onPeriod_ms = onPeriod_msIn;
	ledIn->offPeriod_ms = offPeriod_msIn;
	ledIn->currState = CXA_LED_STATE_BLINK;
}


void cxa_led_update(cxa_led_t *const ledIn)
{
	cxa_assert(ledIn);

	if( (ledIn->currState == CXA_LED_STATE_BLINK) &&
			cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_blink, (cxa_gpio_getValue(ledIn->gpio) ? ledIn->onPeriod_ms : ledIn->offPeriod_ms)) )
	{
		cxa_gpio_toggle(ledIn->gpio);
	}
}


// ******** local function implementations ********
