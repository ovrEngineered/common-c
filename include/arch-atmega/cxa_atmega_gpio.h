/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ATMEGA_GPIO_H_
#define CXA_ATMEGA_GPIO_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_atmega_gpio_t object
 */
typedef struct cxa_atmega_gpio cxa_atmega_gpio_t;



typedef enum
{
	CXA_ATM_GPIO_PORT_B,
	CXA_ATM_GPIO_PORT_C,
	CXA_ATM_GPIO_PORT_D
}cxa_atmega_gpio_port_t;



/**
 * @private
 */
struct cxa_atmega_gpio
{
	cxa_gpio_t super;

	cxa_gpio_polarity_t polarity;
	cxa_gpio_direction_t dir;

	cxa_atmega_gpio_port_t port;
	uint8_t pinNum;

	bool lastVal;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the GPIO object for immediate use as an input.
 *
 * @param[in] gpioIn pointer to a pre-allocated GPIO object
 * @param[in] portIn the gpio port containing the pin
 * @param[in] pinNumIn pin number describing which pin to use
 * @param[in] polarityIn the polarity of the GPIO pin. See @ref cxa_gpio.h for a discussion of polarity.
 */
void cxa_atmega_gpio_init_input(cxa_atmega_gpio_t *const gpioIn, const cxa_atmega_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn);


/**
 * @public
 * @brief Initializes the GPIO object for immediate use as an output.
 *
 * @param[in] gpioIn pointer to a pre-allocated GPIO object
 * * @param[in] portIn the gpio port containing the pin
 * @param[in] pinNumIn pin number describing which pin to use
 * @param[in] polarityIn the polarity of the GPIO pin. See @ref cxa_gpio.h for a discussion of polarity.
 * @param[in] initValIn the initial logical value of the output pin. This initial value is written
 *		before setting the direction of the pin. Assuming the pin has not been previously initialized
 *		(fresh boot of the processor) it should be configured as a high-impedance input. Thus, the
 *		pin will transition directly from a high-impedance input to the specified logic value.
 */
void cxa_atmega_gpio_init_output(cxa_atmega_gpio_t *const gpioIn, const cxa_atmega_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn);


/**
 * @public
 * @brief Initializes the GPIO object for later use.
 * This function simply stores the pin number.
 * <b>It does not perform any actions which will change the current state/direction/polarity of the pin</b>
 * If a pin is configured using this function, it is recommended to call the following functions before use:
 *     1. cxa_gpio_setPolarity
 *     2. cxa_gpio_setValue			(only if configuring as an output)
 *     3. cxa_gpio_setDirection
 *
 * @param[in] gpioIn pointer to a pre-allocated GPIO object
 * * @param[in] portIn the gpio port containing the pin
 * @param[in] pinNumIn pin number describing which pin to use
 */
void cxa_atmega_gpio_init_safe(cxa_atmega_gpio_t *const gpioIn, const cxa_atmega_gpio_port_t portIn, const uint8_t pinNumIn);


#endif
