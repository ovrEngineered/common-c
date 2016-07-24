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
#include "cxa_ble112_gpio.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>
#include <cxa_array.h>
#include <cxa_config.h>
#include <cxa_criticalSection.h>


// ******** local macro definitions ********
#define CXA_BLE112_GPIO_MAXNUM_INTERRUPTS		4


// ******** local type definitions ********
typedef struct
{
	cxa_ble112_gpio_t* gpio;
	cxa_gpio_interruptType_t intType;
	cxa_gpio_cb_onInterrupt_t cb;
	void* userVar;
}interruptMap_entry_t;


// ******** local function prototypes ********
static void initIntMap(void);
static void setSelToGpio(cxa_ble112_gpio_t *const gpioIn);
static void setEdgeSensitivity(cxa_ble112_gpio_t *const gpioIn, cxa_gpio_interruptType_t intTypeIn);

static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn);
static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn);
static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn);
static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn);
static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn);
static bool scm_getValue(cxa_gpio_t *const superIn);
static bool scm_enableInterrupt(cxa_gpio_t *const superIn, cxa_gpio_interruptType_t intTypeIn, cxa_gpio_cb_onInterrupt_t cbIn, void* userVarIn);


static inline void pinIsr(uint8_t ifgIn);


// ********  local variable declarations *********
static bool isIntMapInit = false;
static cxa_array_t interruptMap;
static interruptMap_entry_t interruptMap_raw[CXA_BLE112_GPIO_MAXNUM_INTERRUPTS];


// ******** global function implementations ********
void cxa_ble112_gpio_init_input(cxa_ble112_gpio_t *const gpioIn, cxa_ble112_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (portIn == CXA_BLE112_GPIO_PORT_0) ||
				(portIn == CXA_BLE112_GPIO_PORT_1) ||
				(portIn == CXA_BLE112_GPIO_PORT_2) );
	cxa_assert(pinNumIn < 8);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	if( !isIntMapInit ) initIntMap();

	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	gpioIn->hasBeenSeld = false;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, scm_enableInterrupt);

	// make sure we're set for GPIO
	setSelToGpio(gpioIn);

	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_ble112_gpio_init_output(cxa_ble112_gpio_t *const gpioIn, cxa_ble112_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (portIn == CXA_BLE112_GPIO_PORT_0) ||
					(portIn == CXA_BLE112_GPIO_PORT_1) ||
					(portIn == CXA_BLE112_GPIO_PORT_2) );
	cxa_assert(pinNumIn < 8);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	if( !isIntMapInit ) initIntMap();

	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	gpioIn->hasBeenSeld = false;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, scm_enableInterrupt);

	// make sure we're set for GPIO
	setSelToGpio(gpioIn);

	// set our initial value (before we set direction to avoid glitches)
	cxa_gpio_setValue(&gpioIn->super, initValIn);

	// now set our direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_ble112_gpio_init_safe(cxa_ble112_gpio_t *const gpioIn, cxa_ble112_gpio_port_t portIn, const uint8_t pinNumIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (portIn == CXA_BLE112_GPIO_PORT_0) ||
					(portIn == CXA_BLE112_GPIO_PORT_1) ||
					(portIn == CXA_BLE112_GPIO_PORT_2) );
	cxa_assert(pinNumIn < 8);

	if( !isIntMapInit ) initIntMap();

	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = CXA_GPIO_POLARITY_NONINVERTED;
	gpioIn->hasBeenSeld = false;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, scm_enableInterrupt);

	// don't set any value or direction...just leave everything as it is
}


// ******** local function implementations ********
static void initIntMap(void)
{
	cxa_array_initStd(&interruptMap, interruptMap_raw);
	isIntMapInit = true;
}


