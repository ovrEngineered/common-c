/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_LIGHTSENSOR_H_
#define CXA_LIGHTSENSOR_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_lightSensor cxa_lightSensor_t;


/**
 * @public
 */
typedef void (*cxa_lightSensor_cb_updatedValue_t)(cxa_lightSensor_t *const lightSnsIn, bool wasSuccessfulIn, bool valueDidChangeIn, uint8_t newLight_255In, void* userVarIn);


/**
 * @protected
 */
typedef bool (*cxa_lightSensor_scm_requestNewValue_t)(cxa_lightSensor_t *const superIn);


/**
 * @private
 */
struct cxa_lightSensor
{
	cxa_lightSensor_scm_requestNewValue_t scm_requestNewValue;

	bool wasLastReadSuccessful;
	uint8_t lastReading_255;

	cxa_lightSensor_cb_updatedValue_t cb_onUpdate;
	void* userVar;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_lightSensor_init(cxa_lightSensor_t *const lightSnsIn, cxa_lightSensor_scm_requestNewValue_t scm_requestNewValIn);


/**
 * @public
 */
bool cxa_lightSensor_getValue_withCallback(cxa_lightSensor_t *const lightSnsIn, cxa_lightSensor_cb_updatedValue_t cbIn, void* userVarIn);


/**
 * @public
 */
bool cxa_lightSensor_wasLastReadSuccessful(cxa_lightSensor_t *const lightSnsIn);


/**
 * @public
 */
uint8_t cxa_lightSensor_getLastValue_255(cxa_lightSensor_t *const lightSnsIn);


/**
 * @protected
 */
void cxa_lightSensor_notify_updatedValue(cxa_lightSensor_t *const lightSnsIn, bool wasSuccessfulIn, uint8_t newLight_255In);


#endif /* CXA_LIGHTSENSOR_H_ */
