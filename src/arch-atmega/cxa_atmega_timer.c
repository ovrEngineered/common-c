/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include <cxa_assert.h>
#include <cxa_atmega_timer.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static bool is16BitTimer(cxa_atmega_timer_id_t idIn);
static uint16_t getPrescalerValueFromPrescaler(const cxa_atmega_timer_prescaler_t prescaleIn);
static uint8_t getModeBitsFromTimerAndMode(const cxa_atmega_timer_id_t idIn, const cxa_atmega_timer_mode_t modeIn);
static bool getPrescaleBitsTimerAndPrescale(const cxa_atmega_timer_id_t idIn, const cxa_atmega_timer_prescaler_t prescaleIn, uint8_t* prescaleBitsOut);

static void handleOverflowInterruptWithTimer(cxa_atmega_timer_t *const timerIn);


// ********  local variable declarations *********
static cxa_atmega_timer_t* timer0 = NULL;
static cxa_atmega_timer_t* timer1 = NULL;
static cxa_atmega_timer_t* timer2 = NULL;


// ******** global function implementations ********
void cxa_atmega_timer_init(cxa_atmega_timer_t *const timerIn, const cxa_atmega_timer_id_t idIn, const cxa_atmega_timer_mode_t modeIn, const cxa_atmega_timer_prescaler_t prescaleIn)
{
	cxa_assert(timerIn);

	// save our references and initialize our state
	timerIn->id = idIn;
	timerIn->prescaler = prescaleIn;
	timerIn->mode = modeIn;
	cxa_array_initStd(&timerIn->listeners, timerIn->listeners_raw);

	// setup our input capture and output compare units
	cxa_atmega_timer_icp_init(&timerIn->icp, timerIn);
	cxa_atmega_timer_ocr_init(&timerIn->ocrA, timerIn);
	cxa_atmega_timer_ocr_init(&timerIn->ocrB, timerIn);

	uint8_t modeBits = getModeBitsFromTimerAndMode(timerIn->id, timerIn->mode);
	uint8_t prescaleBits;
	cxa_assert(getPrescaleBitsTimerAndPrescale(timerIn->id, timerIn->prescaler, &prescaleBits));

	switch( timerIn->id )
	{
		case CXA_ATM_TIMER_0:
			TCCR0A = ((modeBits & 0x03) << 0);
			TCCR0B = (((modeBits >> 2) & 0x03) << 3) | (prescaleBits << 0);
			TIMSK0 = 0;

			timer0 = timerIn;			// save for interrupts
			break;

		case CXA_ATM_TIMER_1:
			TCCR1A = ((modeBits & 0x03) << 0);
			TCCR1B = (((modeBits >> 2) & 0x03) << 3) | (prescaleBits << 0);
			TIMSK1 = 0;

			timer1 = timerIn;			// save for interrupts
			break;

		case CXA_ATM_TIMER_2:
			TCCR2A = ((modeBits & 0x03) << 0);
			TCCR2B = (((modeBits >> 2) & 0x03) << 3) | (prescaleBits << 0);
			TIMSK2 = 0;

			timer2 = timerIn;			// save for interrupts
			break;
	}
}


uint16_t cxa_atmega_timer_getCurrentCounts(cxa_atmega_timer_t *const timerIn)
{
	cxa_assert(timerIn);

	uint16_t retVal = 0;
	switch( timerIn->id )
	{
		case CXA_ATM_TIMER_0:
			retVal = TCNT0;
			break;

		case CXA_ATM_TIMER_1:
			retVal = TCNT1;
			break;

		case CXA_ATM_TIMER_2:
			retVal = TCNT2;
			break;
	}
	return retVal;
}


