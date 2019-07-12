/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_POSIX_GPIOCONSOLE_H_
#define CXA_POSIX_GPIOCONSOLE_H_


// ******** includes ********
#include <cxa_gpio.h>
#include <cxa_logger_header.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_gpio_t super;

	const char *name;
	cxa_gpio_direction_t currDir;
	cxa_gpio_polarity_t polarity;
	bool currVal;

	cxa_logger_t logger;
}cxa_posix_gpioConsole_t;


// ******** global function prototypes ********
void cxa_posix_gpioConsole_init_input(cxa_posix_gpioConsole_t *const gpioIn, const char *nameIn);
void cxa_posix_gpioConsole_init_output(cxa_posix_gpioConsole_t *const gpioIn, const char *nameIn, const bool initValIn);
void cxa_posix_gpioConsole_init_safe(cxa_posix_gpioConsole_t *const gpioIn, const char *nameIn);


#endif // CXA_POSIX_GPIOCONSOLE_H_
