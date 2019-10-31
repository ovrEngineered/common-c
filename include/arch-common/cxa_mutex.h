/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MUTEX_H_
#define CXA_MUTEX_H_


// ******** includes ********


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_mutex cxa_mutex_t;


/**
 * @private
 */
struct cxa_mutex
{
	void* placeholder;
};


// ******** global function prototypes ********
cxa_mutex_t* cxa_mutex_reserve(void);

void cxa_mutex_aquire(cxa_mutex_t *const mutexIn);

void cxa_mutex_release(cxa_mutex_t *const mutexIn);


#endif
