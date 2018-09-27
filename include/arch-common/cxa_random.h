/**
 * @file
 * @copyright 2018 opencxa.org
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
#ifndef CXA_RANDOM_H_
#define CXA_RANDOM_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 * @brief Generates a *random* number between the provided
 * lower and upper limit (inclusive)
 *
 * Random numbers here are provided to the best of the ability
 * of the platform / architecture and may not be truly random.
 *
 * @param[in] lowerLimitIn lower limit of the output
 * @param[in] uperLimitIn upper limit of the output
 */
uint32_t cxa_random_numberInRange(uint32_t lowerLimitIn, uint32_t upperLimitIn);


#endif