static void setSelToGpio(cxa_ble112_gpio_t *const gpioIn)
{
	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			APCFG &= ~(1 << gpioIn->pinNum);		// analog peripheral config
			P0SEL &= ~(1 << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			P1SEL &= ~(1 << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			P2SEL &= ~(1 << gpioIn->pinNum);
			break;
	}

	gpioIn->hasBeenSeld = true;
}


static void setEdgeSensitivity(cxa_ble112_gpio_t *const gpioIn, cxa_gpio_interruptType_t intTypeIn)
{
	cxa_assert(gpioIn);
	cxa_assert(intTypeIn != CXA_GPIO_INTERRUPTTYPE_ONCHANGE);

	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			PICTL = (PICTL & ~(1 << 0)) | ((intTypeIn == CXA_GPIO_INTERRUPTTYPE_FALLING_EDGE) << 0);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			// edge sensitivity
			if( gpioIn->pinNum < 4 ) PICTL = (PICTL & ~(1 << 1)) | ((intTypeIn == CXA_GPIO_INTERRUPTTYPE_FALLING_EDGE) << 1);
			else PICTL = (PICTL & ~(1 << 2)) | ((intTypeIn == CXA_GPIO_INTERRUPTTYPE_FALLING_EDGE) << 2);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			// edge sensitivity
			PICTL = (PICTL & ~(1 << 3)) | ((intTypeIn == CXA_GPIO_INTERRUPTTYPE_FALLING_EDGE) << 3);
			break;
	}
}


static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	// make sure we're set for GPIO
	setSelToGpio(gpioIn);

	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			if( dirIn == CXA_GPIO_DIR_OUTPUT ) P0DIR |= (1 << gpioIn->pinNum);
			else P0DIR &= ~(1 << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			if( dirIn == CXA_GPIO_DIR_OUTPUT ) P1DIR |= (1 << gpioIn->pinNum);
			else P1DIR &= ~(1 << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			if( dirIn == CXA_GPIO_DIR_OUTPUT ) P2DIR |= (1 << gpioIn->pinNum);
			else P2DIR &= ~(1 << gpioIn->pinNum);
			break;
	}
}


static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	cxa_gpio_direction_t retVal = CXA_GPIO_DIR_UNKNOWN;
	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			retVal = ((P0DIR & (1 << gpioIn->pinNum)) ? CXA_GPIO_DIR_OUTPUT : CXA_GPIO_DIR_INPUT);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			retVal = ((P1DIR & (1 << gpioIn->pinNum)) ? CXA_GPIO_DIR_OUTPUT : CXA_GPIO_DIR_INPUT);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			retVal = ((P2DIR & (1 << gpioIn->pinNum)) ? CXA_GPIO_DIR_OUTPUT : CXA_GPIO_DIR_INPUT);
			break;
	}

	return retVal;
}


static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	gpioIn->polarity = polarityIn;
}


static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	return gpioIn->polarity;
}


