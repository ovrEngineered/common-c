/**
 * @file
 * This file contains prototypes and an implementation for an XMega 16-bit timer/counter unit.
 *
 * @note This file contains functionality restricted to the CXA Atmel XMega implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // initializes and starts the counter in free-run mode (continuous counting)
 * cxa_xmega_timer16_init_freerun(CXA_XMEGA_TIMER16_TCC1, CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV8);
 *
 * // get the current value of the timer
 * uint16_t value = cxa_xmega_timer16_getCount(CXA_XMEGA_TIMER16_TCC1);
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
#ifndef CXA_XMEGA_TIMER16_H_
#define CXA_XMEGA_TIMER16_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief Enumeration containing all possible clock sources for a timer
 */
typedef enum
{
	CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK=0x01,								///< Peripheral/system clock, no divisor
	CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV2=0x02,						///< Peripheral/system clock / 2
	CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV4=0x03,						///< Peripheral/system clock / 4
	CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV8=0x04,						///< Peripheral/system clock / 8
	CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV64=0x05,						///< Peripheral/system clock / 64
	CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV256=0x06,						///< Peripheral/system clock / 256
	CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV1024=0x07,						///< Peripheral/system clock / 1024
	CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_0=0x08,						///< Timer is clocked by an event on channel 0
	CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_1=0x09,						///< Timer is clocked by an event on channel 1
	CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_2=0x0A,						///< Timer is clocked by an event on channel 2
	CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_3=0x0B,						///< Timer is clocked by an event on channel 3
	CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_4=0x0C,						///< Timer is clocked by an event on channel 4
	CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_5=0x0D,						///< Timer is clocked by an event on channel 5
	CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_6=0x0E,						///< Timer is clocked by an event on channel 6
	CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_7=0x0F						///< Timer is clocked by an event on channel 7
}cxa_xmega_timer16_clockSource_t;


/**
 * @public
 * @brief Enumeration identifying each timer/counter unit on the processor
 */
typedef enum
{
	CXA_XMEGA_TIMER16_TCC0,												///< PORTC, Timer 0
	CXA_XMEGA_TIMER16_TCC1,												///< PORTC, Timer 1
	CXA_XMEGA_TIMER16_TCD0,												///< PORTD, Timer 0
	CXA_XMEGA_TIMER16_TCD1,												///< PORTD, Timer 1
	CXA_XMEGA_TIMER16_TCE0,												///< PORTE, Timer 0
	CXA_XMEGA_TIMER16_TCE1,												///< PORTE, Timer 1
	CXA_XMEGA_TIMER16_TCF0												///< PORTF, Timer 0
}cxa_xmega_timer16_t;


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified timer in free-run mode with the given clock source.
 * In free-run mode, the timer will simply count up to 65535 at the rate specified
 * by the clock source then reset to 0 (and keep counting)
 *
 * @param[in] timerIn the timer to initialize/configure
 * @param[in] clkSourceIn the source that will cause the timer to count
 */
void cxa_xmega_timer16_init_freerun(const cxa_xmega_timer16_t timerIn, cxa_xmega_timer16_clockSource_t clkSourceIn);


/**
 * @public
 * @brief Returns the current count of the timer.
 *
 * @param[in] timerIn the target timer
 * 
 * @return the current count of the timer
 */
uint16_t cxa_xmega_timer16_getCount(const cxa_xmega_timer16_t timerIn);


/**
 * @public
 * @brief Returns the TC0_t / TC1_t struct associated with this timer.
 *
 * @param[in] timerIn the target timer
 *
 * @return pointer to the TC0_t / TC1_t struct for this timer. If passed
 *		an unknown timer, this function will assert / halt execution.
 */
void* cxa_xmega_timer16_getAvrTc(const cxa_xmega_timer16_t timerIn);


/**
 * @public
 * @brief Returns the number of counts per second for this timer and configured clock source.
 *
 * @param[in] timerIn the target timer
 *
 * @return the number of counts per second
 */
uint32_t cxa_xmega_timer16_getResolution_cntsPerS(const cxa_xmega_timer16_t timerIn);


/**
 * @public
 * @brief Returns the maximum possible value of the timer.
 * For most cases this value will be 65535. In some cases where the PER
 * register of the timer has been set lower, this function will return that value.
 *
 * @param[in] timerIn the target timer
 *
 * @return the value of the PER register
 */
uint16_t cxa_xmega_timer16_getMaxVal_cnts(const cxa_xmega_timer16_t timerIn);


#endif // CXA_XMEGA_TIMER16_H_
