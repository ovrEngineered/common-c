/**
 * @file
 * @copyright 2017 opencxa.org
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
#ifndef CXA_SNTPCLIENT_H_
#define CXA_SNTPCLIENT_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef void (*cxa_sntpClient_cb_onInitialTimeSet_t)(void *const userVarIn);


// ******** global function prototypes ********
void cxa_sntpClient_init(void);
void cxa_sntpClient_addListener(cxa_sntpClient_cb_onInitialTimeSet_t cbIn, void *const userVarIn);
bool cxa_sntpClient_isClockSet(void);

uint32_t cxa_sntpClient_getUnixTimeStamp(void);

#endif
