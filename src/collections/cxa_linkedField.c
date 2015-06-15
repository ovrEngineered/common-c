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
#include "cxa_linkedField.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>
#include <cxa_numberUtils.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_fixedByteBuffer_t* getParentBufferFromChild(cxa_linkedField_t *const fbbLfIn);
static size_t getLengthOfAllFixedFields_bytes(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn);
static size_t getStartIndexInParent(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn);
static cxa_linkedField_t* getEndOfChain(cxa_linkedField_t *const fbbLfIn);
static bool validateChain(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn);
static bool isUnfilledFixedLengthFieldUpChain(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn);
static bool isNonEmptyFieldDownChain(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn);


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_linkedField_initRoot(cxa_linkedField_t *const fbbLfIn, cxa_fixedByteBuffer_t *const parentFbbIn, const size_t startIndexInParentIn, const size_t initialSize_bytesIn)
{
	cxa_assert(fbbLfIn);
	cxa_assert(parentFbbIn);

	// save our internal state
	fbbLfIn->parent = parentFbbIn;
	fbbLfIn->prev = NULL;
	fbbLfIn->next = NULL;
	fbbLfIn->startIndex = startIndexInParentIn;
	fbbLfIn->isFixedLength = false;
	fbbLfIn->maxFixedLength_bytes = 0;
	fbbLfIn->currSize_bytes = initialSize_bytesIn;

	// make sure the start index isn't outside the max bounds for the parent
	if( startIndexInParentIn+initialSize_bytesIn > cxa_fixedByteBuffer_getMaxSize_bytes(parentFbbIn) ) return false;

	return true;
}


bool cxa_linkedField_initRoot_fixedLen(cxa_linkedField_t *const fbbLfIn, cxa_fixedByteBuffer_t *const parentFbbIn, const size_t startIndexInParentIn, const size_t maxLen_bytesIn)
{
	cxa_assert(fbbLfIn);
	cxa_assert(parentFbbIn);

	// save our internal state
	fbbLfIn->parent = parentFbbIn;
	fbbLfIn->prev = NULL;
	fbbLfIn->next = NULL;
	fbbLfIn->startIndex = startIndexInParentIn;
	fbbLfIn->isFixedLength = true;
	fbbLfIn->maxFixedLength_bytes = maxLen_bytesIn;
	fbbLfIn->currSize_bytes = CXA_MIN(maxLen_bytesIn, cxa_fixedByteBuffer_getSize_bytes(parentFbbIn));

	// make sure that our fixed size (with index) isn't bigger than the parent's capacity
	if( (startIndexInParentIn + maxLen_bytesIn) > cxa_fixedByteBuffer_getMaxSize_bytes(parentFbbIn) ) return false;

	return true;
}

bool cxa_linkedField_initChild(cxa_linkedField_t *const fbbLfIn, cxa_linkedField_t *const prevFbbLfIn, const size_t initialSize_bytesIn)
{
	cxa_assert(fbbLfIn);

	// save our internal state
	fbbLfIn->prev = prevFbbLfIn;
	fbbLfIn->next = NULL;
	prevFbbLfIn->next = fbbLfIn;
	fbbLfIn->parent = NULL;
	fbbLfIn->parent = getParentBufferFromChild(fbbLfIn);
	if( fbbLfIn->parent == NULL )
	{
		// we failed to initialize properly
		prevFbbLfIn->next = NULL;
		return false;
	}

	fbbLfIn->isFixedLength = false;
	fbbLfIn->maxFixedLength_bytes = 0;
	fbbLfIn->currSize_bytes = initialSize_bytesIn;

	if( ((getStartIndexInParent(fbbLfIn, true) + fbbLfIn->currSize_bytes) > cxa_fixedByteBuffer_getSize_bytes(fbbLfIn->parent)) || !validateChain(fbbLfIn, true) )
	{
		// we failed to initialize properly
		prevFbbLfIn->next = NULL;
		return false;
	}

	return true;
}


