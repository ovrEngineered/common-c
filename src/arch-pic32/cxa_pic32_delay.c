/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_delay.h"


// ******** includes ********
#include "system_definitions.h"


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_delay_ms(uint16_t delay_msIn)
{
	vTaskDelay(delay_msIn / portTICK_PERIOD_MS);
}


// ******** local function implementations ********
