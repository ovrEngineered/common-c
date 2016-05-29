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
#ifndef CXA_AWCU300_TIMER32_H_
#define CXA_AWCU300_TIMER32_H_


// ******** includes ********
#include <stdint.h>

#include <board.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_awcu300_timer32_t object
 */
typedef struct cxa_awcu300_timer32 cxa_awcu300_timer32_t;


/**
 * @private
 */
struct cxa_awcu300_timer32
{
	GPT_ID_Type id;
	uint8_t clockDiv;
};


// ******** global function prototypes ********
/**
 *
 * @param[in] timerIn pointer to the pre-allocated timer object
 * @param[in] timerIdIn the desired GPT unit id
 * @param[in] clockDivIn desired clock divider used as follows:
 * 		clkRate = SystemClockRate / (2^clockDivIn)
 * 		where clockDivIn [0:1:15]
 */
void cxa_awcu300_timer32_init_freerun(cxa_awcu300_timer32_t *const timerIn, GPT_ID_Type timerIdIn, uint8_t clockDivIn);


/**
 * @public
 * @brief Returns the current count of the timer.
 *
 * @param[in] timerIn pointer to the pre-initialized timer object
 * 
 * @return the current count of the timer
 */
uint32_t cxa_awcu300_timer32_getCount(cxa_awcu300_timer32_t *const timerIn);


/**
 * @public
 * @brief Returns the number of counts per second for this timer and configured clock source.
 *
 * @param[in] timerIn pointer to the pre-initialized timer object
 *
 * @return the number of counts per second
 */
uint32_t cxa_awcu300_timer32_getResolution_cntsPerS(cxa_awcu300_timer32_t *const timerIn);


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
uint32_t cxa_awcu300_timer32_getMaxVal_cnts(cxa_awcu300_timer32_t *const timerIn);


#endif // CXA_AWCU300_TIMER32_H_
