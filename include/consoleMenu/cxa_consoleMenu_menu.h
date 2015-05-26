/**
 * @file
 * This file contains a prototypes for actually inserting menu items into
 * cxa_consoleMenu menu.
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_consoleMenu_t myMenu;
 * // initialize the main menu object (per cxa_consoleMenu.h)
 * // and get reference to the root menu object
 * cxa_consoleMenu_menu_t *rootMenu = cxa_consoleMenu_getRootMenu(&smIn->consoleMenu);
 *
 * ...
 * // add a callback (callback is called when a user selects the menu option)
 * cxa_consoleMenu_menu_addItem_callback(rootMenu, "do something", menu_cb_doSomething, NULL);
 *
 * // create a submenu and add it (with a nested callback)
 * cxa_consoleMenu_menu_t subMenu;
 * cxa_consoleMenu_menu_init(&subMenu);
 * cxa_consoleMenu_menu_addItem_callback(&subMenu, "do another thing", menu_cb_doAnotherThing, NULL);
 * cxa_consoleMenu_menu_addItem_subMenu(rootMenu, "subMenu text", &subMenu);
 *
 * ...
 *
 * static void menu_cb_doSomething(void *userVarIn){ ... }
 * static void menu_cb_doAnotherThing(void *userVarIn){ ... }
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
#ifndef CXA_CONSOLE_MENU_MENU_H_
#define CXA_CONSOLE_MENU_MENU_H_


// ******** includes ********
#include <cxa_config.h>
#include <cxa_array.h>


// ******** global macro definitions ********
#ifndef CXA_CONSOLE_MENU_MAX_MENU_ITEMS
	#define CXA_CONSOLE_MENU_MAX_MENU_ITEMS					4				///< The maximum number of items each menu can have
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_consoleMenu_menu_t object
 */
typedef struct cxa_consoleMenu_menu cxa_consoleMenu_menu_t;


/**
 * @public
 * @brief Callback for when a callback menu item is selected by the user.
 *
 * @param[in] userVarIn the user variable passed to ::cxa_consoleMenu_menu_addItem_callback
 */
typedef void (*cxa_consoleMenu_menu_itemCb_t)(void *userVarIn);


/**
 * @private
 */
typedef enum
{
	CXA_CM_MENU_ITEM_TYPE_CALLBACK,
	CXA_CM_MENU_ITEM_TYPE_SUBMENU
}cxa_consoleMenu_menu_itemType_t;


/**
 * @private
 */
typedef struct
{	
	const char *name;
	cxa_consoleMenu_menu_itemType_t type;
	union
	{
		cxa_consoleMenu_menu_itemCb_t cb;	
		cxa_consoleMenu_menu_t *subMenu;
	};
		
	void *userVar;
}cxa_consoleMenu_menu_itemEntry_t;


/**
 * @private
 */
struct cxa_consoleMenu_menu
{
	void *parentMenu;
	
	cxa_array_t entries;
	cxa_consoleMenu_menu_itemEntry_t entries_raw[CXA_CONSOLE_MENU_MAX_MENU_ITEMS];
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the provided consoleMenu_menu object.
 * @note The cxa_consoleMenu_menu_t object returned by cxa_consoleMenu_getRootMenu does
 *		<b>NOT</b> need to be initialized. This function should only be used to initialize
 *		user-created menus.
 *
 * @param[in] cmmIn pointer to the pre-allocated consoleMenu_menu object
 */
void cxa_consoleMenu_menu_init(cxa_consoleMenu_menu_t *const cmmIn);


/**
 * @public
 * @brief Adds a menu item to the provided menu. When selected the 
 * consoleMenu subsystem will call the provided callback.
 *
 * @param[in] cmmIn pointer to the pre-initialized consoleMenu_menu object
 * @param[in] nameIn pointer to the string which should be displayed to the user
 *		in the menu
 * @param[in] cbIn the callback which should be called when the user selects
 *		this menu option
 * @param[in] userVarIn the user variable which should be passed to the provided callback
 */
void cxa_consoleMenu_menu_addItem_callback(cxa_consoleMenu_menu_t *const cmmIn, const char *nameIn, cxa_consoleMenu_menu_itemCb_t cbIn, void *userVarIn);


/**
 * @public
 * @brief Adds a subMenu item to the provided menu. When selected the 
 * consoleMenu subsystem will display the provided menu in place of
 * the previously displayed menu.
 *
 * @param[in] cmmIn pointer to the pre-initialized consoleMenu_menu object
 * @param[in] nameIn pointer to the string which should be displayed to the user
 *		in the menu
 * @param[in] childCmmIn pointer to the pre-initialized consoleMenu_menu object
 *		which contains the submenu items
 */
void cxa_consoleMenu_menu_addItem_subMenu(cxa_consoleMenu_menu_t *const parentCmmIn, const char *nameIn, cxa_consoleMenu_menu_t *const childCmmIn);


#endif // CXA_CONSOLE_MENU_SUBMENU_H_
