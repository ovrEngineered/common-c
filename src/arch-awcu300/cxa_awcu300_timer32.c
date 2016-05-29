/**
 * Copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_awcu300_timer32.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_awcu300_timer32_init_freerun(cxa_awcu300_timer32_t *const timerIn, GPT_ID_Type timerIdIn, uint8_t clockDivIn)
{
	cxa_assert(timerIn);
	cxa_assert( (timerIdIn == GPT0_ID) ||
				(timerIdIn == GPT1_ID) ||
				(timerIdIn == GPT2_ID) );

	// save our references
	timerIn->id = timerIdIn;
	timerIn->clockDiv = clockDivIn;
	
	CLK_Module_Type clk = CLK_GPT0;
	switch( timerIn->id )
	{
		case GPT0_ID:
			clk = CLK_GPT0;
			break;

		case GPT1_ID:
			clk = CLK_GPT1;
			break;

		case GPT2_ID:
			clk = CLK_GPT2;
			break;

		case GPT3_ID:
			clk = CLK_GPT3;
			break;
	}

	// configure the module input clock
	CLK_GPTInternalClkSrc(timerIn->id, CLK_SYSTEM);
	CLK_ModuleClkDivider(clk, 1);
	CLK_ModuleClkEnable(clk);

	// configure the actual GPT
	GPT_Config_Type gptConf =
			{
				.clockDivider = clockDivIn,
				.clockPrescaler = 0,
				.clockSrc = GPT_CLOCK_0,
				.cntUpdate = GPT_CNT_VAL_UPDATE_NORMAL,
				.uppVal = UINT32_MAX
			};
	GPT_Init(timerIn->id, &gptConf);
	GPT_Start(timerIn->id);
}


uint32_t cxa_awcu300_timer32_getCount(cxa_awcu300_timer32_t *const timerIn)
{	
	cxa_assert(timerIn);

	return GPT_GetCounterVal(timerIn->id);
}


uint32_t cxa_awcu300_timer32_getResolution_cntsPerS(cxa_awcu300_timer32_t *const timerIn)
{
	cxa_assert(timerIn);

	return board_cpu_freq() / (((uint32_t)1) << timerIn->clockDiv);
}


uint32_t cxa_awcu300_timer32_getMaxVal_cnts(cxa_awcu300_timer32_t *const timerIn)
{
	cxa_assert(timerIn);

	return UINT32_MAX;
}


// ******** local function implementations ********

