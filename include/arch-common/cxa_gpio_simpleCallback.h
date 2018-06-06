/**
 * @file
 * @copyright 2016 opencxa.org
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
#ifndef CXA_GPIO_SIMPLECALLBACK_H_
#define CXA_GPIO_SIMPLECALLBACK_H_


// ******** includes ********
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_gpio_simpleCallback_t object
 */
typedef struct cxa_gpio_simpleCallback cxa_gpio_simpleCallback_t;


/**
 @public
 */
typedef bool (*cxa_gpio_simpleCallback_getValue_t)(cxa_gpio_t *const superIn, int userIdIn);
typedef void (*cxa_gpio_simpleCallback_setValue_t)(cxa_gpio_t *const superIn, int userIdIn, bool valueIn);


/**
 * @private
 */
struct cxa_gpio_simpleCallback
{
	cxa_gpio_t super;
	
	cxa_gpio_polarity_t polarity;
	cxa_gpio_direction_t dir;

    int userId;
    cxa_gpio_simpleCallback_getValue_t cb_getValue;
	cxa_gpio_simpleCallback_setValue_t cb_setValue;

	bool lastVal;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the simpleCallback GPIO object for immediate use as an input.
 *
 * @param[in] gpioIn pointer to a pre-allocated pic32 GPIO object
 * @param[in] userIdIn pin number describing which pin to use
 * @param[in] polarityIn the polarity of the GPIO pin. See @ref cxa_gpio.h for a discussion of polarity.
 */
void cxa_gpio_simpleCallback_init_input(cxa_gpio_simpleCallback_t *const gpioIn, const int userIdIn, 
                                        cxa_gpio_simpleCallback_getValue_t cb_getValueIn,
                                        cxa_gpio_simpleCallback_setValue_t cb_setValueIn,
                                        const cxa_gpio_polarity_t polarityIn);

#endif
