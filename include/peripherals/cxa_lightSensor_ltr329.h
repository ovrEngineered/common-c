/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_LIGHTSENSOR_LTR329_H_
#define CXA_LIGHTSENSOR_LTR329_H_


// ******** includes ********
#include <cxa_i2cMaster.h>
#include <cxa_lightSensor.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_lightSensor_t super;

	cxa_i2cMaster_t* i2c;

	uint16_t readVal;

	cxa_stateMachine_t stateMachine;
}cxa_lightSensor_ltr329_t;


// ******** global function prototypes ********
void cxa_lightSensor_ltr329_init(cxa_lightSensor_ltr329_t *const lightSnsIn, cxa_i2cMaster_t *const i2cIn, int threadIdIn);


#endif
