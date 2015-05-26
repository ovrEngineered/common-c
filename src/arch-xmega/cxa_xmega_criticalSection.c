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
#include "cxa_criticalSection.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <avr/interrupt.h>
#include <cxa_config.h>
#include <cxa_assert.h>
#include <cxa_xmega_pmic.h>
#include <cxa_array.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct  
{
	cxa_criticalSection_cb_t preEnter;
	cxa_criticalSection_cb_t postExit;
	
	void *userVar;
}callback_entry_t;


// ******** local function prototypes ********


// ********  local variable declarations *********
static volatile uint8_t storedSreg;
static volatile uint8_t nestLevels = 0;

static bool isInitialized = false;
static cxa_array_t callbackEntries;
static callback_entry_t callbackEntries_raw[CXA_CRITICAL_SECTION_MAX_CBS];


// ******** global function implementations ********
void cxa_criticalSection_enter(void)
{
	// immediately increment our nesting levels (to mark that we're in a crit section)
	nestLevels++;
	
	// now if we're nested (or a second caller to this function) don't do anything
	if( nestLevels > 1 ) return;
	
	// if we made it here, we're the first caller...call our pre-entry callbacks
	if( isInitialized )
	{
		for( size_t i = 0; i < cxa_array_getSize_elems(&callbackEntries); i++ )
		{
			callback_entry_t *currEntry = (callback_entry_t*)cxa_array_getAtIndex(&callbackEntries, i);
			if( currEntry == NULL ) continue;
			if( currEntry->preEnter != NULL ) currEntry->preEnter(currEntry->userVar);
		}
	}

	// save our state and disable interrupts
	storedSreg = SREG;
	cxa_xmega_pmic_disableGlobalInterrupts();
}


void cxa_criticalSection_exit(void)
{	
	// immediately mark us as de-nesting (so we don't turn away people thinking we're in a critical section)
	nestLevels--;
	
	// only re-enable interrupts if we're the last ones out
	if( nestLevels == 0 )
	{
		// this statement doesn't necessarily enable interrupts, but simply returns the
		// the interrupt-enabled state to whichever state it was in BEFORE calling
		// cxa_criticalSection_enter()
		SREG = storedSreg;
	
		// now, we need to call our post-exit callbacks...but be sure to keep checking our
		// nest levels in case somebody calls enter while we're still processing
		if( isInitialized && (nestLevels == 0) )
		{
			for( size_t i = 0; i < cxa_array_getSize_elems(&callbackEntries); i++ )
			{
				callback_entry_t *currEntry = (callback_entry_t*)cxa_array_getAtIndex(&callbackEntries, i);
				if( currEntry == NULL ) continue;
				
				// if somebody tries to enter a critical section while we're still here, bail
				if( nestLevels != 0 ) return;
				
				if( currEntry->postExit != NULL ) currEntry->postExit(currEntry->userVar);
			}
		}
	}	
}


void cxa_criticalSection_addCallback(cxa_criticalSection_cb_t cb_preEnterIn, cxa_criticalSection_cb_t cb_postExitIn, void *userVarIn)
{
	if( !isInitialized )
	{
		cxa_array_init(&callbackEntries, sizeof(*callbackEntries_raw), (void*)callbackEntries_raw, sizeof(callbackEntries_raw));
		isInitialized = true;
	}
	
	callback_entry_t newEntry = {.preEnter=cb_preEnterIn, .postExit=cb_postExitIn, .userVar=userVarIn};
	cxa_assert(cxa_array_append(&callbackEntries, &newEntry));
}


void cxa_criticalSection_notifyExternal_enter(void)
{
	// immediately increment our nesting levels (to mark that we're in a crit section)
	nestLevels++;
	
	// now if we're nested (or a second caller to this function) don't do anything
	if( nestLevels > 1 ) return;
	
	// if we made it here, we're the first caller...call our pre-entry callbacks
	if( isInitialized )
	{
		for( size_t i = 0; i < cxa_array_getSize_elems(&callbackEntries); i++ )
		{
			callback_entry_t *currEntry = (callback_entry_t*)cxa_array_getAtIndex(&callbackEntries, i);
			if( currEntry == NULL ) continue;
			if( currEntry->preEnter != NULL ) currEntry->preEnter(currEntry->userVar);
		}
	}

	// since this was an external trigger (maybe an interrupt) we
	// don't actually want to disable any interrupts
}


void cxa_criticalSection_notifyExternal_exit(void)
{
	// immediately mark us as de-nesting (so we don't turn away people thinking we're in a critical section)
	nestLevels--;
		
	// only re-enable interrupts if we're the last ones out
	if( nestLevels == 0 )
	{
		// since this was an external trigger (maybe an interrupt) we
		// dont' actually want to re-enable interrupts (since we didn't disable them in the first place)
			
		// now, we need to call our post-exit callbacks...but be sure to keep checking our
		// nest levels in case somebody calls enter while we're still processing
		if( isInitialized && (nestLevels == 0) )
		{
			for( size_t i = 0; i < cxa_array_getSize_elems(&callbackEntries); i++ )
			{
				callback_entry_t *currEntry = (callback_entry_t*)cxa_array_getAtIndex(&callbackEntries, i);
				if( currEntry == NULL ) continue;
					
				// if somebody tries to enter a critical section while we're still here, bail
				if( nestLevels != 0 ) return;
					
				if( currEntry->postExit != NULL ) currEntry->postExit(currEntry->userVar);
			}
		}
	}
}


// ******** local function implementations ********

