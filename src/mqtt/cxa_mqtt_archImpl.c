/**
 * Copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_mqtt_archImpl.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********
static cxa_timeBase_t* timeBase = NULL;


// ******** global function implementations ********
void cxa_mqtt_archImpl_init(cxa_timeBase_t *const timeBaseIn)
{
	timeBase = timeBaseIn;
}


void InitTimer(Timer* timerIn)
{
	cxa_timeDiff_init(&timerIn->timeDiff, timeBase, true);
	timerIn->timeout_ms = 0;
}


char expired(Timer* timerIn)
{
	return cxa_timeDiff_isElapsed_ms(&timerIn->timeDiff, timerIn->timeout_ms);
}


void countdown_ms(Timer* timerIn, unsigned int timeout_msIn)
{
	timerIn->timeout_ms = timeout_msIn;
}


void countdown(Timer* timerIn, unsigned int timeout_sIn)
{
	timerIn->timeout_ms = timeout_sIn * 1000;
}


int left_ms(Timer* timerIn)
{
	return timerIn->timeout_ms - cxa_timeDiff_getElapsedTime_ms(&timerIn->timeDiff);
}


void NewNetwork(Network*)
{

}


int ConnectNetwork(Network*, char*, int)
{

}


void cxa_disconnect(Network*)
{

}


int cxa_read(Network*, unsigned char*, int, int)
{

}


int cxa_write(Network*, unsigned char*, int, int)
{

}


// ******** local function implementations ********
