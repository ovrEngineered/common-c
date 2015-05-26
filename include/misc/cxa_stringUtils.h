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
#ifndef CXA_STRINGUTILS_H_
#define CXA_STRINGUTILS_H_


/**
 * @file
 * This is a file which contains utility functions for manipulating C-strings
 *
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
const bool cxa_stringUtils_startsWith(const char *targetStringIn, const char *prefixStringIn);

const bool cxa_stringUtils_strcmp_ignoreCase(const char *str1In, const char *str2In);

#endif // CXA_STRINGUTILS_H_
