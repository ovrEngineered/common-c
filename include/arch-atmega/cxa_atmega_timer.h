/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ATMEGA_TIMER_H_
#define CXA_ATMEGA_TIMER_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_atmega_timer_ocr.h>
#include <cxa_atmega_timer_icp.h>


// ******** global macro definitions ********
#ifndef CXA_ATMEGA_TIMER_MAXNUM_LISTENERS
#define CXA_ATMEGA_TIMER_MAXNUM_LISTENERS			1
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_atmega_timer cxa_atmega_timer_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER_0,
	CXA_ATM_TIMER_1,
	CXA_ATM_TIMER_2,
}cxa_atmega_timer_id_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER_MODE_NORMAL,
	CXA_ATM_TIMER_MODE_FASTPWM,
	CXA_ATM_TIMER_MODE_CTC
}cxa_atmega_timer_mode_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER_PRESCALE_STOPPED,
	CXA_ATM_TIMER_PRESCALE_1,
	CXA_ATM_TIMER_PRESCALE_8,
	CXA_ATM_TIMER_PRESCALE_32,
	CXA_ATM_TIMER_PRESCALE_64,
	CXA_ATM_TIMER_PRESCALE_128,
	CXA_ATM_TIMER_PRESCALE_256,
	CXA_ATM_TIMER_PRESCALE_1024,
}cxa_atmega_timer_prescaler_t;


/**
 * @public
 */
typedef void (*cxa_atmega_timer_cb_onOverflow_t)(cxa_atmega_timer_t *const timerIn, void *userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_atmega_timer_cb_onOverflow_t cb_onOverflow;
	void* userVar;
}cxa_atmega_timer_listenerEntry_t;


/**
 * @private
 */
struct cxa_atmega_timer
{
	cxa_atmega_timer_id_t id;
	cxa_atmega_timer_prescaler_t prescaler;
	cxa_atmega_timer_mode_t mode;

	cxa_atmega_timer_ocr_t ocrA;
	cxa_atmega_timer_ocr_t ocrB;

	cxa_atmega_timer_icp_t icp;

	cxa_array_t listeners;
	cxa_atmega_timer_listenerEntry_t listeners_raw[CXA_ATMEGA_TIMER_MAXNUM_LISTENERS];
};


// ******** global function prototypes ********
void cxa_atmega_timer_init(cxa_atmega_timer_t *const timerIn, const cxa_atmega_timer_id_t idIn, const cxa_atmega_timer_mode_t modeIn, const cxa_atmega_timer_prescaler_t prescaleIn);

bool cxa_atmega_timer_setPrescalar(cxa_atmega_timer_t *const timerIn, const cxa_atmega_timer_prescaler_t prescaleIn);

void cxa_atmega_timer_addListener(cxa_atmega_timer_t *const timerIn, cxa_atmega_timer_cb_onOverflow_t cb_onOverflowIn, void* userVarIn);

cxa_atmega_timer_ocr_t* cxa_atmega_timer_getOcrA(cxa_atmega_timer_t const* timerIn);
cxa_atmega_timer_ocr_t* cxa_atmega_timer_getOcrB(cxa_atmega_timer_t const* timerIn);
cxa_atmega_timer_icp_t* cxa_atmega_timer_getIcp(cxa_atmega_timer_t const* timerIn);

void cxa_atmega_timer_enableInterrupt_overflow(cxa_atmega_timer_t *const timerIn);

float cxa_atmega_timer_getOverflowPeriod_s(cxa_atmega_timer_t *const timerIn);
float cxa_atmega_timer_getTimerTickPeriod_s(cxa_atmega_timer_t *const timerIn);

uint16_t cxa_atmega_timer_getMaxTicks(cxa_atmega_timer_t *const timerIn);

float cxa_atmega_timer_getTimerTickPeriod_forPrescalar(const cxa_atmega_timer_prescaler_t prescaleIn);

#endif