bool cxa_atmega_timer_setPrescalar(cxa_atmega_timer_t *const timerIn, const cxa_atmega_timer_prescaler_t prescaleIn)
{
	cxa_assert(timerIn);

	timerIn->prescaler  = prescaleIn;

	uint8_t prescaleBits;
	if( !getPrescaleBitsTimerAndPrescale(timerIn->id, timerIn->prescaler, &prescaleBits) ) return false;

	switch( timerIn->id )
	{
		case CXA_ATM_TIMER_0:
			TCCR0B =  (TCCR0B & 0xF8) | (prescaleBits << 0);
			break;

		case CXA_ATM_TIMER_1:
			TCCR1B = (TCCR1B & 0xF8) | (prescaleBits << 0);
			break;

		case CXA_ATM_TIMER_2:
			TCCR2B = (TCCR2B & 0xF8) | (prescaleBits << 0);
			break;
	}
	return true;
}


void cxa_atmega_timer_addListener(cxa_atmega_timer_t *const timerIn, cxa_atmega_timer_cb_onOverflow_t cb_onOverflowIn, void* userVarIn)
{
	cxa_assert(timerIn);

	cxa_atmega_timer_listenerEntry_t newEntry = {
			.cb_onOverflow = cb_onOverflowIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&timerIn->listeners, &newEntry));
}


cxa_atmega_timer_ocr_t* cxa_atmega_timer_getOcrA(cxa_atmega_timer_t const* timerIn)
{
	cxa_assert(timerIn);

	return (cxa_atmega_timer_ocr_t*)&timerIn->ocrA;
}


cxa_atmega_timer_ocr_t* cxa_atmega_timer_getOcrB(cxa_atmega_timer_t const* timerIn)
{
	cxa_assert(timerIn);

	return (cxa_atmega_timer_ocr_t*)&timerIn->ocrB;
}


cxa_atmega_timer_icp_t* cxa_atmega_timer_getIcp(cxa_atmega_timer_t const* timerIn)
{
	cxa_assert(timerIn);
	cxa_assert(timerIn->id == CXA_ATM_TIMER_1);

	return (cxa_atmega_timer_icp_t*)&timerIn->icp;
}


void cxa_atmega_timer_enableInterrupt_overflow(cxa_atmega_timer_t *const timerIn)
{
	cxa_assert(timerIn);

	cli();
	switch( timerIn->id )
	{
		case CXA_ATM_TIMER_0:
			TIFR0 |= (1 << 0);
			TIMSK0 |= (1 << 0);
			break;

		case CXA_ATM_TIMER_1:
			TIFR1 |= (1 << 0);
			TIMSK1 |= (1 << 0);
			break;

		case CXA_ATM_TIMER_2:
			TIFR2 |= (1 << 0);
			TIMSK2 |= (1 << 0);
			break;
	}
	sei();
}


float cxa_atmega_timer_getOverflowPeriod_s(cxa_atmega_timer_t *const timerIn)
{
	cxa_assert(timerIn);

	uint32_t max_counts = 0;
	switch( timerIn->mode )
	{
		case CXA_ATM_TIMER_MODE_NORMAL:
		case CXA_ATM_TIMER_MODE_FASTPWM:
		case CXA_ATM_TIMER_MODE_CTC:
			max_counts = is16BitTimer(timerIn->id) ? UINT16_MAX : UINT8_MAX;
			break;
	}

	return cxa_atmega_timer_getTimerTickPeriod_s(timerIn) * max_counts;
}


float cxa_atmega_timer_getTimerTickPeriod_s(cxa_atmega_timer_t *const timerIn)
{
	cxa_assert(timerIn);

	return cxa_atmega_timer_getTimerTickPeriod_forPrescalar(timerIn->prescaler);
}


uint16_t cxa_atmega_timer_getMaxTicks(cxa_atmega_timer_t *const timerIn)
{
	cxa_assert(timerIn);

	return is16BitTimer(timerIn->id) ? UINT16_MAX : UINT8_MAX;
}


float cxa_atmega_timer_getTimerTickPeriod_forPrescalar(const cxa_atmega_timer_prescaler_t prescaleIn)
{
	if( prescaleIn == CXA_ATM_TIMER_PRESCALE_STOPPED ) return 0.0;

	return 1.0 / (((float)F_CPU) / ((float)getPrescalerValueFromPrescaler(prescaleIn)));
}


