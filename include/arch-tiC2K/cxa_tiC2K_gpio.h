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
#include <gpio.h>
#include <cxa_logger_header.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_tiC2K_gpio cxa_tiC2K_gpio_t;


/**
 * @public
 */
struct cxa_tiC2K_gpio
{
	cxa_gpio_t super;

	cxa_gpio_polarity_t polarity;
	cxa_gpio_direction_t dir;

	//TI specific
	uint32_t pinConfig;
    uint32_t pin;
	GPIO_CoreSelect core;
	uint32_t pinType;
//    uint32_t outVal;

    bool lastVal;

	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_tiC2K_gpio_init_input(cxa_tiC2K_gpio_t *const gpioIn, const uint32_t pinConfigIn, const uint32_t pinIn, const GPIO_CoreSelect coreIn, const uint32_t pinTypeIn, const cxa_gpio_polarity_t polarityIn);

void cxa_tiC2K_gpio_init_output(cxa_tiC2K_gpio_t *const gpioIn, const uint32_t pinConfigIn, const uint32_t pinIn, const GPIO_CoreSelect coreIn, const uint32_t pinTypeIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn);


#endif
