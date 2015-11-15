/**
 * @file
 * This file contains an implementation of a statically allocated, fixed-max-length FIFO
 * (first-in, first-out) buffer holding elements of a single datatype (and size). The
 * FIFO itself does not hold ant data, rather, it stores the data in an external buffer
 * supplied during initialization.
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_fixedFifo_t myFifo
 * uint16_t myFifo_buffer[16];			// where the data is actually stored
 *
 * // initialize the FIFO with an element type of uint16 (2 bytes), storing a maximum of 16 elements
 * // note the subtle differences:
 * // sizeof(*myFifo_buffer)      2 bytes, the size of each element in the FIFO
 * // sizeof(myFifo_buffer)       32 bytes, 16 elements of type uint16 (2 bytes)
 * cxa_fixedFifo_init(&myFifo, CXA_FF_ON_FULL_DROP, sizeof(*myFifo_buffer), (void*)myFifo_buffer, sizeof(myFifo_buffer));
 * // OR more simply:
 * cxa_fixedFifo_initStd(&myFifo, CXA_FF_ON_FUL_DROP, myFifo_buffer);
 * ...
 *
 * // add a new value to the FIFO
 * uint16_t newVal = 1234;
 * cxa_fixedFifo_queue(&myFifo, (void*)&newVal);
 *
 * ...
 *
 * // see how many elements are in the FIFO (should be 1 at this point)
 * size_t numElems = cxa_fixedFifo_getSize_elems(&myArray);
 *
 * // now dequeue that item from the FIFO
 * uint16_t storedVal;
 * cxa_fixedFifo_dequeue(&myFifo, &(void*)storedVal);
 * @endcode
 *
 *
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
#ifndef CXA_FIXED_FIFO_H_
#define CXA_FIXED_FIFO_H_


/**
 * @file
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdlib.h>
#include <stdbool.h>
#include <cxa_array.h>
#include <cxa_config.h>


// ******** global macro definitions ********
/**
 * @public
 * This is a global configuration directive which determines the maximum
 * number of listeners each FIFO can have. Since we're statically allocated,
 * this reserves space even if you don't use the listener functionality. If
 * you don't require this functionality set to 0 in @see cxa_config.h.
 */
#ifndef CXA_FF_MAX_LISTENERS
	#define CXA_FF_MAX_LISTENERS		1
#endif


/**
 * @public
 * @brief Shortcut to initialize the fifo with a buffer of an explict data type
 *
 * @code
 * cxa_fixedFifo_t myFifo;
 * double myBuffer[100];
 *
 * cxa_fixedFifo_initStd(&myArray, CXA_FF_ON_FULL_DROP, myBuffer);
 * // equivalent to
 * cxa_fixedFifo_init(&myArray, CXA_FF_ON_FULL_DROP, sizeof(*myBuffer), (void)myBuffer, sizeof(myBuffer));
 * @endcode
 *
 * @param[in] fifoIn pointer to FIFO to initialize
 * @param[in] onFullAction the action which should be performed when a queue is attempted
 * 		on a full FIFO
 * @param[in] bufferIn pointer to the declared c-style array which
 * 		will contain the data for the FIFO.
 */
#define cxa_fixedFifo_initStd(fifoIn, onFullActionIn, bufferIn)						cxa_fixedFifo_init((fifoIn), (onFullActionIn), sizeof(*(bufferIn)), ((void*)(bufferIn)), sizeof(bufferIn))


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_fixedFifo_t object
 */
typedef struct cxa_fixedFifo cxa_fixedFifo_t;


/**
 * @public
 * The action which should be performed when this FIFO is full
 * and a queue is attempted.
 */
typedef enum
{
	CXA_FF_ON_FULL_DEQUEUE,			//!< dequeue an element from the array and queue the new item
	CXA_FF_ON_FULL_DROP    			//!< do NOT perform the desired queue
}cxa_fixedFifo_onFullAction_t;


#if CXA_FF_MAX_LISTENERS > 0
/**
 * @public
 * @brief Callback for 'fifoIsNoLongerFull' events.
 * This is a callback prototype for a function that is called when the FIFO was full
 * and a dequeue operation has been performed (hence making the FIFO no-longer full).
 * This callback function must be registered using ::cxa_fixedFifo_addListener.
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 * @param[in] userVarIn user-supplied pointer specified in call to ::cxa_fixedFifo_addListener
 */
typedef void (*cxa_fixedFifo_cb_noLongerFull_t)(cxa_fixedFifo_t *const fifoIn, void* userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_fixedFifo_cb_noLongerFull_t cb_noLongerFull;

	void* userVarIn;
}cxa_fixedFifo_listener_entry_t;
#endif


/**
 * @private
 */
struct cxa_fixedFifo
{
	void *bufferLoc;
	
	size_t insertIndex;
	size_t removeIndex;

	size_t datatypeSize_bytes;
	size_t maxNumElements;
	
	cxa_fixedFifo_onFullAction_t onFullAction;

