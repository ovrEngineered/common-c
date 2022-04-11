/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ATMEGA_TIMER_OCR_H_
#define CXA_ATMEGA_TIMER_OCR_H_


// ******** includes ********
#include <cxa_array.h>
#include <stdint.h>


// ******** global macro definitions ********
#ifndef CXA_ATMEGA_TIMER_OCR_MAXNUM_LISTENERS
#define CXA_ATMEGA_TIMER_OCR_MAXNUM_LISTENERS			1
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_atmega_timer_ocr cxa_atmega_timer_ocr_t;


/**
 * @public
 */
typedef void (*cxa_atmega_timer_ocr_cb_onCompareMatch_t)(cxa_atmega_timer_ocr_t *const ocrIn, void *userVarIn);


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER_OCR_MODE_DISCONNECTED = 0,
	CXA_ATM_TIMER_OCR_MODE_TOGGLE = 1,
	CXA_ATM_TIMER_OCR_MODE_NON_INVERTING = 2,
	CXA_ATM_TIMER_OCR_MODE_INVERTING = 3
}cxa_atmega_timer_ocr_mode_t;


/**
 * @private
 */
typedef struct cxa_atmega_timer cxa_atmega_timer_t;


/**
 * @private
 */
typedef struct
{
	cxa_atmega_timer_ocr_cb_onCompareMatch_t cb_onCompareMatch;
	void* userVar;
}cxa_atmega_timer_ocr_listenerEntry_t;


/**
 * @private
 */
struct cxa_atmega_timer_ocr
{
	cxa_atmega_timer_t* parent;

	cxa_atmega_timer_ocr_mode_t mode;

	cxa_array_t listeners;
	cxa_atmega_timer_ocr_listenerEntry_t listeners_raw[CXA_ATMEGA_TIMER_OCR_MAXNUM_LISTENERS];
};



// ******** global function prototypes ********
void cxa_atmega_timer_ocr_configure(cxa_atmega_timer_ocr_t *const ocrIn, const cxa_atmega_timer_ocr_mode_t modeIn);

uint16_t cxa_atmega_timer_ocr_getValue(cxa_atmega_timer_ocr_t *const ocrIn);
void cxa_atmega_timer_ocr_setValue(cxa_atmega_timer_ocr_t *const ocrIn, const uint16_t valueIn);

void cxa_atmega_timer_ocr_addListener(cxa_atmega_timer_ocr_t *const ocrIn, cxa_atmega_timer_ocr_cb_onCompareMatch_t cb_onCompareMatchIn, void* userVarIn);

void cxa_atmega_timer_ocr_enableInterrupt_compareMatch(cxa_atmega_timer_ocr_t *const ocrIn);
void cxa_atmega_timer_ocr_disableInterrupt_compareMatch(cxa_atmega_timer_ocr_t *const ocrIn);


/**
 * @private
 */
void cxa_atmega_timer_ocr_init(cxa_atmega_timer_ocr_t *const ocrIn, cxa_atmega_timer_t *const parentIn);

/**
 * @private
 */
void cxa_atmega_timer_ocr_handleInterrupt_compareMatch(cxa_atmega_timer_ocr_t *const ocrIn);

#endif
