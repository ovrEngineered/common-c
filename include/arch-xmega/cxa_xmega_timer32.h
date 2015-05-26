/**
 * @file
 * This file contains prototypes and an implementation of a 32-bit timer object.
 * The 32-bit timer is realized by chaining two XMega 16-bit timers using the overflow
 * event of the lower timer as the clock source for the upper timer. To read the
 * full 32-bit value, two event channels linked to a capture channel on each timer are
 * used to read the instantaneous/atomic timer value.
 *
 * @note This file contains functionality restricted to the CXA Atmel XMega implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_xmega_timer32_t myTimer;
 * // chain PORTC Timer 0 and PORTC Timer 1, using peripheral clock / 1024 as a clock source
 * cxa_xmega_timer32_init_freerun(&myTimer, CXA_XMEGA_TIMER16_TCC0, CXA_XMEGA_TIMER16_TCC1, CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV1024);
 *
 * // get the current value of the timer
 * uint32_t value = cxa_xmega_timer16_getCount(&mytimer);
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
#ifndef CXA_XMEGA_TIMER32_H_
#define CXA_XMEGA_TIMER32_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_xmega_timer16.h>
#include <cxa_xmega_timer16_captureChannel.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_xmega_timer32_t object
 */
typedef struct cxa_xmega_timer32 cxa_xmega_timer32_t;


/**
 * @private
 */
struct cxa_xmega_timer32
{
	cxa_xmega_timer16_t timer_lower;
	cxa_xmega_timer16_t timer_upper;
	
	cxa_xmega_timer16_captureChannel_t cc_lower;
	cxa_xmega_timer16_captureChannel_t cc_upper;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified timer in free-run mode with the given clock source.
 * In free-run mode, the timer will simply count up to 2^32-1 at the rate specified
 * by the clock source then reset to 0 (and keep counting)
 *
 * @param[in] timerIn pointer to the pre-allocated timer object
 * @param[in] timer_lowerIn 16-bit XMega timer that will be used as the lower timer
 * @param[in] timer_upperIn 16-bit XMega timer that will be used as the upper timer
 * @param[in] clkSourceIn the source that will cause the timer to count
 */
void cxa_xmega_timer32_init_freerun(cxa_xmega_timer32_t *const timerIn, const cxa_xmega_timer16_t timer_lowerIn, const cxa_xmega_timer16_t timer_upperIn, cxa_xmega_timer16_clockSource_t clkSourceIn);


/**
 * @public
 * @brief Returns the current count of the timer.
 *
 * @param[in] timerIn pointer to the pre-initialized timer object
 * 
 * @return the current count of the timer
 */
uint32_t cxa_xmega_timer32_getCount(cxa_xmega_timer32_t *const timerIn);


/**
 * @public
 * @brief Returns the number of counts per second for this timer and configured clock source.
 *
 * @param[in] timerIn pointer to the pre-initialized timer object
 *
 * @return the number of counts per second
 */
uint32_t cxa_xmega_timer32_getResolution_cntsPerS(cxa_xmega_timer32_t *const timerIn);


/**
 * @public
 * @brief Returns the maximum possible value of the timer.
 * For most cases this value will be 2^32-1. In some cases where the PER
 * register of the composite 16-bite timers have been set lower,
 * this function will return the result of combining the composite PER registers.
 *
 * @param[in] timerIn pointer to the pre-initialized timer object
 *
 * @return the value of the composite PER register
 */
uint32_t cxa_xmega_timer32_getMaxVal_cnts(cxa_xmega_timer32_t *const timerIn);


#endif // CXA_XMEGA_TIMER32_H_
