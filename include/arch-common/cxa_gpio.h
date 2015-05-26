/**
 * @file
 * This file contains prototypes and a top-level implementation of a general-purpose input/output (GPIO) object.
 * A GPIO object is used to read and write a single digital IO line.
 *
 * @note This file contains the base functionality for a GPIO object available across all architectures. Additional
 *		functionality, including initialization is available in the architecture-specific implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_<arch>_gpio_t myGpio;
 * // initialization for architecture specific implementation
 *
 * ...
 *
 * // ensure the GPIO object is an input, then read the value
 * cxa_gpio_setDirection(&myGpio, CXA_GPIO_DIR_INPUT);
 * bool currVal = cxa_gpio_getValue(&myGpio);
 *
 * ...
 *
 * // ensure the GPIO object is an output, then toggle its value
 * cxa_gpio_setDirection(&myGpio, CXA_GPIO_DIR_OUTPUT);
 * cxa_gpio_toggle(&myGpio);
 * @endcode
 *
 *
 * @anchor polarity
 * #### Polarity: ####
 * Polarity of a GPIO object is essentially a mapping of logical '0'/'1' to
 * electrical '0'/'1'. Sometimes it is useful to think of a GPIO object as
 * being on, for example, a RESET line for an external peripheral. To logically
 * assert the reset line in code:
 * @code
 * cxa_gpio_setValue(&gpio_reset, 1);
 * @endcode
 *
 * For situations where an electrical '1' will reset the peripheral device,
 * we'd use the standard mapping of ::CXA_GPIO_POLARITY_NONINVERTED (eg. logical
 * '1' --> electrical '1'). For situations where an electrical '0' is required
 * to actually reset the peripheral device, we'd use the inverted mapping of
 * ::CXA_GPIO_POLARITY_INVERTED (eg. logical '1' --> electrical '0').
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
#ifndef CXA_GPIO_H_
#define CXA_GPIO_H_


// ******** includes ********
#include <stdbool.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_gpio_t object
 */
typedef struct cxa_gpio cxa_gpio_t;


/**
 * @public
 * @brief Enumeration encapsulating the possible directions for a GPIO object
 */
typedef enum
{
	CXA_GPIO_DIR_UNKNOWN,								///< Direction is unknown
	CXA_GPIO_DIR_INPUT,									///< GPIO object should read from the GPIO pin (pin is generally not driving any current)
	CXA_GPIO_DIR_OUTPUT									///< GPIO object should write to the GPIO pin (pin is generally driving a '0' or a '1')
}cxa_gpio_direction_t;


/**
 * @public
 * @brief Enumeration encapsulating the possible polarity mappings for GPIO object
 * See @ref polarity "Polarity"
 */
typedef enum
{
	CXA_GPIO_POLARITY_NONINVERTED,						///< Polarity is non-inverted (logical '1' --> electrical '1')
	CXA_GPIO_POLARITY_INVERTED							///< Polarity is inverted (logical '1' --> electrical '0')
}cxa_gpio_polarity_t;


/**
 * @private
 */
struct cxa_gpio
{
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Sets the direction of the GPIO object/pin
 *
 * @param[in] gpioIn pointer to a pre-initialized GPIO object
 * @param[in] dirIn the direction of the GPIO pin (input/output, reading/writing)
 */
void cxa_gpio_setDirection(cxa_gpio_t *const gpioIn, const cxa_gpio_direction_t dirIn);


/**
 * @public
 * @brief Returns the direction of the GPIO object/pin
 *
 * @param[in] gpioIn pointer to a pre-initialized GPIO object
 *
 * @return the direction of the GPIO pin (input/output, reading/writing)
 */
cxa_gpio_direction_t cxa_gpio_getDirection(cxa_gpio_t *const gpioIn);


/**
 * @public
 * @brief Sets the polarity of the GPIO object/pin
 * See @ref polarity "Polarity"
 *
 * @param[in] gpioIn pointer to a pre-initialized GPIO object
 * @param[in] polarityIn the polarity of the GPIO pin
 */
void cxa_gpio_setPolarity(cxa_gpio_t *const gpioIn, const cxa_gpio_polarity_t polarityIn);


/**
 * @public
 * @brief Returns the polarity of the GPIO object/pin
 * See @ref polarity "Polarity"
 *
 * @param[in] gpioIn pointer to a pre-initialized GPIO object
 * @return the the polarity of the GPIO pin
 */
cxa_gpio_polarity_t cxa_gpio_getPolarity(cxa_gpio_t *const gpioIn);


/**
 * @public
 * @brief Sets the output value of the GPIO object/pin
 * If the GPIO object is configured as an output, will immediately
 * drive the value (after polarity adjustment) on the pin.
 * If the GPIO object is configured as an input, will store the value
 * (after polarity adjustment) internally should the GPIO object
 * eventually become an output. If the GPIO object is configured
 * with ::CXA_GPIO_POLARITY_NONINVERTED, the provided value will
 * be used without modification. If the GPIO object is configured
 * with ::CXA_GPIO_POLARITY_INVERTED, the provided value will
 * be inverted, then used.
 *
 * @param[in] gpioIn pointer to a pre-initialized GPIO object
 * @param[in] valIn the logical boolean value to write to the GPIO object/pin
 */
void cxa_gpio_setValue(cxa_gpio_t *const gpioIn, const bool valIn);


/**
 * @public
 * @brief Returns the input value of the GPIO object/pin
 * If the GPIO object is configured as an output, will return the
 * last value written to the pin (after polarity adjustment).
 * If the GPIO object is configured as an input, will read the
 * electrical state of the GPIO pin and adjust for polarity.
 * If the GPIO object is configured with ::CXA_GPIO_POLARITY_NONINVERTED,
 * the read value will be returned without modification. If the GPIO
 * object is configured with ::CXA_GPIO_POLARITY_INVERTED, the read
 * value will be inverted, then returned.
 *
 * @param[in] gpioIn pointer to a pre-initialized GPIO object
 * 
 * @return the logical boolean value of GPIO object/pin
 */
bool cxa_gpio_getValue(cxa_gpio_t *const gpioIn);

/**
 * @public
 * @brief Toggles the output value of the GPIO object/pin
 * This is essentially a convenience function for the following code:
 * @code 
 * cxa_gpio_setValue(gpioIn, !cxa_gpio_getValue(gpioIn));
 * @endcode
 *
 * @param[in] gpioIn pointer to a pre-initialized GPIO object
 */
void cxa_gpio_toggle(cxa_gpio_t *const gpioIn);


/**
 * @public
 * @brief Returns a user-friendly text description of the directional state of the GPIO object/pin
 *
 * @param[in] gpioIn pointer to a pre-initialized GPIO object
 *
 * @return pointer to a user-friendly text description of the directional state of the GPIO object/pin
 */
const char* cxa_gpio_direction_toString(const cxa_gpio_direction_t dirIn);


#endif // CXA_GPIO_H_
