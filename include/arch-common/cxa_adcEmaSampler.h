/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ADCEMASAMPLER_H_
#define CXA_ADCEMASAMPLER_H_


// Exponential moving average sampler


// ******** includes ********
#include <cxa_adcChannel.h>
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_adcEmaSampler cxa_adcEmaSampler_t;


/**
 * @public
 */
typedef struct
{
	float currAverage_v;
	float currDerivative_vps;
}cxa_adcEmaSampler_retVal_t;


/**
 * @private
 */
struct cxa_adcEmaSampler
{
	cxa_adcChannel_t* adcChan;

	float smoothingConstant;
	float samplePeriod_sec;

	float currVal_v;
	float prevVal_v;
};


// ******** global function prototypes ********
/**
 * @public
 *
 * @param smoothingConstantIn 0.0 (more weight to older values) - 1.0 (more weight to newer values)
 */
void cxa_adcEmaSampler_init(cxa_adcEmaSampler_t *const adcSampIn, cxa_adcChannel_t *const adcChanIn, float smoothingConstantIn, uint32_t samplePeriod_msIn, int threadIdIn);


/**
 * @public
 */
void cxa_adcEmaSampler_resetAverage(cxa_adcEmaSampler_t *const adcSampIn);


/**
 * @public
 */
cxa_adcEmaSampler_retVal_t cxa_adcEmaSampler_getCurrentAverage(cxa_adcEmaSampler_t *const adcSampIn);


#endif
