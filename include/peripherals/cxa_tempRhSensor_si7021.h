/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_TEMPSENSOR_SI7021_H_
#define CXA_TEMPSENSOR_SI7021_H_


// ******** includes ********
#include <cxa_i2cMaster.h>
#include <cxa_tempSensor.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_tempSensor_t super;

	cxa_i2cMaster_t* i2c;
}cxa_tempRhSensor_si7021_t;


// ******** global function prototypes ********
void cxa_tempRhSensor_si7021_init(cxa_tempRhSensor_si7021_t *const siIn, cxa_i2cMaster_t *const i2cIn);


#endif
