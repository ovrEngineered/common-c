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
#ifndef CXA_SOFTWATCHDOG_H_
#define CXA_SOFTWATCHDOG_H_


// ******** includes ********
#include <stdint.h>

#include <cxa_timeDiff.h>


// ******** global macro definitions ********
#ifndef CXA_SOFTWATCHDOG_MAXNUM_ENTRIES
	#define CXA_SOFTWATCHDOG_MAXNUM_ENTRIES				10
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_softWatchDog cxa_softWatchDog_t;


/**
 * @public
 */
typedef void (*cxa_softWatchDog_cb_t)(void* userVarIn);


/**
 * @private
 */
struct cxa_softWatchDog
{
	cxa_softWatchDog_cb_t cb;
	void* userVar;

	bool isPaused;
	uint32_t timeoutPeriod_ms;
	cxa_timeDiff_t td_timeout;
};


// ******** global function prototypes ********
void cxa_softWatchDog_init(cxa_softWatchDog_t *const swdIn, uint32_t timeoutPeriod_msIn, cxa_softWatchDog_cb_t cbIn, void *const userVarIn);
void cxa_softWatchDog_kick(cxa_softWatchDog_t *const swdIn);
void cxa_softWatchDog_pause(cxa_softWatchDog_t *const swdIn);

bool cxa_softWatchDog_isPaused(cxa_softWatchDog_t *const swdIn);


#endif
