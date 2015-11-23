/**
 * @file
 *
 * @copyright 2015 opencxa.org
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
#ifndef CXA_UNIQUE_ID_H_
#define CXA_UNIQUE_ID_H_


// ******** includes ********
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
void cxa_uniqueId_getBytes(uint8_t** bytesOut, size_t* numBytesOut);
char* cxa_uniqueId_getHexString(void);


#endif // CXA_UNIQUE_ID_H_
