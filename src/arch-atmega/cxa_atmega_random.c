/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
