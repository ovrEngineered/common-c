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
#ifndef CXA_ONESHOTTIMER_H_
#define CXA_ONESHOTTIMER_H_


// ******** includes ********
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_oneShotTimer cxa_oneShotTimer_t;


typedef void (*cxa_oneShotTimer_cb_t)(void *const userVarIn);


/**
 * @private
 */
struct cxa_oneShotTimer
{
	bool isActive;

	cxa_timeDiff_t timeDiff;
	uint32_t delay_ms;


	cxa_oneShotTimer_cb_t cb;
	void* userVar;
};


// ******** global function prototypes ********
void cxa_oneShotTimer_init(cxa_oneShotTimer_t *const ostIn, int threadIdIn);

void cxa_oneShotTimer_schedule(cxa_oneShotTimer_t *const ostIn, uint32_t delay_msIn, cxa_oneShotTimer_cb_t cbIn, void *const userVarIn);

#endif
