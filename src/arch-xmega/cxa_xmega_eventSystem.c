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
#include "cxa_xmega_eventSystem.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <avr/io.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	cxa_xmega_eventSystem_eventChannel_t chan;
	bool isUsed;
}isChanUsed_map_entry_t;


// ******** local function prototypes ********
static register8_t* getMuxByChannel(const cxa_xmega_eventSystem_eventChannel_t chanIn);
static register8_t* getCtrlByChannel(const cxa_xmega_eventSystem_eventChannel_t chanIn);
static void markChannelUsed(const cxa_xmega_eventSystem_eventChannel_t chanIn);


// ********  local variable declarations *********
static isChanUsed_map_entry_t isChanUsedMap[] = {
	{ CXA_XMEGA_EVENT_CHAN_0, false },
	{ CXA_XMEGA_EVENT_CHAN_1, false },
	{ CXA_XMEGA_EVENT_CHAN_2, false },
	{ CXA_XMEGA_EVENT_CHAN_3, false },
	{ CXA_XMEGA_EVENT_CHAN_4, false },
	{ CXA_XMEGA_EVENT_CHAN_5, false },
	{ CXA_XMEGA_EVENT_CHAN_6, false },
	{ CXA_XMEGA_EVENT_CHAN_7, false },
};


// ******** global function implementations ********
void cxa_xmega_eventSystem_initChannel_timerEvent(const cxa_xmega_eventSystem_eventChannel_t chanIn, const cxa_xmega_timer16_t timerIn, const cxa_xmega_eventSystem_timerEvent_t timerEvIn)
{
	cxa_assert( (chanIn == CXA_XMEGA_EVENT_CHAN_0) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_1) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_2) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_3) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_4) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_5) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_6) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_7) );
	cxa_assert( (timerEvIn == CXA_XMEGA_EVENT_SOURCE_TC_OVERFLOW) );
	
	// make sure we have power to event system
	if( (PR.PRGEN & (PR_EVSYS_bm)) ) PR.PRGEN &= ~(PR_EVSYS_bm);
	
	// setup our event source mux
	uint8_t muxVal = 0;
	switch( timerIn )
	{
		case CXA_XMEGA_TIMER16_TCC0:
			muxVal = 0xC0 | timerEvIn;
			break;
		
		case CXA_XMEGA_TIMER16_TCC1:
			muxVal = 0xC8 | timerEvIn;
			break;
		
		case CXA_XMEGA_TIMER16_TCD0:
			muxVal = 0xD0 | timerEvIn;
			break;
		
		case CXA_XMEGA_TIMER16_TCD1:
			muxVal = 0xD8 | timerEvIn;
			break;
		
		case CXA_XMEGA_TIMER16_TCE0:
			muxVal = 0xD0 | timerEvIn;
			break;
		
		case CXA_XMEGA_TIMER16_TCE1:
			muxVal = 0xE8 | timerEvIn;
			break;
		
		case CXA_XMEGA_TIMER16_TCF0:
			muxVal = 0xF0 | timerEvIn;
			break;
			
		default:
			cxa_assert(0);
			break;
	}
	*getMuxByChannel(chanIn) = muxVal;
	
	// no special settings
	*getCtrlByChannel(chanIn) = 0x00;
	
	// mark our channel as used
	markChannelUsed(chanIn);
}


void cxa_xmega_eventSystem_initChannel_manualOnly(const cxa_xmega_eventSystem_eventChannel_t chanIn)
{
	cxa_assert( (chanIn == CXA_XMEGA_EVENT_CHAN_0) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_1) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_2) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_3) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_4) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_5) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_6) ||
				(chanIn == CXA_XMEGA_EVENT_CHAN_7) );
	
	// make sure we have power to event system
	if( (PR.PRGEN & (PR_EVSYS_bm)) ) PR.PRGEN &= ~(PR_EVSYS_bm);
	
	// easy defaults
	*getMuxByChannel(chanIn) = 0x00;
	*getCtrlByChannel(chanIn) = 0x00;
				
	// mark our channel as used
	markChannelUsed(chanIn);
}	


void cxa_xmega_eventSystem_triggerEvents(uint8_t eventsToTrigger)
{
	EVSYS.STROBE = eventsToTrigger;
}


cxa_xmega_eventSystem_eventChannel_t cxa_xmega_eventSystem_getUnusedEventChannel(void)
{
	for( size_t i = 0; i < (sizeof(isChanUsedMap)/sizeof(*isChanUsedMap)); i++ )
	{
		isChanUsed_map_entry_t *currEntry = &isChanUsedMap[i];
		if( !currEntry->isUsed ) return currEntry->chan;
	}
	
	// if we made it here, we ran out of event channels
	cxa_assert(0);
	return 0;
}


// ******** local function implementations ********
static register8_t* getMuxByChannel(const cxa_xmega_eventSystem_eventChannel_t chanIn)
{
	register8_t* retVal = NULL;
	
	switch( chanIn )
	{
		case CXA_XMEGA_EVENT_CHAN_0:
			retVal = &EVSYS.CH0MUX;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_1:
			retVal = &EVSYS.CH1MUX;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_2:
			retVal = &EVSYS.CH2MUX;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_3:
			retVal = &EVSYS.CH3MUX;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_4:
			retVal = &EVSYS.CH4MUX;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_5:
			retVal = &EVSYS.CH5MUX;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_6:
			retVal = &EVSYS.CH6MUX;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_7:
			retVal = &EVSYS.CH7MUX;
			break;
			
		default:
			cxa_assert(0);
			break;
	}
	
	return retVal;
}


static register8_t* getCtrlByChannel(const cxa_xmega_eventSystem_eventChannel_t chanIn)
{
	register8_t* retVal = NULL;
	
	switch( chanIn )
	{
		case CXA_XMEGA_EVENT_CHAN_0:
			retVal = &EVSYS.CH0CTRL;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_1:
			retVal = &EVSYS.CH1CTRL;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_2:
			retVal = &EVSYS.CH2CTRL;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_3:
			retVal = &EVSYS.CH3CTRL;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_4:
			retVal = &EVSYS.CH4CTRL;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_5:
			retVal = &EVSYS.CH5CTRL;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_6:
			retVal = &EVSYS.CH6CTRL;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_7:
			retVal = &EVSYS.CH7CTRL;
			break;
		
		default:
			cxa_assert(0);
			break;
	}
	
	return retVal;
}


static void markChannelUsed(const cxa_xmega_eventSystem_eventChannel_t chanIn)
{
	for( size_t i = 0; i < (sizeof(isChanUsedMap)/sizeof(*isChanUsedMap)); i++ )
	{
		isChanUsed_map_entry_t *currEntry = &isChanUsedMap[i];
		if( currEntry->chan == chanIn )
		{
			currEntry->isUsed = true;
			return;
		}
	}
	
	// if we made it here, we have an unknown channel
	cxa_assert(0);
}