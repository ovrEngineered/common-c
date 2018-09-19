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
#include "cxa_led_gpio.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setBrightness(cxa_led_runLoop_t *const superIn, uint8_t brightness_255In);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_led_gpio_init(cxa_led_gpio_t *const ledIn, cxa_gpio_t *const gpioIn, int threadIdIn)
{
	cxa_assert(ledIn);
	cxa_assert(gpioIn);

	// save our references
	ledIn->gpio = gpioIn;

	// initialize our super class (since it sets our initial value)
	cxa_led_runLoop_init(&ledIn->super, scm_setBrightness, threadIdIn);

}


// ******** local function implementations ********
static void scm_setBrightness(cxa_led_runLoop_t *const superIn, uint8_t brightness_255In)
{
	cxa_led_gpio_t* ledIn = (cxa_led_gpio_t*)superIn;
	cxa_assert(ledIn);

	// don't change any state, just change the brightness
	cxa_gpio_setValue(ledIn->gpio, (brightness_255In > 0));
}