bool cxa_linkedField_initChild_fixedLen(cxa_linkedField_t *const fbbLfIn, cxa_linkedField_t *const prevFbbLfIn, const size_t maxLen_bytesIn)
{
	cxa_assert(fbbLfIn);

	// save our internal state
	fbbLfIn->prev = prevFbbLfIn;
	fbbLfIn->next = NULL;
	prevFbbLfIn->next = fbbLfIn;
	fbbLfIn->parent = NULL;
	fbbLfIn->parent = getParentBufferFromChild(fbbLfIn);
	if( fbbLfIn->parent == NULL ) return false;

	fbbLfIn->isFixedLength = true;
	fbbLfIn->maxFixedLength_bytes = maxLen_bytesIn;
	fbbLfIn->currSize_bytes = CXA_MIN(maxLen_bytesIn, (cxa_fixedByteBuffer_getSize_bytes(fbbLfIn->parent) - getStartIndexInParent(fbbLfIn, true)));

	// make sure that that sizes of all fixed-length fields aren't too big...
	if( getLengthOfAllFixedFields_bytes(fbbLfIn, true) > cxa_fixedByteBuffer_getMaxSize_bytes(fbbLfIn->parent) ) return false;

	return validateChain(fbbLfIn, true);
}


bool cxa_linkedField_append(cxa_linkedField_t *const fbbLfIn, uint8_t *const ptrIn, const size_t numBytesIn)
{
	cxa_assert(fbbLfIn);
	cxa_assert(ptrIn);

	// chain validation is done multiple times elsewhere
	return cxa_linkedField_insert(fbbLfIn, fbbLfIn->currSize_bytes, ptrIn, numBytesIn);
}


bool cxa_linkedField_remove(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, const size_t numBytesIn)
{
	cxa_assert(fbbLfIn);

	// ensure our chain is valid
	if( !validateChain(fbbLfIn, true) ) return false;

	// we can't be removed if there is stuff after us and we are fixed length...
	if( fbbLfIn->isFixedLength && isNonEmptyFieldDownChain(fbbLfIn, true) ) return false;

	// make sure the index is in bounds
	size_t removeIndex = getStartIndexInParent(fbbLfIn, true) + indexIn;
	if( ((indexIn + numBytesIn) > fbbLfIn->currSize_bytes) || ((removeIndex+ numBytesIn) > cxa_fixedByteBuffer_getSize_bytes(fbbLfIn->parent)) ) return false;

	// if we made it here, we can try the remove
	if( !cxa_fixedByteBuffer_remove(fbbLfIn->parent, removeIndex , numBytesIn) ) return false;
	fbbLfIn->currSize_bytes -= numBytesIn;

	return true;
}


bool cxa_linkedField_remove_cString(cxa_linkedField_t *const fbbLfIn, const size_t indexIn)
{
	cxa_assert(fbbLfIn);

	// ensure our chain is valid
	if( !validateChain(fbbLfIn, true) ) return false;

	// get our target string
	uint8_t* targetString = cxa_linkedField_get_pointerToIndex(fbbLfIn, indexIn);
	if( targetString == NULL ) return false;

	// figure out how long the string is...
	size_t strLen_bytes = strlen((const char*)targetString) + 1;

	return cxa_linkedField_remove(fbbLfIn, indexIn, strLen_bytes);
}


uint8_t* cxa_linkedField_get_pointerToIndex(cxa_linkedField_t *const fbbLfIn, const size_t indexIn)
{
	cxa_assert(fbbLfIn);

	// ensure our chain is valid
	if( !validateChain(fbbLfIn, true) ) return NULL;

	// we need an index
	size_t parentIndex = getStartIndexInParent(fbbLfIn, true) + indexIn;

	return cxa_fixedByteBuffer_get_pointerToIndex(fbbLfIn->parent, parentIndex);
}


bool cxa_linkedField_get(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, bool transposeIn, uint8_t *const valOut, const size_t numBytesIn)
{
	cxa_assert(fbbLfIn);

	// ensure our chain is valid
	if( !validateChain(fbbLfIn, true) ) return false;

	// make sure that we have enough bytes in _our_ buffer
	if( numBytesIn > fbbLfIn->currSize_bytes ) return false;

	// we need an index
	size_t parentIndex = getStartIndexInParent(fbbLfIn, true) + indexIn;

	return cxa_fixedByteBuffer_get(fbbLfIn->parent, parentIndex, transposeIn, valOut, numBytesIn);
}


