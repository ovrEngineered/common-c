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
#ifndef CXA_FD_LINEPARSER_H_
#define CXA_FD_LINEPARSER_H_


/**
 * @file <description>
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <stdbool.h>
#include <cxa_array.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef void (*cxa_fdLineParser_lineCb_t)(uint8_t *lineStartIn, size_t numBytesInLineIn, void *userVarIn);

typedef struct 
{
	FILE *fd;
	bool echoUser;
	
	cxa_fdLineParser_lineCb_t cb;
	void *userVar;
	
	bool wasLastByteCr;
	cxa_array_t lineBuffer;
}cxa_fdLineParser_t;


// ******** global function prototypes ********
void cxa_fdLineParser_init(cxa_fdLineParser_t *const fdlpIn, FILE *fdIn, bool echoUserIn, void *bufferIn, size_t bufferSize_bytesIn, cxa_fdLineParser_lineCb_t cbIn, void *userVarIn);

bool cxa_fdLineParser_update(cxa_fdLineParser_t *const fdlpIn);


#endif // CXA_FD_LINEPARSER_H_
