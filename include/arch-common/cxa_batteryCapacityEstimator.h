/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_BATTERYCAPACITYESTIMATOR_H_
#define CXA_BATTERYCAPACITYESTIMATOR_H_


// ******** includes ********
#include <cxa_adcChannel.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_batteryCapacityEstimator cxa_batteryCapacityEstimator_t;


/**
 * @public
 */
typedef void (*cxa_batteryCapacityEstimator_cb_updatedValue_t)(cxa_batteryCapacityEstimator_t *const cbeIn, bool wasSuccessfulIn, float battPcntIn, void* userVarIn);


/**
 * @private
 */
struct cxa_batteryCapacityEstimator
{
	cxa_adcChannel_t* adc_battVoltage;

	float battVoltMult;
	float maxVoltage;
	float minVoltage;

	cxa_batteryCapacityEstimator_cb_updatedValue_t cb_updateValue;
	void* userVar;
};


// ******** global function prototypes ********
void cxa_batteryCapacityEstimator_init(cxa_batteryCapacityEstimator_t *const bceIn, cxa_adcChannel_t* adcIn, float battVoltMultIn, float maxVoltageIn, float minVoltageIn);

bool cxa_batteryCapacityEstimator_getValue_withCallback(cxa_batteryCapacityEstimator_t *const bceIn, cxa_batteryCapacityEstimator_cb_updatedValue_t cbIn, void* userVarIn);


#endif /* CXA_BATTERYCAPACITYESTIMATOR_H_ */
