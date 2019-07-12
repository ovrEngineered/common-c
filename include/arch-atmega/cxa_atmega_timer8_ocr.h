/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ATMEGA_TIMER8_OCR_H_
#define CXA_ATMEGA_TIMER8_OCR_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_atmega_timer8_ocr cxa_atmega_timer8_ocr_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER8_OCR_MODE_DISCONNECTED = 0,
	CXA_ATM_TIMER8_OCR_MODE_TOGGLE = 1,
	CXA_ATM_TIMER8_OCR_MODE_NON_INVERTING = 2,
	CXA_ATM_TIMER8_OCR_MODE_INVERTING = 3
}cxa_atmega_timer8_ocr_mode_t;


/**
 * @private
 */
typedef struct cxa_atmega_timer8 cxa_atmega_timer8_t;


/**
 * @private
 */
struct cxa_atmega_timer8_ocr
{
	cxa_atmega_timer8_t* parent;
};



// ******** global function prototypes ********
void cxa_atmega_timer8_ocr_configure(cxa_atmega_timer8_ocr_t *const ocrIn, const cxa_atmega_timer8_ocr_mode_t modeIn);

void cxa_atmega_timer8_ocr_setValue(cxa_atmega_timer8_ocr_t *const ocrIn, const uint8_t valueIn);

#endif
