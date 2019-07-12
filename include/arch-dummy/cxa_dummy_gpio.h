/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_DUMMY_GPIO_H_
#define CXA_DUMMY_GPIO_H_


// ******** includes ********
#include <stdbool.h>

#include <cxa_gpio.h>
#include <cxa_logger_header.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_dummy_gpio cxa_dummy_gpio_t;


/**
 * @public
 */
struct cxa_dummy_gpio
{
	cxa_gpio_t super;

	cxa_gpio_polarity_t polarity;
	cxa_gpio_direction_t dir;

	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_dummy_gpio_init_input(cxa_dummy_gpio_t *const gpioIn, const cxa_gpio_polarity_t polarityIn);

void cxa_dummy_gpio_init_output(cxa_dummy_gpio_t *const gpioIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn);


#endif
