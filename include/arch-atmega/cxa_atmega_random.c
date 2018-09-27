/**
 * @copyright 2018 opencxa.org
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
#include "cxa_random.h"


// ******** includes ********
#include <stdbool.h>
#include <stdlib.h>

#include <cxa_uniqueId.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void init(void);


// ********  local variable declarations *********
static bool isInit = false;


// ******** global function implementations ********
uint32_t cxa_random_numberInRange(uint32_t lowerLimitIn, uint32_t upperLimitIn)
{
	if( isInit ) init();

	// RAND_MAX is 0x7FFF...need to combine at least two of these....
	uint32_t randUint32Range = (((uint32_t)rand()) << 16) | (((uint32_t)rand()) << 0);

	// taken from http://c-faq.com/lib/randrange.html
	return lowerLimitIn + randUint32Range / (UINT32_MAX / (upperLimitIn - lowerLimitIn + 1) + 1);
}


// ******** local function implementations ********
static void init(void)
{
	// generate a random seed for our random reset period
	unsigned randomSeed = 0;
	uint8_t* uniqueBytes;
	size_t numUniqueBytes;
	cxa_uniqueId_getBytes(&uniqueBytes, &numUniqueBytes);
	for( size_t i = 0; i < numUniqueBytes; i++ )
	{
		randomSeed += uniqueBytes[i];
	}
	srand(randomSeed);

	// we're now seeded an initialized
	isInit = true;
}
