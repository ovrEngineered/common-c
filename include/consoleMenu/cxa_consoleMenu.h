/**
 * @file
 * This file contains a platform-independent implementation of a console-based menu. The console
 * menu allows the programmer to create a series of menu-like screens which is presented
 * to the user using a file descriptor (serial port, etc). The user can then pick a menu
 * item by entering a number and pressing return. Each menu item either leads to a nested
 * menu OR a initiates a programmer-specified callback.
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_consoleMenu_t myMenu;
 * // initialize the main menu object, using stdIn to print text
 * // AND receive text from the user (in most cases, stdin == stdout).
 * cxa_consoleMenu_init(&myMenu, stdIn);
 *
 * // now get a reference to the root menu (the first menu the user sees)
 * cxa_consoleMenu_menu_t *rootMenu = cxa_consoleMenu_getRootMenu(&smIn->consoleMenu);
 *
 * // see cxa_consoleMenu_menu.h for adding menu items
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
#ifndef CXA_CONSOLE_MENU_H_
#define CXA_CONSOLE_MENU_H_


// ******** includes ********
#include <stdio.h>
#include <stdbool.h>
#include <cxa_consoleMenu_menu.h>
#include <cxa_fdLineParser.h>


// ******** global macro definitions ********
#define CXA_CONSOLE_MENU_MAX_USER_INPUT_LEN_CHARS		3					///< max number of chars the user can type


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_consoleMenu_t object
 */
typedef struct cxa_consoleMenu cxa_consoleMenu_t;


/**
 * @private
 */
struct cxa_consoleMenu
{
	FILE *fd;
	
	bool hasRenderedMenu;
	char *errorMsg;
	
	cxa_consoleMenu_menu_t rootMenu;
	cxa_consoleMenu_menu_t *activeMenu;
	
	uint8_t lineParserBuffer[CXA_CONSOLE_MENU_MAX_USER_INPUT_LEN_CHARS];
	cxa_fdLineParser_t lineParser;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the provided consoleMenu object.
 * Uses the provided file descriptor for all input (keys from the user)
 * and output (text to the screen).
 *
 * @param[in] cmIn pointer to the pre-allocated consoleMenu object
 * @param[in] fdIn pointer to the pre-initialized file descriptor
 *		which can be read from (fgetc, etc) to get user input and output to
 *		(fputc, etc) to display text to the screen.
 */
void cxa_consoleMenu_init(cxa_consoleMenu_t *const cmIn, FILE *fdIn);


/**
 * @public
 * @brief Used to return the actual "menu" object which can be then
 * be used to menu items.
 * The menu returned is the first menu the user sees.
 *
 * @param[in] cmIn pointer to the pre-initialized consoleMenu object
 *
 * @return pointer to the root menu object for the provided consoleMenu object
 */
cxa_consoleMenu_menu_t* cxa_consoleMenu_getRootMenu(cxa_consoleMenu_t *const cmIn);


/**
 * @public
 * @brief <b>MUST be called on a regular basis</b> to parse user input.
 * All callbacks/printing are executed within this function, therefore, 
 * you should not count on this function returning in a timely manner.
 * However, this function is explicitly non-blocking (ie. will not block
 * waiting for user input).
 *
 * @param[in] cmIn pointer to the pre-initialized consoleMenu object
 */
void cxa_consoleMenu_update(cxa_consoleMenu_t *const cmIn);


#endif // CXA_CONSOLE_MENU_H_
