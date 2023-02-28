/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_PROFILER_H_
#define CXA_PROFILER_H_


// ******** includes ********
#include <cxa_logger_header.h>
#include <cxa_timeBase.h>


// ******** global macro definitions ********
#define cxa_profiler_start(profIn)				(profIn)->lastTime_us = cxa_timeBase_getCount_us()
#define cxa_profiler_step(profIn)				cxa_profiler_step_impl(profIn, __FILE__, __LINE__)


// ******** global type definitions *********
typedef struct {
	cxa_logger_t logger;

	uint32_t lastTime_us;
}cxa_profiler_t;


// ******** global function prototypes ********
void cxa_profiler_init(cxa_profiler_t *const profIn, const char *nameIn);


/**
 * @private
 */
void cxa_profiler_step_impl(cxa_profiler_t *const profIn, const char* fileIn, const int lineNumIn);


#endif