bool cxa_linkedField_get_cstring(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, char *const stringOut, size_t maxOutputSize_bytes)
{
	cxa_assert(fbbLfIn);

	// ensure our chain is valid
	if( !validateChain(fbbLfIn, true) ) return false;

	// get our target string
	char* targetString = (char*)cxa_linkedField_get_pointerToIndex(fbbLfIn, indexIn);
	if( targetString == NULL ) return false;

	// strlen+1 for term
	size_t targetStringLen_bytes = strlen(targetString)+1;

	// make sure that we have enough bytes in _our_ buffer
	if( targetStringLen_bytes > fbbLfIn->currSize_bytes ) return false;

	size_t parentIndex = getStartIndexInParent(fbbLfIn, true) + indexIn;
	return cxa_fixedByteBuffer_get_cString(fbbLfIn->parent, parentIndex, stringOut, maxOutputSize_bytes);
}


bool cxa_linkedField_replace(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, uint8_t *const ptrIn, const size_t numBytesIn)
{
	cxa_assert(fbbLfIn);

	// ensure our chain is valid
	if( !validateChain(fbbLfIn, true) ) return false;

	// make sure that we have enough bytes in _our_ buffer
	if( numBytesIn > fbbLfIn->currSize_bytes ) return false;

	// we need an index
	size_t parentIndex = getStartIndexInParent(fbbLfIn, true) + indexIn;
	return cxa_fixedByteBuffer_replace(fbbLfIn->parent, parentIndex, ptrIn, numBytesIn);
}


bool cxa_linkedField_replace_cstring(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, char *const stringIn)
{
	cxa_assert(fbbLfIn);

	// chain validation is done multiple times elsewhere

	// calculate our sizes
	size_t replacementStringSize_bytes = strlen(stringIn)+1;
	char* targetString = (char*)cxa_linkedField_get_pointerToIndex(fbbLfIn, indexIn);
	if( targetString == NULL ) return false;
	size_t targetStringSize_bytes = strlen(targetString)+1;
	ssize_t discrepantSize_bytes = replacementStringSize_bytes - targetStringSize_bytes;

	// make sure we have room for this operation at the specified index
	if( (indexIn+replacementStringSize_bytes) > cxa_linkedField_getMaxSize_bytes(fbbLfIn) ) return false;

	// now, make sure that we have enough free space in the buffer if the replacement string is larger
	if( (discrepantSize_bytes > 0) && (cxa_linkedField_getFreeSize_bytes(fbbLfIn) < discrepantSize_bytes) ) return false;

	// if we made it here, we should be good to perform the operation...start by removing the current string
	if( !cxa_linkedField_remove_cString(fbbLfIn, indexIn) ) return false;

	// now insert the new string
	return cxa_linkedField_insert_cString(fbbLfIn, indexIn, stringIn);
}


bool cxa_linkedField_insert(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, uint8_t *const ptrIn, const size_t numBytesIn)
{
	cxa_assert(fbbLfIn);
	cxa_assert(ptrIn);

	// ensure our chain is valid
	if( !validateChain(fbbLfIn, true) ) return false;

	// make sure there aren't any unfilled fixed-length fields before us
	if( isUnfilledFixedLengthFieldUpChain(fbbLfIn, true) ) return false;

	// if we made it here, we can at least try to insert the item...
	size_t parentIndex = getStartIndexInParent(fbbLfIn, true) + indexIn;
	if( !cxa_fixedByteBuffer_insert(fbbLfIn->parent, parentIndex, ptrIn, numBytesIn) ) return false;

	fbbLfIn->currSize_bytes += numBytesIn;

	return true;
}


size_t cxa_linkedField_getSize_bytes(cxa_linkedField_t *const fbbLfIn)
{
	cxa_assert(fbbLfIn);
	if( !validateChain(fbbLfIn, true) ) return 0;

	return fbbLfIn->currSize_bytes;
}


size_t cxa_linkedField_getMaxSize_bytes(cxa_linkedField_t *const fbbLfIn)
{
	cxa_assert(fbbLfIn);
	if( !validateChain(fbbLfIn, true) ) return 0;

	if( fbbLfIn->isFixedLength ) return fbbLfIn->maxFixedLength_bytes;

	// if we made it here, this is a little more complicated...
	return cxa_fixedByteBuffer_getMaxSize_bytes(fbbLfIn->parent) - getLengthOfAllFixedFields_bytes(fbbLfIn, true);
}


size_t cxa_linkedField_getFreeSize_bytes(cxa_linkedField_t *const fbbLfIn)
{
	cxa_assert(fbbLfIn);

	return cxa_linkedField_getMaxSize_bytes(fbbLfIn) - cxa_linkedField_getSize_bytes(fbbLfIn);
}


