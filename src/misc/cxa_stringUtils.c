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
#include <errno.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	cxa_stringUtils_dataType_t dataType;
	const char* string;
}dataType_string_mapEntry_t;


// ******** local function prototypes ********


// ********  local variable declarations *********
static dataType_string_mapEntry_t DT_STRING_MAP[] =
{
		{CXA_STRINGUTILS_DATATYPE_DOUBLE, "double"},
		{CXA_STRINGUTILS_DATATYPE_INTEGER, "integer"},
		{CXA_STRINGUTILS_DATATYPE_STRING, "string"},
		{CXA_STRINGUTILS_DATATYPE_UNKNOWN, "unknown"}
};


// ******** global function implementations ********
bool cxa_stringUtils_startsWith(const char* targetStringIn, const char* prefixStringIn)
{
	if( (targetStringIn == NULL) || (prefixStringIn == NULL) ) return false;

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


bool cxa_stringUtils_contains(const char* targetStringIn, const char* elementIn)
{
	if( (targetStringIn == NULL) || (elementIn == NULL) ) return false;

	return (strstr(targetStringIn, elementIn) != NULL);
}


bool cxa_stringUtils_strcmp_ignoreCase(const char* str1In, const char* str2In)
{
	if( (str1In == NULL) || (str2In == NULL) ) return false;

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


cxa_stringUtils_parseResult_t cxa_stringUtils_parseString(char *const strIn)
{
	cxa_stringUtils_parseResult_t retVal = {.dataType=CXA_STRINGUTILS_DATATYPE_UNKNOWN};
	if( strIn == NULL ) return retVal;

	// first things first...see if the string contains a period
	if( cxa_stringUtils_contains(strIn, ".") )
	{
		// this may be a double or a string
		char* p = strIn;
		errno = 0;
		double val = strtod(strIn, &p);
		if( (errno != 0) || (strIn == p) || (*p != 0) )
		{
			// couldn't parse a double...must be a string
			retVal.dataType = CXA_STRINGUTILS_DATATYPE_STRING;
			retVal.val_string = strIn;
		}
		else
		{
			// we parsed a double!
			retVal.dataType = CXA_STRINGUTILS_DATATYPE_DOUBLE;
			retVal.val_double = val;
		}
	}
	else
	{
		// this may be a signed integer or a string
		char* p = strIn;
		errno = 0;
		long val = strtol(strIn, &p, 10);
		if( (errno != 0) || (strIn == p) || (*p != 0) )
		{
			// couldn't parse an integer...must be a string
			retVal.dataType = CXA_STRINGUTILS_DATATYPE_STRING;
			retVal.val_string = strIn;
		}
		else
		{
			// we got an integer!
			retVal.dataType = CXA_STRINGUTILS_DATATYPE_INTEGER;
			retVal.val_int = val;
		}
	}

	return retVal;
}


const char* cxa_stringUtils_getStringForDataType(cxa_stringUtils_dataType_t dataTypeIn)
{
	for(int i = 0; i < (sizeof(DT_STRING_MAP)/sizeof(*DT_STRING_MAP)); i++ )
	{
		if( DT_STRING_MAP[i].dataType == dataTypeIn ) return DT_STRING_MAP[i].string;
	}

	return DT_STRING_MAP[(sizeof(DT_STRING_MAP)/sizeof(*DT_STRING_MAP))-1].string;
}


// ******** local function implementations ********

