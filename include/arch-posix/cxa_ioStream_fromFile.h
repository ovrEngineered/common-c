/**
 * Copyright 2013-2015 opencxa.org
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
#ifndef CXA_IOSTREAM_FROMFILE_H_
#define CXA_IOSTREAM_FROMFILE_H_


/**
 * @file
 *
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <stdio.h>
#include <cxa_ioStream.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_ioStream_fromFile_t object
 */
typedef struct cxa_ioStream_fromFile cxa_ioStream_fromFile_t;


struct cxa_ioStream_fromFile
{
	cxa_ioStream_t super;

	FILE* file;
};


// ******** global function prototypes ********
void cxa_ioStream_fromFile_init(cxa_ioStream_fromFile_t *const ioStreamIn, FILE *const fileIn);
void cxa_ioStream_fromFile_close(cxa_ioStream_fromFile_t *const ioStreamIn);

#endif // CXA_IOSTREAM_FROMFILE_H_
