/**
 * Copyright 2015 opencxa.org
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
#ifndef CXA_XMEGA_IOSTREAM_TOFILE_H_
#define CXA_XMEGA_IOSTREAM_TOFILE_H_


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


// ******** global function prototypes ********
FILE* cxa_xmega_ioStream_toFile(cxa_ioStream_t *const ioStreamIn);

#endif // CXA_XMEGA_IOSTREAM_TOFILE_H_
