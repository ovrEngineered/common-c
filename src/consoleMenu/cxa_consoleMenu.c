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
#include "cxa_consoleMenu.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>


// ******** local macro definitions ********
#define MAX_NAME_LEN_CHARS					20


// ******** local type definitions ********


// ******** local function prototypes ********
static void lineParser_cb(uint8_t *lineStartIn, size_t numBytesInLineIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_consoleMenu_init(cxa_consoleMenu_t *const cmIn, FILE *fdIn)
{
	cxa_assert(cmIn);
	cxa_assert(fdIn);
	
	// save our references
	cmIn->fd = fdIn;
	
	// setup our internal state
	cxa_fdLineParser_init(&cmIn->lineParser, fdIn, true, (void*)cmIn->lineParserBuffer, sizeof(cmIn->lineParserBuffer), lineParser_cb, (void*)cmIn);
	cxa_consoleMenu_menu_init(&cmIn->rootMenu);
	cmIn->activeMenu = &cmIn->rootMenu;
	cmIn->hasRenderedMenu = false;
	cmIn->errorMsg = NULL;
}


cxa_consoleMenu_menu_t* cxa_consoleMenu_getRootMenu(cxa_consoleMenu_t *const cmIn)
{
	cxa_assert(cmIn);
	
	return &cmIn->rootMenu;
}


void cxa_consoleMenu_update(cxa_consoleMenu_t *const cmIn)
{
	cxa_assert(cmIn);
	
	// make sure we only render the menu once
	if( !cmIn->hasRenderedMenu )
	{
		fprintf(cmIn->fd, "\fPlease choose from the following options:\r\n");
		if( cmIn->activeMenu != &cmIn->rootMenu ) fprintf(cmIn->fd, "   ^.  return to previous menu\r\n");
		for( size_t i = 0; i < cxa_array_getSize_elems(&cmIn->activeMenu->entries); i++ )
		{
			cxa_consoleMenu_menu_itemEntry_t *currEntry = (cxa_consoleMenu_menu_itemEntry_t*)cxa_array_get(&cmIn->activeMenu->entries, i);
			if( currEntry == NULL ) continue;
		
			fprintf(cmIn->fd, "  %2lu.  %s\r\n", i+1, currEntry->name);
		}
	
		if( cmIn->errorMsg != NULL ) fprintf(cmIn->fd, "Error: %s\r\n", cmIn->errorMsg);
		
		fputs("> ", cmIn->fd);	
		cmIn->hasRenderedMenu = true;
	}
	
	// keep parsing our user's input
	if( !cxa_fdLineParser_update(&cmIn->lineParser) )
	{
		// the user entered too many characters
		cmIn->errorMsg = "too many characters detected";
		cmIn->hasRenderedMenu = false;
	}
}


// ******** local function implementations ********
static void lineParser_cb(uint8_t *lineStartIn, size_t numBytesInLineIn, void *userVarIn)
{
	cxa_consoleMenu_t *cmIn = (cxa_consoleMenu_t*)userVarIn;
	cxa_assert(cmIn);
	
	unsigned int choice;
	if( (cmIn->activeMenu != &cmIn->rootMenu) && (strcmp((char*)lineStartIn, "^") == 0) )
	{
		// return to the previous menu
		cmIn->activeMenu = (cxa_consoleMenu_menu_t*)cmIn->activeMenu->parentMenu;
	}
	else if( sscanf((char*)lineStartIn, "%u", &choice) == 1 )
	{
		// we have a valid number...let's see if it matches
		cxa_consoleMenu_menu_itemEntry_t *currEntry;
		if( (choice == 0) || ((currEntry = (cxa_consoleMenu_menu_itemEntry_t*)cxa_array_get(&cmIn->activeMenu->entries, choice-1)) == NULL) )
		{
			cmIn->errorMsg = "invalid selection";
		}
		else
		{
			// we have a valid selection
			switch( currEntry->type )
			{
				case CXA_CM_MENU_ITEM_TYPE_CALLBACK:
				// printf a crlf in case our callback prints anything
				fputs("\r\n", cmIn->fd);
				if( currEntry->cb != NULL ) currEntry->cb(currEntry->userVar);
				break;
				
				case CXA_CM_MENU_ITEM_TYPE_SUBMENU:
				if( currEntry->subMenu != NULL ) cmIn->activeMenu = currEntry->subMenu;
				break;
			}
			
			// reset our error message marker
			cmIn->errorMsg = NULL;
		}
	}
	else cmIn->errorMsg = "invalid input";
	
	// no matter what, re-render the menu
	cmIn->hasRenderedMenu = false;
}
