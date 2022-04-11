/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ATMEGA_TIMER_ICP_H_
#define CXA_ATMEGA_TIMER_ICP_H_


// ******** includes ********
#include <cxa_array.h>
#include <stdint.h>


// ******** global macro definitions ********
#ifndef CXA_ATMEGA_TIMER_ICP_MAXNUM_LISTENERS
#define CXA_ATMEGA_TIMER_ICP_MAXNUM_LISTENERS			1
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_atmega_timer_icp cxa_atmega_timer_icp_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER_ICP_FALLING = 0,
	CXA_ATM_TIMER_ICP_RISING = 1
}cxa_atmega_timer_icp_edgeType_t;


/**
 * @public
 */
typedef void (*cxa_atmega_timer_icp_cb_onInputCapture_t)(cxa_atmega_timer_icp_t *const icpIn, void *userVarIn);


/**
 * @private
 */
typedef struct cxa_atmega_timer cxa_atmega_timer_t;


/**
 * @private
 */
typedef struct
{
	cxa_atmega_timer_icp_cb_onInputCapture_t cb_onInputCapture;
	void* userVar;
}cxa_atmega_timer_icp_listenerEntry_t;


/**
 * @private
 */
struct cxa_atmega_timer_icp
{
	cxa_atmega_timer_t* parent;

	cxa_array_t listeners;
	cxa_atmega_timer_icp_listenerEntry_t listeners_raw[CXA_ATMEGA_TIMER_ICP_MAXNUM_LISTENERS];
};


// ******** global function prototypes ********
void cxa_atmega_timer_icp_configure(cxa_atmega_timer_icp_t *const icpIn, const cxa_atmega_timer_icp_edgeType_t edgeIn);
void cxa_atmega_timer_icp_swapEdge(cxa_atmega_timer_icp_t *const icpIn);

void cxa_atmega_timer_icp_addListener(cxa_atmega_timer_icp_t *const icpIn, cxa_atmega_timer_icp_cb_onInputCapture_t cb_onInputCaptureIn, void* userVarIn);

void cxa_atmega_timer_icp_enableInterrupt_inputCapture(cxa_atmega_timer_icp_t *const icpIn);

uint16_t cxa_atmega_timer_icp_getValue(cxa_atmega_timer_icp_t *const icpIn);


/**
 * @private
 */
void cxa_atmega_timer_icp_init(cxa_atmega_timer_icp_t *const icpIn, cxa_atmega_timer_t *const parentIn);

/**
 * @private
 */
void cxa_atmega_timer_icp_handleInterrupt_inputCapture(cxa_atmega_timer_icp_t *const icpIn);

#endif
