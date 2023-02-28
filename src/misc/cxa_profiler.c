/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_profiler.h"


// ******** includes ********
#include <cxa_assert.h>
#include <string.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_profiler_init(cxa_profiler_t *const profIn, const char *nameIn)
{
	cxa_assert(profIn);

	cxa_logger_init_formattedString(&profIn->logger, "prof-%s", nameIn);
}


void cxa_profiler_step_impl(cxa_profiler_t *const profIn, const char* fileIn, const int lineNumIn)
{
	cxa_assert(profIn);

	uint32_t currTime_us = cxa_timeBase_getCount_us();

	// shorten our file name
	char *file_sep = strrchr(fileIn, '/');
	if(file_sep) fileIn = file_sep+1;
	else{
		file_sep = strrchr(fileIn, '\\');
		if (file_sep) fileIn = file_sep+1;
	}
	cxa_logger_debug(&profIn->logger, "%s::%d - %.3fs", fileIn, lineNumIn, ((float)(currTime_us - profIn->lastTime_us)) / 1.0E6);

	profIn->lastTime_us = currTime_us;
}


// ******** local function implementations ********
