/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include <cxa_timeBase.h>


// ******** includes ********
#include <cxa_assert.h>
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


uint32_t cxa_timeBase_getCount_us(void)
{
	struct timeval ts;
	current_utc_time(&ts);
	return (1000000 * ts.tv_sec) + (ts.tv_usec);
}


uint32_t cxa_timeBase_getMaxCount_us(void)
{
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
		ts->tv_usec = mts.tv_nsec / 1000;
	#else
		gettimeofday(ts, NULL);
	#endif
}
