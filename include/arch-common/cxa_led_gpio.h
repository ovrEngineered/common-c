/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_LED_GPIO_H_
#define CXA_LED_GPIO_H_


// ******** includes ********
#include <cxa_led_runLoop.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_led_runLoop_t super;

	cxa_gpio_t* gpio;
}cxa_led_gpio_t;


// ******** global function prototypes ********
void cxa_led_gpio_init(cxa_led_gpio_t *const ledIn, cxa_gpio_t *const gpioIn, int threadIdIn);


#endif /* CXA_LED_GPIO_H_ */
