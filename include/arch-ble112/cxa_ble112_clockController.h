/**
 * @file
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
 * @copyright 2013-2014 opencxa.org
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
#ifndef CXA_BLE112_CLOCK_CONTROLLER_H_
#define CXA_BLE112_CLOCK_CONTROLLER_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef enum
{
	CXA_BLE112_CLOCK_POWERMODE_ACTIVE,
	CXA_BLE112_CLOCK_POWERMODE_IDLE,
	CXA_BLE112_CLOCK_POWERMODE_PM1,
	CXA_BLE112_CLOCK_POWERMODE_PM2,
	CXA_BLE112_CLOCK_POWERMODE_PM3,
}cxa_ble112_clockController_powerMode_t;


typedef enum
{
	CXA_BLE112_CLOCK_SPEED_UNKNOWN=0x08,
	CXA_BLE112_CLOCK_SPEED_32MHZ=0x00,
	CXA_BLE112_CLOCK_SPEED_16MHZ=0x01,
	CXA_BLE112_CLOCK_SPEED_8MHZ=0x02,
	CXA_BLE112_CLOCK_SPEED_4MHZ=0x03,
	CXA_BLE112_CLOCK_SPEED_2MHZ=0x04,
	CXA_BLE112_CLOCK_SPEED_1MHZ=0x05,
	CXA_BLE112_CLOCK_SPEED_500KHZ=0x06,
	CXA_BLE112_CLOCK_SPEED_250KHZ=0x07,
}cxa_ble112_clockController_clockSpeed_t;


// ******** global function prototypes ********
void cxa_ble112_clockController_setPowerMode(cxa_ble112_clockController_powerMode_t modeIn);
cxa_ble112_clockController_powerMode_t cxa_ble112_clockController_getPowerMode(void);

void cxa_ble112_clockController_setClockSpeed(cxa_ble112_clockController_clockSpeed_t speedIn);
cxa_ble112_clockController_clockSpeed_t cxa_ble112_clockController_getClockSpeed(void);
float cxa_ble112_clockController_getClockSpeed_hz(void);


#endif // CXA_BLE112_CLOCK_CONTROLLER_H_
