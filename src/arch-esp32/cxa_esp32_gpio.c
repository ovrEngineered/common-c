/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_esp32_gpio.h"


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn);
static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn);
static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn);
static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn);
static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn);
static bool scm_getValue(cxa_gpio_t *const superIn);
static bool scm_enableInterrupts(cxa_gpio_t *const superIn, cxa_gpio_interruptType_t intTypeIn, cxa_gpio_cb_onInterrupt_t cbIn, void* userVarIn);

static void esp32GpioIsr(void* userVarIn);


// ********  local variable declarations *********
static bool isGpioIsrServiceInstalled = false;


// ******** global function implementations ********
void cxa_esp32_gpio_init_input(cxa_esp32_gpio_t *const gpioIn, const gpio_num_t pinNumIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;

	// make sure our pin is GPIO
	gpio_pad_select_gpio(pinNumIn);

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, scm_enableInterrupts);

	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_esp32_gpio_init_output(cxa_esp32_gpio_t *const gpioIn, const gpio_num_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;

	// make sure our pin is GPIO
	gpio_pad_select_gpio(pinNumIn);

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// set our initial value (before we set direction to avoid glitches)
	cxa_gpio_setValue(&gpioIn->super, initValIn);

	// now set our direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_esp32_gpio_init_safe(cxa_esp32_gpio_t *const gpioIn, const gpio_num_t pinNumIn)
{
	cxa_assert(gpioIn);
	cxa_assert(pinNumIn < 8);

	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = CXA_GPIO_POLARITY_NONINVERTED;
	gpioIn->dir = CXA_GPIO_DIR_UNKNOWN;

	// initialze our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// don't set any value or direction...just leave everything as it is
}


void cxa_esp32_gpio_setPullMode(cxa_esp32_gpio_t *const gpioIn, gpio_pull_mode_t pullModeIn)
{
	cxa_assert(gpioIn);

	gpio_set_pull_mode(gpioIn->pinNum, pullModeIn);
}


// ******** local function implementations ********
static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_esp32_gpio_t *const gpioIn = (cxa_esp32_gpio_t *const)superIn;

	gpio_set_direction(gpioIn->pinNum, (dirIn == CXA_GPIO_DIR_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
	gpioIn->dir = dirIn;
}


static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_esp32_gpio_t *const gpioIn = (cxa_esp32_gpio_t *const)superIn;

	return gpioIn->dir;
}


static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_esp32_gpio_t *const gpioIn = (cxa_esp32_gpio_t *const)superIn;

	gpioIn->polarity = polarityIn;
}


static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_esp32_gpio_t *const gpioIn = (cxa_esp32_gpio_t *const)superIn;

	return gpioIn->polarity;
}


static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_esp32_gpio_t *const gpioIn = (cxa_esp32_gpio_t *const)superIn;

	gpio_set_level(gpioIn->pinNum, (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn);
	gpioIn->lastVal = valIn;
}


static bool scm_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_esp32_gpio_t *const gpioIn = (cxa_esp32_gpio_t *const)superIn;

	bool retVal = (gpioIn->dir == CXA_GPIO_DIR_INPUT) ? gpio_get_level(gpioIn->pinNum) : gpioIn->lastVal;
	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal : retVal;
}


static bool scm_enableInterrupts(cxa_gpio_t *const superIn, cxa_gpio_interruptType_t intTypeIn, cxa_gpio_cb_onInterrupt_t cbIn, void* userVarIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_esp32_gpio_t *const gpioIn = (cxa_esp32_gpio_t *const)superIn;

	// install the GPIO ISR service if needed
	if( !isGpioIsrServiceInstalled )
	{
		gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
		isGpioIsrServiceInstalled = true;
	}

	// register a handler
	gpio_isr_handler_add(gpioIn->pinNum, esp32GpioIsr, (void*)gpioIn);

	// set our interrupt type
	gpio_int_type_t esp32IntType = GPIO_INTR_DISABLE;
	switch( intTypeIn )
	{
		case CXA_GPIO_INTERRUPTTYPE_RISING_EDGE:
			esp32IntType = GPIO_INTR_POSEDGE;
			break;

		case CXA_GPIO_INTERRUPTTYPE_FALLING_EDGE:
			esp32IntType = GPIO_INTR_NEGEDGE;
			break;

		case CXA_GPIO_INTERRUPTTYPE_ONCHANGE:
			esp32IntType = GPIO_INTR_ANYEDGE;
			break;
	}
	gpio_set_intr_type(gpioIn->pinNum, esp32IntType);

	// enable our interrupt
	gpio_intr_enable(gpioIn->pinNum);

	return true;
}


static void esp32GpioIsr(void* userVarIn)
{
	cxa_assert(userVarIn);

	// get a pointer to our class
	cxa_esp32_gpio_t *const gpioIn = (cxa_esp32_gpio_t *const)userVarIn;

	cxa_gpio_notify_onInterrupt(&gpioIn->super);
}