// ******** local function implementations ********
static bool is16BitTimer(cxa_atmega_timer_id_t idIn)
{
	return (idIn == CXA_ATM_TIMER_1);
}


static uint16_t getPrescalerValueFromPrescaler(const cxa_atmega_timer_prescaler_t prescaleIn)
{
	uint16_t retVal = 0;
	switch( prescaleIn )
	{
		case CXA_ATM_TIMER_PRESCALE_STOPPED:
			retVal = 0;
			break;

		case CXA_ATM_TIMER_PRESCALE_1:
			retVal = 1;
			break;

		case CXA_ATM_TIMER_PRESCALE_8:
			retVal = 8;
			break;

		case CXA_ATM_TIMER_PRESCALE_32:
			retVal = 32;
			break;

		case CXA_ATM_TIMER_PRESCALE_64:
			retVal = 64;
			break;

		case CXA_ATM_TIMER_PRESCALE_128:
			retVal = 128;
			break;

		case CXA_ATM_TIMER_PRESCALE_256:
			retVal = 256;
			break;

		case CXA_ATM_TIMER_PRESCALE_1024:
			retVal = 1024;
			break;
	}
	return retVal;
}


static uint8_t getModeBitsFromTimerAndMode(const cxa_atmega_timer_id_t idIn, const cxa_atmega_timer_mode_t modeIn)
{
	uint8_t retVal = 0;
	switch( idIn )
	{
		case CXA_ATM_TIMER_0:
			if( modeIn == CXA_ATM_TIMER_MODE_FASTPWM ) retVal = 0x03;
			else if( modeIn == CXA_ATM_TIMER_MODE_NORMAL ) retVal = 0x00;
			else if( modeIn == CXA_ATM_TIMER_MODE_CTC ) retVal = 0x02;
			else cxa_assert(0);
			break;

		case CXA_ATM_TIMER_1:
			if( modeIn == CXA_ATM_TIMER_MODE_FASTPWM ) retVal = 0x05;
			else if( modeIn == CXA_ATM_TIMER_MODE_NORMAL ) retVal = 0x00;
			else if( modeIn == CXA_ATM_TIMER_MODE_CTC ) retVal = 0x04;
			else cxa_assert(0);
			break;

		case CXA_ATM_TIMER_2:
			if( modeIn == CXA_ATM_TIMER_MODE_FASTPWM ) retVal = 0x03;
			else if( modeIn == CXA_ATM_TIMER_MODE_NORMAL ) retVal = 0x00;
			else if( modeIn == CXA_ATM_TIMER_MODE_CTC ) retVal = 0x02;
			else cxa_assert(0);
			break;
	}
	return retVal;
}


