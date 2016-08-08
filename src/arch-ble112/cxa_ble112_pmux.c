/**
 * @copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_ble112_pmux.h"


// ******** includes ********
#include <blestack/config.h>
#include <blestack/hw.h>


// ******** local macro definitions ********
#define PMUX_DREGSTA    BIT(3)
#define PICTL_PADSC     BIT(7)//IO pin drive strength


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ble112_pmux_init(void)
{
	/**
	 * Read configuration from flash only if valid
	 */
	if( !(config_flags & HWCONFIG_INVALID) )
	{
		HAL_BANK(hwconfig_page,{
				// SETUP PMUX configuration
				PMUX = ~hwdata.pmux;
				if( PMUX & PMUX_DREGSTA )
				{
					// if regulator status is output, then assumes that dc-dc is connected and vdd is less than 2.6v
					// -> enable strong pull-ups
					PICTL |= PICTL_PADSC;
				}
		});
	}
	// put pmux output pin to down
	if( PMUX & BIT(3) )
	{
		uint8 pin = BIT( PMUX & 0x7 );
		// pull down
		P1 &= ~pin;
		// put to output
		P1DIR |= pin;
	}
}


// ******** local function implementations ********
