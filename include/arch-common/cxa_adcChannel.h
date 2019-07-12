/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_adcChannel_H_
#define CXA_adcChannel_H_


// ******** includes ********
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <cxa_array.h>


// ******** global macro definitions ********
#ifndef CXA_ADCCHAN_MAXNUM_LISTENERS
#define CXA_ADCCHAN_MAXNUM_LISTENERS		1
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_adcChannel cxa_adcChannel_t;


/**
 * @public
 */
typedef void (*cxa_adcChannel_cb_conversionComplete_t)(cxa_adcChannel_t *const adcChanIn, bool wasSuccessfulIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn);


/**
 * @protected
 */
typedef void (*cxa_adcChannel_scm_startConversion_singleShot_t)(cxa_adcChannel_t *const superIn);


/**
 * @protected
 */
typedef uint16_t (*cxa_adcChannel_scm_getMaxRawValue_t)(cxa_adcChannel_t *const superIn);


/**
 * @private
 */
typedef struct
{
	cxa_adcChannel_cb_conversionComplete_t cb_convComp;

	void *userVar;
}cxa_adcChannel_listener_t;


/**
 * @private
 */
struct cxa_adcChannel
{
	cxa_array_t listeners;
	cxa_adcChannel_listener_t listeners_raw[CXA_ADCCHAN_MAXNUM_LISTENERS];

	struct
	{
		bool wasSuccessful;
		float voltage;
		uint16_t raw;
	}lastConversionValue;

	struct
	{
		cxa_adcChannel_scm_startConversion_singleShot_t startConv_ss;
		cxa_adcChannel_scm_getMaxRawValue_t getMaxRawValue;
	}scms;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_adcChannel_init(cxa_adcChannel_t* const adcChanIn,
						 cxa_adcChannel_scm_startConversion_singleShot_t scm_startConv_ssIn,
						 cxa_adcChannel_scm_getMaxRawValue_t scm_getMaxRawValueIn);


/**
 * @public
 */
void cxa_adcChannel_addListener(cxa_adcChannel_t *const adcChanIn,
						 cxa_adcChannel_cb_conversionComplete_t cb_convCompIn,
						 void* userVarIn);


/**
 * @public
 */
void cxa_adcChannel_startConversion_singleShot(cxa_adcChannel_t *const adcChanIn);


/**
 * @public
 */
bool cxa_adcChannel_wasLastConversionSuccessful(cxa_adcChannel_t *const adcChanIn);


/**
 * @public
 */
float cxa_adcChannel_getLastConversionValue_voltage(cxa_adcChannel_t *const adcChanIn);


/**
 * @public
 */
uint16_t cxa_adcChannel_getLastConversionValue_raw(cxa_adcChannel_t *const adcChanIn);


/**
 * @public
 */
uint16_t cxa_adcChannel_getMaxRawValue(cxa_adcChannel_t *const adcChanIn);


/**
 * @protected
 */
void cxa_adcChannel_notify_conversionComplete(cxa_adcChannel_t *const adcChanIn, bool wasSuccessfulIn, float voltageIn, const uint16_t rawValIn);


#endif /* CXA_ADCCHAN_H_ */
