/**
 * @file
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
 * @copyright 2013-2014 opencxa.org
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
#ifndef CXA_BLE112_GPIO_H_
#define CXA_BLE112_GPIO_H_


// ******** includes ********
#include <cxa_gpio.h>
#include <stdint.h>
#include <blestack/hw_regs.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_ble112_gpio_t object
 */
typedef struct cxa_ble112_gpio cxa_ble112_gpio_t;


typedef enum
{
	CXA_BLE112_GPIO_PORT_0,
	CXA_BLE112_GPIO_PORT_1,
	CXA_BLE112_GPIO_PORT_2
}cxa_ble112_gpio_port_t;


/**
 * @private
 */
struct cxa_ble112_gpio
{
	cxa_gpio_t super;
	
	cxa_gpio_polarity_t polarity;

	cxa_ble112_gpio_port_t port;
	uint8_t pinNum;

	bool hasBeenSeld;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the GPIO object for immediate use as an input.
 *
 * @param[in] gpioIn pointer to a pre-allocated GPIO object
 * @param[in] portIn the port containing the target GPIO pin
 * @param[in] pinNumIn 0-based integer describing which pin to use [0-7]
 * @param[in] polarityIn the polarity of the GPIO pin. See @ref cxa_gpio.h for a discussion of polarity.
 */
void cxa_ble112_gpio_init_input(cxa_ble112_gpio_t *const gpioIn, cxa_ble112_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn);


/**
 * @public
 * @brief Initializes the  GPIO object for immediate use as an output.
 *
 * @param[in] gpioIn pointer to a pre-allocated GPIO object
 * @param[in] portIn the port containing the target GPIO pin
 * @param[in] pinNumIn 0-based integer describing which pin to use [0-7]
 * @param[in] polarityIn the polarity of the GPIO pin. See @ref cxa_gpio.h for a discussion of polarity.
 * @param[in] initValIn the initial logical value of the output pin. This initial value is written
 *		before setting the direction of the pin. Assuming the pin has not been previously initialized
 *		(fresh boot of the processor) it should be configured as a high-impedance input. Thus, the
 *		pin will transition directly from a high-impedance input to the specified logic value.
 */
void cxa_ble112_gpio_init_output(cxa_ble112_gpio_t *const gpioIn, cxa_ble112_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn);


/**
 * @public
 * @brief Initializes the GPIO object for later use.
 * This function simply stores the XMega port and pin number.
 * <b>It does not perform any actions which will change the current state/direction/polarity of the pin</b>
 * If a pin is configured using this function, it is recommended to call the following functions before use:
 *     1. cxa_gpio_setPolarity
 *     2. cxa_gpio_setValue			(only if configuring as an output)
 *     3. cxa_gpio_setDirection
 *
 * @param[in] gpioIn pointer to a pre-allocated GPIO object
 * @param[in] portIn the port containing the target GPIO pin
 * @param[in] pinNumIn 0-based integer describing which pin to use [0-7]
 */
void cxa_ble112_gpio_init_safe(cxa_ble112_gpio_t *const gpioIn, cxa_ble112_gpio_port_t portIn, const uint8_t pinNumIn);


#endif // CXA_BLE112_GPIO_H_
