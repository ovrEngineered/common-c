/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_mutex.h"


// ******** includes ********
#include <cxa_assert.h>
#include <avr/interrupt.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	cxa_mutex_t super;
}cxa_atmega_mutex_t;


typedef struct
{
	bool isUsed;
	cxa_atmega_mutex_t mutex;
}mutexEntry_t;


// ******** local function prototypes ********
static void init(void);


// ********  local variable declarations *********
static bool isInit = false;
static mutexEntry_t mutexEntries[1];


// ******** global function implementations ********
cxa_mutex_t* cxa_mutex_reserve(void)
{
	if( !isInit ) init();

	for( size_t i = 0; i < sizeof(mutexEntries)/sizeof(*mutexEntries); i++ )
	{
		if( !mutexEntries[i].isUsed )
		{
			mutexEntries[i].isUsed = true;
			return &mutexEntries[i].mutex.super;
		}
	}

	// no free mutexs
	return NULL;
}


void cxa_mutex_aquire(cxa_mutex_t *const mutexIn)
{
	cxa_assert(mutexIn);

	cli();
}


void cxa_mutex_release(cxa_mutex_t *const mutexIn)
{
	cxa_assert(mutexIn);

	sei();
}


// ******** local function implementations ********
static void init(void)
{
	for( size_t i = 0; i < sizeof(mutexEntries)/sizeof(*mutexEntries); i++ )
	{
		mutexEntries[i].isUsed = false;
	}

	isInit = true;
}
