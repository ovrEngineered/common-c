/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_SOFTWATCHDOG_H_
#define CXA_SOFTWATCHDOG_H_


// ******** includes ********
#include <stdint.h>

#include <cxa_timeDiff.h>


// ******** global macro definitions ********


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
void cxa_softWatchDog_init(cxa_softWatchDog_t *const swdIn, uint32_t timeoutPeriod_msIn, int threadIdIn,
						   cxa_softWatchDog_cb_t cbIn, void *const userVarIn);
void cxa_softWatchDog_kick(cxa_softWatchDog_t *const swdIn);
void cxa_softWatchDog_pause(cxa_softWatchDog_t *const swdIn);

bool cxa_softWatchDog_isPaused(cxa_softWatchDog_t *const swdIn);


#endif