static bool getPrescaleBitsTimerAndPrescale(const cxa_atmega_timer_id_t idIn, const cxa_atmega_timer_prescaler_t prescaleIn, uint8_t* prescaleBitsOut)
{
	uint8_t retVal = 0;
	switch( prescaleIn )
	{
		case CXA_ATM_TIMER_PRESCALE_STOPPED:
			if( idIn == CXA_ATM_TIMER_0 ) retVal = 0;
			else if( idIn == CXA_ATM_TIMER_1 ) retVal = 0;
			else if( idIn == CXA_ATM_TIMER_2 ) retVal = 0;
			break;

		case CXA_ATM_TIMER_PRESCALE_1:
			if( idIn == CXA_ATM_TIMER_0 ) retVal = 1;
			else if( idIn == CXA_ATM_TIMER_1 ) retVal = 1;
			else if( idIn == CXA_ATM_TIMER_2 ) retVal = 1;
			break;

		case CXA_ATM_TIMER_PRESCALE_8:
			if( idIn == CXA_ATM_TIMER_0 ) retVal = 2;
			else if( idIn == CXA_ATM_TIMER_1 ) retVal = 2;
			else if( idIn == CXA_ATM_TIMER_2 ) retVal = 2;
			break;

		case CXA_ATM_TIMER_PRESCALE_32:
			if( idIn == CXA_ATM_TIMER_0 ) return false;
			else if( idIn == CXA_ATM_TIMER_1 ) return false;
			else if( idIn == CXA_ATM_TIMER_2 ) retVal = 3;
			break;

		case CXA_ATM_TIMER_PRESCALE_64:
			if( idIn == CXA_ATM_TIMER_0 ) retVal = 3;
			else if( idIn == CXA_ATM_TIMER_1 ) retVal = 3;
			else if( idIn == CXA_ATM_TIMER_2 ) retVal = 4;
			break;

		case CXA_ATM_TIMER_PRESCALE_128:
			if( idIn == CXA_ATM_TIMER_0 ) return false;
			else if( idIn == CXA_ATM_TIMER_1 ) return false;
			else if( idIn == CXA_ATM_TIMER_2 ) retVal = 5;
			break;

		case CXA_ATM_TIMER_PRESCALE_256:
			if( idIn == CXA_ATM_TIMER_0 ) retVal = 4;
			else if( idIn == CXA_ATM_TIMER_1 ) retVal = 4;
			else if( idIn == CXA_ATM_TIMER_2 ) retVal = 6;
			break;

		case CXA_ATM_TIMER_PRESCALE_1024:
			if( idIn == CXA_ATM_TIMER_0 ) retVal = 5;
			else if( idIn == CXA_ATM_TIMER_1 ) retVal = 5;
			else if( idIn == CXA_ATM_TIMER_2 ) retVal = 7;
			break;
	}
	if( prescaleBitsOut != NULL ) *prescaleBitsOut = retVal;
	return true;
}


static void handleOverflowInterruptWithTimer(cxa_atmega_timer_t *const timerIn)
{
	if( timerIn == NULL ) return;

	cxa_array_iterate(&timerIn->listeners, currListener, cxa_atmega_timer_listenerEntry_t)
	{
		if( (currListener != NULL) && (currListener->cb_onOverflow != NULL) ) currListener->cb_onOverflow(timerIn, currListener->userVar);
	}
}


// ******** interrupt implementations *******
ISR(TIMER0_OVF_vect)
{
	if( timer0 != NULL ) handleOverflowInterruptWithTimer(timer0);
}


ISR(TIMER1_OVF_vect)
{
	if( timer1 != NULL ) handleOverflowInterruptWithTimer(timer1);
}


ISR(TIMER2_OVF_vect)
{
	if( timer2 != NULL ) handleOverflowInterruptWithTimer(timer2);
}


ISR(TIMER0_COMPA_vect)
{
	if( timer0 != NULL ) cxa_atmega_timer_ocr_handleInterrupt_compareMatch(&timer0->ocrA);
}


ISR(TIMER0_COMPB_vect)
{
	if( timer0 != NULL ) cxa_atmega_timer_ocr_handleInterrupt_compareMatch(&timer0->ocrB);
}


ISR(TIMER1_COMPA_vect)
{
	if( timer1 != NULL ) cxa_atmega_timer_ocr_handleInterrupt_compareMatch(&timer1->ocrA);
}


ISR(TIMER1_COMPB_vect)
{
	if( timer1 != NULL ) cxa_atmega_timer_ocr_handleInterrupt_compareMatch(&timer1->ocrB);
}


ISR(TIMER2_COMPA_vect)
{
	if( timer2 != NULL ) cxa_atmega_timer_ocr_handleInterrupt_compareMatch(&timer2->ocrA);
}


ISR(TIMER2_COMPB_vect)
{
	if( timer2 != NULL ) cxa_atmega_timer_ocr_handleInterrupt_compareMatch(&timer2->ocrB);
}


ISR(TIMER1_CAPT_vect)
{
	if( timer1 != NULL ) cxa_atmega_timer_icp_handleInterrupt_inputCapture(&timer1->icp);
}
