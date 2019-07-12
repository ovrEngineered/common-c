/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_delay.h"


// ******** includes ********
#include <stdbool.h>
#include <udelay.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********
static bool isCalibrated = false;


// ******** global function implementations ********
void cxa_delay_ms(uint16_t delay_msIn)
{
	if( !isCalibrated )
	{
		UDELAY_Calibrate();
		isCalibrated = true;
	}

	for( uint16_t i = 0; i < delay_msIn; i++ )
	{
		UDELAY_Delay(1000);
	}
}


// ******** local function implementations ********
