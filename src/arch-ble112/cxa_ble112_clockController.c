/**
 * Copyright 2013 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_ble112_clockController.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>
#include <blestack/hw_regs.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ble112_clockController_setPowerMode(cxa_ble112_clockController_powerMode_t modeIn)
{
	switch( modeIn )
	{
		case CXA_BLE112_CLOCK_POWERMODE_IDLE:
			SLEEPCMD = (SLEEPCMD & 0xFC) | 0x00;
			PCON |= 1;
			break;

		case CXA_BLE112_CLOCK_POWERMODE_ACTIVE:
			SLEEPCMD = (SLEEPCMD & 0xFC) | 0x00;
			break;

		case CXA_BLE112_CLOCK_POWERMODE_PM1:
			SLEEPCMD = (SLEEPCMD & 0xFC) | 0x01;
			break;

		case CXA_BLE112_CLOCK_POWERMODE_PM2:
			SLEEPCMD = (SLEEPCMD & 0xFC) | 0x02;
			break;

		case CXA_BLE112_CLOCK_POWERMODE_PM3:
			SLEEPCMD = (SLEEPCMD & 0xFC) | 0x03;
			break;
	}

}


cxa_ble112_clockController_powerMode_t cxa_ble112_clockController_getPowerMode(void)
{
	// we'll never return idle since the clock would be stopped
	// and not executing our code

	uint8_t currMode = SLEEPCMD & 0xFC;
	cxa_ble112_clockController_powerMode_t retVal = CXA_BLE112_CLOCK_POWERMODE_ACTIVE;

	if( currMode == 0x01 ) retVal = CXA_BLE112_CLOCK_POWERMODE_PM1;
	else if( currMode == 0x02 ) retVal = CXA_BLE112_CLOCK_POWERMODE_PM2;
	else if( currMode == 0x03 ) retVal = CXA_BLE112_CLOCK_POWERMODE_PM3;

	return retVal;
}


void cxa_ble112_clockController_setClockSpeed(cxa_ble112_clockController_clockSpeed_t speedIn)
{
	// we'll always want to operate on the 32MHZ XOSC (required for RF)
	// but we'll scale down as requested

	CLKCONCMD = (CLKCONCMD & 0xB8) | (speedIn & 0x07);
	while( (CLKCONSTA & 0x43) != (CLKCONCMD & 0x43) );
}


cxa_ble112_clockController_clockSpeed_t cxa_ble112_clockController_getClockSpeed(void)
{
	return (cxa_ble112_clockController_clockSpeed_t)(CLKCONSTA & 0x07);
}


float cxa_ble112_clockController_getClockSpeed_hz(void)
{
	float retVal = 0;

	switch( cxa_ble112_clockController_getClockSpeed() )
	{
		case CXA_BLE112_CLOCK_SPEED_32MHZ:
			retVal = 32000000.0;
			break;

		case CXA_BLE112_CLOCK_SPEED_16MHZ:
			retVal = 16000000.0;
			break;

		case CXA_BLE112_CLOCK_SPEED_8MHZ:
			retVal = 8000000.0;
			break;

		case CXA_BLE112_CLOCK_SPEED_4MHZ:
			retVal = 4000000.0;
			break;

		case CXA_BLE112_CLOCK_SPEED_2MHZ:
			retVal = 2000000.0;
			break;

		case CXA_BLE112_CLOCK_SPEED_1MHZ:
			retVal = 1000000.0;
			break;

		case CXA_BLE112_CLOCK_SPEED_500KHZ:
			retVal = 500000.0;
			break;

		case CXA_BLE112_CLOCK_SPEED_250KHZ:
			retVal = 250000.0;
			break;

		default: break;
	}

	return retVal;
}


// ******** local function implementations ********

