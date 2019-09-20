/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_nvsManager.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_stringUtils.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_nvsManager_get_cString_withDefault(const char *const keyIn, char *const valueOut, size_t maxOutputSize_bytes, char *const defaultIn)
{
	if( !cxa_nvsManager_get_cString(keyIn, valueOut, maxOutputSize_bytes) )
	{
		cxa_stringUtils_copy(valueOut, defaultIn, maxOutputSize_bytes);
	}
}


void cxa_nvsManager_get_uint8_withDefault(const char *const keyIn, uint8_t *const valueOut, uint8_t defaultIn)
{
	if( !cxa_nvsManager_get_uint8(keyIn, valueOut) )
	{
		if( valueOut != NULL ) *valueOut = defaultIn;
	}
}


void cxa_nvsManager_get_uint32_withDefault(const char *const keyIn, uint32_t *const valueOut, uint32_t defaultIn)
{
	if( !cxa_nvsManager_get_uint32(keyIn, valueOut) )
		{
			if( valueOut != NULL ) *valueOut = defaultIn;
		}
}


// ******** local function implementations ********
