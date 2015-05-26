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
#ifndef CXA_LOGGER_HDR_H_
#define CXA_LOGGER_HDR_H_


/**
 * @file <description>
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_LOGGER_MAX_NAME_LEN_CHARS
	#define CXA_LOGGER_MAX_NAME_LEN_CHARS			24
#endif


// ******** global type definitions *********
typedef struct
{
	char name[CXA_LOGGER_MAX_NAME_LEN_CHARS+1];
}cxa_logger_t;


// ******** global function prototypes ********


#endif // CXA_LOGGER_HEADER_H_
