/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
