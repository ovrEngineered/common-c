/**
 * @file
 * This file contains functions for configuring the internal clock system of the XMega processor.
 *
 * @note This file contains functionality restricted to the CXA Atmel XMega implementation.
 *
 * @note This module only supports internal oscillators for the system clock.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // called once at boot to set our clock speed
 * cxa_xmega_clockController_init(CXA_XMEGA_CC_INTOSC_32MHZ);
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
#ifndef CXA_XMEGA_CLOCK_CONTROLLER_H_
#define CXA_XMEGA_CLOCK_CONTROLLER_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief Enumeration containing possible values for the internal
 *		clock source of the XMega microcontroller
 */
typedef enum
{
	CXA_XMEGA_CC_INTOSC_32MHZ=0x01,											///< Use the internal 32MHz oscillator for our system clock
	CXA_XMEGA_CC_INTOSC_2MHZ=0x00,											///< Use the internal 2MHz oscillator for our system clock
	CXA_XMEGA_CC_INTOSC_32KHZ=0x02											///< Use the internal 32KHz oscillator for our system clock
}cxa_xmega_clockController_internalOsc_t;


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the XMega clock controller system and configures it to use the provided internal oscillator.
 * This function MUST be called BEFORE any other function in this file. It will automatically enable
 * the specified internal oscillator and disable the previously used oscillator.
 *
 * @param[in] oscIn the desired internal oscillator which should be used for our system clock
 */
void cxa_xmega_clockController_init(const cxa_xmega_clockController_internalOsc_t oscIn);


/**
 * @public
 * @brief Changes the system clock to use the provided internal oscillator.
 * This function enables the specified oscillator, if needed, and disables the
 * old, unused oscillator when complete.
 *
 * @param[in] oscIn the desired internal oscillator which should be used for our system clock
 */
void cxa_xmega_clockController_setSystemClockSource_internal(const cxa_xmega_clockController_internalOsc_t oscIn);


/**
 * @public
 * @brief Returns the current system clock frequency, in hertz.
 *
 * @return the system clock frequency, in hertz
 */
uint32_t cxa_xmega_clockController_getSystemClockFrequency_hz(void);


/**
 * @public
 * @brief Enables the specified internal oscillator.
 * Once enabled, this function will busy-wait until the oscillator has stabilized.
 *
 * @param[in] oscIn the desired internal oscillator to enable
 */
void cxa_xmega_clockController_internalOsc_enable(const cxa_xmega_clockController_internalOsc_t oscIn);


/**
 * @public
 * @brief disables the specified internal oscillator.
 *
 * @param[in] oscIn the desired internal oscillator to disable
 */
void cxa_xmega_clockController_internalOsc_disable(const cxa_xmega_clockController_internalOsc_t oscIn);


#endif // CXA_XMEGA_CLOCK_CONTROLLER_H_
