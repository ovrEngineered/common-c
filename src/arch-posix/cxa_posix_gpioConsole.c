/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_posix_gpioConsole.h"


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>
#include <cxa_config.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn);
static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn);
static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn);
static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn);
static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn);
static bool scm_getValue(cxa_gpio_t *const superIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_posix_gpioConsole_init_input(cxa_posix_gpioConsole_t *const gpioIn, const char *nameIn)
{
	cxa_assert(gpioIn);
	cxa_assert(nameIn);

	// save our references
	gpioIn->name = nameIn;

	// setup our logger
	cxa_logger_init_formattedString(&gpioIn->logger, "gpio: '%s'", nameIn);

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_posix_gpioConsole_init_output(cxa_posix_gpioConsole_t *const gpioIn, const char *nameIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert(nameIn);

	// save our references
	gpioIn->name = nameIn;

	// setup our logger
	cxa_logger_init_formattedString(&gpioIn->logger, "gpio: '%s'", nameIn);

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// set our initial value
	cxa_gpio_setValue(&gpioIn->super, initValIn);
	// now set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_posix_gpioConsole_init_safe(cxa_posix_gpioConsole_t *const gpioIn, const char *nameIn)
{
	cxa_assert(gpioIn);
	cxa_assert(nameIn);

	// save our references
	gpioIn->name = nameIn;

	// setup our logger
	cxa_logger_init_formattedString(&gpioIn->logger, "gpio: '%s'", nameIn);

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// we don't know our current direction
	gpioIn->currDir = CXA_GPIO_DIR_UNKNOWN;
}


// ******** local function implementations ********
static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_posix_gpioConsole_t *const gpioIn = (cxa_posix_gpioConsole_t *const)superIn;

	gpioIn->currDir = dirIn;
	cxa_logger_debug(&gpioIn->logger, "setDir: %d", dirIn);
}


static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_posix_gpioConsole_t *const gpioIn = (cxa_posix_gpioConsole_t *const)superIn;

	return gpioIn->currDir;
}


static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_posix_gpioConsole_t *const gpioIn = (cxa_posix_gpioConsole_t *const)superIn;

	gpioIn->polarity = polarityIn;
}


static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_posix_gpioConsole_t *const gpioIn = (cxa_posix_gpioConsole_t *const)superIn;

	return gpioIn->polarity;
}


static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_posix_gpioConsole_t *const gpioIn = (cxa_posix_gpioConsole_t *const)superIn;

	gpioIn->currVal = valIn;
	cxa_logger_debug(&gpioIn->logger, "setVal: %d", gpioIn->currVal);
}


static bool scm_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_posix_gpioConsole_t *const gpioIn = (cxa_posix_gpioConsole_t *const)superIn;

	bool retVal = gpioIn->currVal;
	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal : retVal;
}