	#if CXA_FF_MAX_LISTENERS > 0
	cxa_array_t listeners;
	cxa_fixedFifo_listener_entry_t listeners_raw[CXA_FF_MAX_LISTENERS];
	#endif
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the FIFO using the specified buffer (which is empty) to store elements
 *
 * @param[in] fifoIn pointer to the pre-allocated cxa_fixedFifo_t object
 * @param[in] onFullActionIn the action which should be performed when a queue is attempted
 * 		on a full FIFO
 * @param[in] datatypeSize_bytesIn the size of each element that will be inserted
 * 		into the FIFO (all elements MUST be the same size)
 * @param[in] bufferLocIn pointer to the pre-allocated chunk of memory that will
 * 		be used to store elements in the FIFO (the buffer)
 * @param[in] bufferMaxSize_bytesIn the maximum size of the chunk of memory (buffer) in bytes
 */
void cxa_fixedFifo_init(cxa_fixedFifo_t *const fifoIn, cxa_fixedFifo_onFullAction_t onFullActionIn, const size_t datatypeSize_bytesIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn);


#if CXA_FF_MAX_LISTENERS > 0
/**
 * @public
 * @brief Adds a listener to the FIFO which will be called upon various events
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 * @param[in] cb_noLongerFull function that should be called when the FIFO was full,
 * 		but is no longer. Called from the context of the dequeue function.
 * @param[in] userVarIn a user-supplied pointer which will be passed to the callback
 * 		function for each event.
 */
void cxa_fixedFifo_addListener(cxa_fixedFifo_t *const fifoIn, cxa_fixedFifo_cb_noLongerFull_t cb_noLongerFull, void* userVarIn);
#endif


/**
 * @public
 * @brief Queues an element in the FIFO
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 * @param[in] elemIn pointer to the element which will be copied
 * 		into the FIFO's buffer
 *
 * @return true if the queue was successful (either the FIFO was not full OR
 * 		the FIFO was full, but initialized with ::CXA_FF_ON_FULL_DEQUEUE). False
 * 		if the FIFO was full and was initialized with ::CXA_FF_ON_FULL_DROP.
 */
bool cxa_fixedFifo_queue(cxa_fixedFifo_t *const fifoIn, void *const elemIn);


/**
 * @public
 * @brief Dequeues an element from the FIFO
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 * @param[out] elemOut pointer to where the element should be copied. May be
 * 		NULL if no copy is desired.
 *
 * @return true if the FIFO was not empty, false if the FIFO was empty
 */
bool cxa_fixedFifo_dequeue(cxa_fixedFifo_t *const fifoIn, void *elemOut);

/**
 * @public
 * @brief Convenience function for queueing multiple contiguous elements in one call.
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 * @param[in] elemsIn pointer to the contiguous elements which will be copied into
 * 		the FIFO's buffer
 * @param[in] numElemsIn the number of elements to copy into the buffer.
 *
 * @return true if the queues were successful (either the FIFO was not full OR
 * 		the FIFO was full, but initialized with ::CXA_FF_ON_FULL_DEQUEUE). False
 * 		if the FIFO was full and was initialized with ::CXA_FF_ON_FULL_DROP.
 */
bool cxa_fixedFifo_bulkQueue(cxa_fixedFifo_t *const fifoIn, void *const elemsIn, size_t numElemsIn);


/**
 * @public
 * @brief Convenience function for dequeueing multiple elements in one call. This
 * 		function doesn't copy any elements out of the queue so should be used
 * 		in concert with ::cxa_fixedFifo_bulkDequeue_peek.
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 * @param[in] numElemsIn number of elements to dequeue from the FIFO.
 *
 * @return true if the desired number of elements were dequeued, false if not
 */
bool cxa_fixedFifo_bulkDequeue(cxa_fixedFifo_t *const fifoIn, size_t numElemsIn);


/**
 * @public
 * @brief 'Peeks' at the queue and determines the maximum number of contiguous elements
 * 		that can be dequeued in a single operation.
 *
 * Since the implementation of the queue is essentially a ring buffer, this function
 * returns a pointer to the first element to dequeue and will tell you how many
 * contiguous elements are available. The number of contiguous elements _may_ be
 * less than the number of elements stored in the FIFO (if the insert pointer of
 * the buffer has wrapped around).
 *
 * Care should be taken that no other modifying operations (queue, dequeue, etc)
 * are performed on this FIFO until the bulk dequeue is completed using
 * ::cxa_fixedFifo_bulkDequeue.
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 * @param[out] elemsOut a pointer that will be set with the address of the first
 * 		element slated for dequeue (within the FIFO buffer itself). Remaining
 * 		elements can be assumed to be stored contiguously following.
 *
 * @return the number of contiguous elements available for dequeue
 */
size_t cxa_fixedFifo_bulkDequeue_peek(cxa_fixedFifo_t *const fifoIn, void **const elemsOut);


/**
 * @public
 * @brief Determines the size of the FIFO (in number of elements).
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 *
 * @return the size of the FIFO, in number of elements
 */
size_t cxa_fixedFifo_getSize_elems(cxa_fixedFifo_t *const fifoIn);


/**
 * @public
 * @brief Determines the number of free spots/elements in the FIFO.
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 *
 * @return the number of free elements in the FIFO
 */
size_t cxa_fixedFifo_getFreeSize_elems(cxa_fixedFifo_t *const fifoIn);


/**
 * @public
 * @brief Determines whether the FIFO is full (cannot hold any more elements).
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 *
 * @return true if the FIFO cannot hold any more elements
 */
bool cxa_fixedFifo_isFull(cxa_fixedFifo_t *const fifoIn);


/**
 * @public
 * @brief Determines whether the FIFO is empty (does not hold any elements)
 *
 * @param[in] fifoIn pointer to the pre-initialized FIFO object
 *
 * @return true if the FIFO does not current contain any elements
 */
bool cxa_fixedFifo_isEmpty(cxa_fixedFifo_t *const fifoIn);


#endif // CXA_FIXED_FIFO_H_
