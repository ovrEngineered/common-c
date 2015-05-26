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
#include "cxa_stringUtils.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
const bool cxa_stringUtils_startsWith(const char *targetStringIn, const char *prefixStringIn)
{
	cxa_assert(targetStringIn);
	cxa_assert(prefixStringIn);

	// make sure we have enough chars in our target string
	if( strlen(targetStringIn) < strlen(prefixStringIn) ) return false;

	// iterate our characters
	for( size_t i = 0; i < strlen(prefixStringIn); i++ )
	{
		if( targetStringIn[i] != prefixStringIn[i] ) return false;
	}

	// if we made it here, target string starts with prefix string
	return true;
}


const bool cxa_stringUtils_strcmp_ignoreCase(const char *str1In, const char *str2In)
{
	cxa_assert(str1In);
	cxa_assert(str2In);

	// both strings must be of same length
	if( strlen(str1In) != strlen(str2In) ) return false;

	// now iterate over each character
	for( size_t i = 0; i < strlen(str1In); i++ )
	{
		if( tolower((int)str1In[i]) != tolower((int)str2In[i]) ) return false;
	}

	// if we made it here, the strings were identical
	return true;
}


// ******** local function implementations ********

