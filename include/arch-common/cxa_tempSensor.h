/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_TEMPSENSOR_H_
#define CXA_TEMPSENSOR_H_


// ******** includes ********
#include <stdbool.h>

#include <cxa_array.h>


// ******** global macro definitions ********
#define CXA_TEMPSENSE_CTOF(degCIn)			(((degCIn) * 1.8) + 32.0)


#ifndef CXA_TEMPSENSE_MAXNUM_LISTENERS
#define CXA_TEMPSENSE_MAXNUM_LISTENERS			1
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_tempSensor cxa_tempSensor_t;


/**
 * @public
 */
typedef void (*cxa_tempSensor_cb_updatedValue_t)(cxa_tempSensor_t *const tmpSnsIn, bool wasSuccessfulIn, bool valueDidChangeIn, float newTemp_degCIn, void* userVarIn);


/**
 * @protected
 */
typedef void (*cxa_tempSensor_scm_requestNewValue_t)(cxa_tempSensor_t *const superIn);


/**
 * @private
 */
typedef struct
{
	cxa_tempSensor_cb_updatedValue_t cb_onTempUpdate;
	void* userVar;
}cxa_tempSensor_listenerEntry_t;


/**
 * @private
 */
struct cxa_tempSensor
{
	cxa_tempSensor_scm_requestNewValue_t scm_requestNewValue;

	bool wasLastReadSuccessful;
	float lastReading_degC;

	cxa_array_t listeners;
	cxa_tempSensor_listenerEntry_t listeners_raw[CXA_TEMPSENSE_MAXNUM_LISTENERS];
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_tempSensor_init(cxa_tempSensor_t *const tmpSnsIn, cxa_tempSensor_scm_requestNewValue_t scm_requestNewValIn);


/**
 * @public
 */
void cxa_tempSensor_addListener(cxa_tempSensor_t *const tmpSnsIn, cxa_tempSensor_cb_updatedValue_t cbIn, void* userVarIn);


/**
 * @public
 */
void cxa_tempSensor_requestNewValueNow(cxa_tempSensor_t *const tmpSnsIn);


/**
 * @public
 */
bool cxa_tempSensor_wasLastReadSuccessful(cxa_tempSensor_t *const tmpSnsIn);


/**
 * @public
 */
float cxa_tempSensor_getLastValue_degC(cxa_tempSensor_t *const tmpSnsIn);


/**
 * @protected
 */
void cxa_tempSensor_notify_updatedValue(cxa_tempSensor_t *const tmpSnsIn, bool wasSuccessfulIn, float newTemp_degCIn);

#endif /* CXA_TEMPSENSOR_H_ */
