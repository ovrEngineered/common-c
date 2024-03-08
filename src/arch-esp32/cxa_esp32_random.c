/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_random.h"


// ******** includes ********
#include <esp_random.h>

#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
uint32_t cxa_random_numberInRange(uint32_t lowerLimitIn, uint32_t upperLimitIn)
{
	cxa_assert(lowerLimitIn < upperLimitIn);

	return lowerLimitIn + (esp_random() / (UINT32_MAX / (upperLimitIn - lowerLimitIn + 1) + 1));
}


// ******** local function implementations ********
