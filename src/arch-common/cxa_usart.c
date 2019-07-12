/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_usart.h"


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
cxa_ioStream_t* cxa_usart_getIoStream(cxa_usart_t* usartIn)
{
	cxa_assert(usartIn);

	return &usartIn->ioStream;
}


// ******** local function implementations ********