size_t cxa_linkedField_getStartIndexInParent(cxa_linkedField_t *const fbbLfIn)
{
	cxa_assert(fbbLfIn);

	return getStartIndexInParent(fbbLfIn, true);
}


// ******** local function implementations ********
static cxa_fixedByteBuffer_t* getParentBufferFromChild(cxa_linkedField_t *const fbbLfIn)
{
	cxa_assert(fbbLfIn);

	if( fbbLfIn->parent != NULL ) return fbbLfIn->parent;

	// if we made it here, we need to recurse a little bit
	if( fbbLfIn->prev != NULL ) return getParentBufferFromChild(fbbLfIn->prev);

	// if we made it here, we're out of luck
	return NULL;
}


static size_t getLengthOfAllFixedFields_bytes(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn)
{
	cxa_assert(fbbLfIn);

	if( isFirstCallIn )
	{
		cxa_linkedField_t *endOfChain = getEndOfChain(fbbLfIn);
		if( endOfChain == NULL ) return 0;

		return getLengthOfAllFixedFields_bytes(endOfChain, false);
	}

	// if we made it here, this isn't our first call...
	size_t retVal = (fbbLfIn->isFixedLength) ? fbbLfIn->maxFixedLength_bytes : 0;
	if( fbbLfIn->prev != NULL ) retVal += getLengthOfAllFixedFields_bytes(fbbLfIn->prev, false);

	return retVal;
}


static size_t getStartIndexInParent(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn)
{
	cxa_assert(fbbLfIn);

	size_t retVal = 0;

	// add from the previous field
	if( fbbLfIn->prev != NULL ) retVal += getStartIndexInParent(fbbLfIn->prev, false);

	// now add ourselves
	if( fbbLfIn->prev == NULL ) retVal += fbbLfIn->startIndex;
	if( !isFirstCallIn) retVal += fbbLfIn->currSize_bytes;

	return retVal;
}


static cxa_linkedField_t* getEndOfChain(cxa_linkedField_t *const fbbLfIn)
{
	cxa_assert(fbbLfIn);

	return (fbbLfIn->next != NULL) ? getEndOfChain(fbbLfIn->next) : fbbLfIn;
}


static bool validateChain(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn)
{
	cxa_assert(fbbLfIn);

	// if we were the first...
	if( isFirstCallIn )
	{
		// start the recursion from the end of the chain
		cxa_linkedField_t* endOfChain = getEndOfChain(fbbLfIn);
		if( endOfChain == NULL ) return false;

		// now that we found the end of the chain, calculate sizes for it
		size_t calcSize_bytes = getStartIndexInParent(endOfChain, true) + endOfChain->currSize_bytes;
		size_t actualSize_bytes = cxa_fixedByteBuffer_getSize_bytes(endOfChain->parent);

		if( calcSize_bytes > actualSize_bytes ) return false;

		return validateChain(endOfChain, false);
	}

	// if we made it here, we're not the first...do basic checks
	// make sure we have a parent
	if( fbbLfIn->parent == NULL ) return false;

	// now validate the previous field (if applicable)
	return (fbbLfIn->prev != NULL) ? validateChain(fbbLfIn->prev, false) : true;
}


static bool isUnfilledFixedLengthFieldUpChain(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn)
{
	cxa_assert(fbbLfIn);

	// check ourselves (if we're not first)
	if( !isFirstCallIn )
	{
		if( fbbLfIn->isFixedLength && (fbbLfIn->currSize_bytes != fbbLfIn->maxFixedLength_bytes) ) return true;
	}

	// check the previous field
	if( fbbLfIn->prev != NULL)
	{
		if( isUnfilledFixedLengthFieldUpChain(fbbLfIn->prev, false) ) return true;
	}

	return false;
}


static bool isNonEmptyFieldDownChain(cxa_linkedField_t *const fbbLfIn, bool isFirstCallIn)
{
	cxa_assert(fbbLfIn);

	// check ourselves (if we're not first)
	if( !isFirstCallIn )
	{
		if( fbbLfIn->currSize_bytes != 0 ) return true;
	}

	// check the next field
	if( fbbLfIn->next != NULL )
	{
		if( isNonEmptyFieldDownChain(fbbLfIn->next, false) ) return true;
	}

	return false;
}
