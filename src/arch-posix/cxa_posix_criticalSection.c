/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_criticalSection.h"


// ******** includes ********
#include <pthread.h>


// ******** local macro definitions ********


// ******** local type definitions ********



// ******** local function prototypes ********


// ********  local variable declarations *********
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


// ******** global function implementations ********
void cxa_criticalSection_enter(void)
{
	pthread_mutex_lock( &mutex );
}


void cxa_criticalSection_exit(void)
{
	pthread_mutex_unlock( &mutex );
}


// ******** local function implementations ********