static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			P0 = (P0 & ~(1 << gpioIn->pinNum)) |
						(((gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn) << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			P1 = (P1 & ~(1 << gpioIn->pinNum)) |
						(((gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn) << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			P2 = (P2 & ~(1 << gpioIn->pinNum)) |
						(((gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn) << gpioIn->pinNum);
			break;
	}
}


static bool scm_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	bool retVal = false;
	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			retVal = (cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_OUTPUT) ?
					((P0 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum) :
					((P0 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			retVal = (cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_OUTPUT) ?
					((P1 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum) :
					((P1 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			retVal = (cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_OUTPUT) ?
					((P2 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum) :
					((P2 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum);
			break;
	}

	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal : retVal;
}


static bool scm_enableInterrupt(cxa_gpio_t *const superIn, cxa_gpio_interruptType_t intTypeIn, cxa_gpio_cb_onInterrupt_t cbIn, void* userVarIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	// make sure we can properly configure this interrupt
	bool hasOtherPortInterruptsEnabled = false;
	bool doesPreviousEdgeMatchNewEdge = false;
	cxa_array_iterate(&interruptMap, currEntry, interruptMap_entry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->gpio->port == gpioIn->port )
		{
			hasOtherPortInterruptsEnabled = true;
			doesPreviousEdgeMatchNewEdge = (currEntry->intType == intTypeIn);
			break;
		}
	}
	if( hasOtherPortInterruptsEnabled &&
		(!doesPreviousEdgeMatchNewEdge || (intTypeIn == CXA_GPIO_INTERRUPTTYPE_ONCHANGE)) ) return false;

	// setup our interrupt map entry
	interruptMap_entry_t newEntry = {
			.gpio = gpioIn,
			.intType = intTypeIn,
			.cb = cbIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&interruptMap, &newEntry));


	cxa_criticalSection_enter();

	// autoset our interrupt-type if needed
	if( intTypeIn == CXA_GPIO_INTERRUPTTYPE_ONCHANGE )
	{
		intTypeIn = cxa_gpio_getValue(superIn) ? CXA_GPIO_INTERRUPTTYPE_FALLING_EDGE : CXA_GPIO_INTERRUPTTYPE_RISING_EDGE;
	}

	// enable our interrupt
	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			setEdgeSensitivity(gpioIn, intTypeIn);

			// pin interrupt then enable port interrupt
			P0IEN |= (1 << gpioIn->pinNum);
			IEN1 |= (1 << 5);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			setEdgeSensitivity(gpioIn, intTypeIn);

			// pin interrupt then enable port interrupt
			P1IEN |= (1 << gpioIn->pinNum);
			IEN2 |= (1 << 4);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			setEdgeSensitivity(gpioIn, intTypeIn);

			// pin interrupt then enable port interrupt
			P2IEN |= (1 << gpioIn->pinNum);
			IEN2 |= (1 << 1);
			break;
	}

	cxa_criticalSection_exit();

	return true;
}


// ******** interrupt service routines ********
static inline void pinIsr(uint8_t ifgIn)
{
	cxa_array_iterate(&interruptMap, currEntry, interruptMap_entry_t)
	{
		if( currEntry == NULL ) continue;

		if( (currEntry->gpio->port == CXA_BLE112_GPIO_PORT_0) &&
			(ifgIn & (1 << currEntry->gpio->pinNum)) )
		{
			bool currVal = cxa_gpio_getValue(&currEntry->gpio->super);
			cxa_gpio_interruptType_t currIntType = (currVal) ? CXA_GPIO_INTERRUPTTYPE_RISING_EDGE : CXA_GPIO_INTERRUPTTYPE_FALLING_EDGE;

			// call our callback if set
			if( currEntry->cb != NULL )
			{
				currEntry->cb(&currEntry->gpio->super, currIntType, currVal, currEntry->userVar);
			}

			// change our sensitivity if requested
			if( currEntry->intType == CXA_GPIO_INTERRUPTTYPE_ONCHANGE )
			{
				// edge sensitivity
				setEdgeSensitivity(currEntry->gpio, (currIntType == CXA_GPIO_INTERRUPTTYPE_RISING_EDGE) ? CXA_GPIO_INTERRUPTTYPE_FALLING_EDGE :CXA_GPIO_INTERRUPTTYPE_RISING_EDGE);
			}
		}
	}
}


#pragma vector=P0INT_VECTOR
__interrupt void p0_int(void)
{
	pinIsr(P0IFG);

	// must be performed in this order
	P0IFG = 0;
	IRCON &= ~(1 << 5);
}


#pragma vector=P1INT_VECTOR
__interrupt void p1_int(void)
{
	pinIsr(P1IFG);

	// must be performed in this order
	P1IFG = 0;
	IRCON2 &= ~(1 << 3);
}


#pragma vector=P2INT_VECTOR
__interrupt void p2_int(void)
{
	pinIsr(P2IFG);

	// must be performed in this order
	P2IFG = 0;
	IRCON2 &= ~(1 << 0);
}
