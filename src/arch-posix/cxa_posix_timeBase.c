/**
 * Copyright 2013-2015 opencxa.org
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
 */
#include <cxa_assert.h>
#include <cxa_posix_timeBase.h>
#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void current_utc_time(struct timeval *ts);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_posix_timeBase_init(cxa_timeBase_t *const tbIn)
{
	cxa_assert(tbIn);
	// nothing to do here
}


uint32_t cxa_timeBase_getCount_us(cxa_timeBase_t *const superIn)
{
	cxa_assert(superIn);
	
	struct timeval ts;
	current_utc_time(&ts);
	return (1000000 * ts.tv_sec) + (ts.tv_usec);
}


uint32_t cxa_timeBase_getMaxCount_us(cxa_timeBase_t *const superIn)
{
	cxa_assert(superIn);
	
	return UINT32_MAX;
}


// ******** local function implementations ********
static void current_utc_time(struct timeval *ts)
{
	#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
		clock_serv_t cclock;
		mach_timespec_t mts;
		host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
		clock_get_time(cclock, &mts);
		mach_port_deallocate(mach_task_self(), cclock);
		ts->tv_sec = mts.tv_sec;
		ts->tv_nsec = mts.tv_nsec;
	#else
		gettimeofday(ts, NULL);
	#endif
}

