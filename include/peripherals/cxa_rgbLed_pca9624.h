/**
 * @file
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
 * @copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RGBLED_PCA9624_H_
#define CXA_RGBLED_PCA9624_H_


// ******** includes ********
#include <cxa_rgbLed.h>
#include <cxa_oneShotTimer.h>
#include <cxa_pca9624.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_rgbLed_t super;

	int threadId;
	cxa_pca9624_t* pca;

	uint8_t maxBrightness;

	uint8_t chanIndex_r;
	uint8_t chanIndex_g;
	uint8_t chanIndex_b;

	cxa_oneShotTimer_t ost;

	struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
	}currColor;

	struct
	{
		uint16_t onPeriod_msIn;
		uint16_t offPeriod_msIn;
		uint8_t r;
		uint8_t g;
		uint8_t b;
	}blink;

	struct
	{
		uint16_t color1Period_msIn;
		uint8_t r1;
		uint8_t g1;
		uint8_t b1;

		uint16_t color2Period_msIn;
		uint8_t r2;
		uint8_t g2;
		uint8_t b2;
	}alternate;

	struct
	{
		uint8_t prevR;
		uint8_t prevG;
		uint8_t prevB;
	}flashOnce;
}cxa_rgbLed_pca9624_t;


// ******** global function prototypes ********
void cxa_rgbLed_pca9624_init(cxa_rgbLed_pca9624_t *const ledIn, int threadIdIn,
							cxa_pca9624_t *const pcaIn, uint8_t maxBrightnessIn,
							uint8_t chanIndex_rIn, uint8_t chanIndex_gIn, uint8_t chanIndex_bIn);


#endif /* CXA_RGBLED_PCA9624_H_ */
