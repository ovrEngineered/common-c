/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RGBLED_PCA9624_H_
#define CXA_RGBLED_PCA9624_H_


// ******** includes ********
#include <cxa_rgbLed_runLoop.h>
#include <cxa_oneShotTimer.h>
#include <cxa_pca9624.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_rgbLed_runLoop_t super;

	cxa_pca9624_t* pca;

	uint8_t maxBrightness;

	uint8_t chanIndex_r;
	uint8_t chanIndex_g;
	uint8_t chanIndex_b;
}cxa_rgbLed_pca9624_t;


// ******** global function prototypes ********
void cxa_rgbLed_pca9624_init(cxa_rgbLed_pca9624_t *const ledIn, int threadIdIn,
							cxa_pca9624_t *const pcaIn, uint8_t maxBrightnessIn,
							uint8_t chanIndex_rIn, uint8_t chanIndex_gIn, uint8_t chanIndex_bIn);


#endif /* CXA_RGBLED_PCA9624_H_ */
