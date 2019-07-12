/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
