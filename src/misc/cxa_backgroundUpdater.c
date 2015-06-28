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
#include "cxa_backgroundUpdater.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********
/**
 * @private
 */
typedef struct
{
	cxa_backgroundUpdater_cb_update_t cb;
	void *userVar;
}cxa_backgroundUpdater_entry_t;


/**
 * @private
 */
typedef struct
{
	cxa_array_t cbs;
	cxa_backgroundUpdater_entry_t cbs_raw[CXA_BACKGROUND_UPDATER_MAX_NUM_ENTRIES];
}cxa_backgroundUpdater_t;


// ******** local function prototypes ********


// ********  local variable declarations *********
static cxa_backgroundUpdater_t SINGLETON;
static bool isInit = false;


// ******** global function implementations ********
void cxa_backgroundUpdater_init(void)
{
	if( isInit ) return;

	cxa_array_initStd(&SINGLETON.cbs, SINGLETON.cbs_raw);
	isInit = true;
}


bool cxa_backgroundUpdater_addEntry(cxa_backgroundUpdater_cb_update_t cbIn, void *const userVarIn)
{
	if( !isInit ) cxa_backgroundUpdater_init();

	// create our new entry
	cxa_backgroundUpdater_entry_t newEntry = {.cb=cbIn, .userVar=userVarIn};
	return cxa_array_append(&SINGLETON.cbs, &newEntry);
}


bool cxa_backgroundUpdater_removeEntry(cxa_backgroundUpdater_cb_update_t cbIn)
{
	if( !isInit ) cxa_backgroundUpdater_init();

	// can't use cxa_array_iterate because we need an index
	for( size_t i = 0; i < cxa_array_getSize_elems(&SINGLETON.cbs); i++ )
	{
		cxa_backgroundUpdater_entry_t* currEntry = (cxa_backgroundUpdater_entry_t*)cxa_array_get(&SINGLETON.cbs, i);
		if( currEntry == NULL ) continue;

		if( currEntry->cb == cbIn )
		{
			cxa_array_remove_atIndex(&SINGLETON.cbs, i);
			return true;
		}
	}

	return false;
}


void cxa_backgroundUpdater_update(void)
{
	if( !isInit ) cxa_backgroundUpdater_init();

	cxa_array_iterate(&SINGLETON.cbs, currEntry, cxa_backgroundUpdater_entry_t)
	{
		if( currEntry->cb != NULL) currEntry->cb(currEntry->userVar);
	}
}


// ******** local function implementations ********

