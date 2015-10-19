/**
 * Copyright 2015 opencxa.org
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
 *
 * @author Christopher Armenio
 */
#include "cxa_ble112_dmaController.h"


// ******** includes ********
#include <cxa_assert.h>
#include <blestack/blestack.h>
#include <blestack/hw.h>
#include <blestack/dma.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	cxa_ble112_dma_channel_t dmaChannel;
	bool isUsed;

	cxa_ble112_dmaTransferComplete_cb_t cb_complete;
	void *userVar;
}dmaChannelEntry_t;


// ******** local function prototypes ********
static dmaChannelEntry_t* getChannelEntry_byChannel(cxa_ble112_dma_channel_t channelIn);


// ********  local variable declarations *********
static bool isInit = false;
static dmaChannelEntry_t dmaChannels[] = {
		{.dmaChannel=CXA_BLE112_DMA_CHANNEL_0, .isUsed=true, .cb_complete=NULL, .userVar=NULL},		// used by copy dma
		{.dmaChannel=CXA_BLE112_DMA_CHANNEL_1, .isUsed=true, .cb_complete=NULL, .userVar=NULL},		// AES input data
		{.dmaChannel=CXA_BLE112_DMA_CHANNEL_2, .isUsed=true, .cb_complete=NULL, .userVar=NULL},		// AES output data
		{.dmaChannel=CXA_BLE112_DMA_CHANNEL_3, .isUsed=false, .cb_complete=NULL, .userVar=NULL},
		{.dmaChannel=CXA_BLE112_DMA_CHANNEL_4, .isUsed=false, .cb_complete=NULL, .userVar=NULL}
};


// ******** global function implementations ********
void cxa_ble112_dma_init(void)
{
	if( isInit ) return;

	dma_init();
	isInit = true;
}


cxa_ble112_dma_channel_t cxa_ble112_dma_reserveUnusedChannel(void)
{
	if( !isInit ) cxa_ble112_dma_init();

	for( size_t i = 0; i < sizeof(dmaChannels)/sizeof(*dmaChannels); i++)
	{
		dmaChannelEntry_t* currEntry = &(dmaChannels[i]);
		if( currEntry == NULL ) continue;

		if( !currEntry->isUsed ) return currEntry->dmaChannel;
	}

	// if we made it here, we don't have any free DMA channels
	return CXA_BLE112_DMA_CHANNEL_UNKNOWN;
}


void cxa_ble112_dma_freeUsedChannel(cxa_ble112_dma_channel_t channelIn)
{
	if( !isInit ) cxa_ble112_dma_init();

	dmaChannelEntry_t* targetChannel = getChannelEntry_byChannel(channelIn);
	cxa_assert(targetChannel);

	targetChannel->isUsed = false;
}


void cxa_ble112_dma_armChannelTransfer_fixedLength(cxa_ble112_dma_channel_t channelIn,
												   void* sourceAddrIn, cxa_ble112_dma_incrementMode_t sourceIncrementModeIn,
												   void* destAddrIn, cxa_ble112_dma_incrementMode_t destIncrementModeIn,
												   size_t numBytesIn, cxa_ble112_dma_triggerSource_t triggerSrcIn,
												   cxa_ble112_dma_transferPriority_t priorityIn,
												   cxa_ble112_dmaTransferComplete_cb_t cb_completeIn, void* userVarIn)
{
	if( !isInit ) cxa_ble112_dma_init();

	dmaChannelEntry_t* targetChannel = getChannelEntry_byChannel(channelIn);
	cxa_assert(targetChannel);

	targetChannel->cb_complete = cb_completeIn;
	targetChannel->userVar = userVarIn;

	// actually setup our dmaConf struture
	struct dma_conf* targetConf = &dma[channelIn];
	targetConf->src_hi = ((uint16_t)sourceAddrIn) >> 8;
	targetConf->src_lo = ((uint16_t)sourceAddrIn) & 0xFF;
	targetConf->dst_hi = ((uint16_t)destAddrIn) >> 8;
	targetConf->dst_lo = ((uint16_t)destAddrIn) & 0xFF;

	targetConf->len_hi = (numBytesIn >> 8) & 0x1F;
	targetConf->len_lo = (numBytesIn & 0xFF);

	targetConf->mode = triggerSrcIn;
	targetConf->flags = (sourceIncrementModeIn << 6) | (destIncrementModeIn << 4) | (1 << 3) | priorityIn;

	// arm it!
	dma_arm(channelIn);
}


void cxa_ble112_dma_abortChannelTransfer(cxa_ble112_dma_channel_t channelIn)
{
	if( !isInit ) cxa_ble112_dma_init();

	dmaChannelEntry_t* targetChannel = getChannelEntry_byChannel(channelIn);
	cxa_assert(targetChannel);

	targetChannel->cb_complete = NULL;
	targetChannel->userVar = NULL;

	DMAARM |= (1 << channelIn);
	DMAARM |= 0x80;
	DMAARM |= 0x00;
}


// ******** local function implementations ********
static dmaChannelEntry_t* getChannelEntry_byChannel(cxa_ble112_dma_channel_t channelIn)
{
	for( size_t i = 0; i < sizeof(dmaChannels)/sizeof(*dmaChannels); i++)
	{
		dmaChannelEntry_t* currEntry = &(dmaChannels[i]);
		if( currEntry == NULL ) continue;

		if( currEntry->dmaChannel == channelIn ) return currEntry;
	}

	return NULL;
}


#pragma vector=DMA_VECTOR
__interrupt void isr_dmaComplete(void)
{
	cxa_ble112_dma_channel_t intChan = CXA_BLE112_DMA_CHANNEL_UNKNOWN;
	if( DMAIRQ & 0x10 )
	{
		// channel 4 interrupt
		intChan = CXA_BLE112_DMA_CHANNEL_4;
		DMAIRQ &= ~(0x10);
	}
	else if( DMAIRQ & 0x08 )
	{
		// channel 3 interrupt
		intChan = CXA_BLE112_DMA_CHANNEL_3;
		DMAIRQ &= ~(0x08);
	}
	else if( DMAIRQ & 0x04 )
	{
		// channel 2 interrupt
		intChan = CXA_BLE112_DMA_CHANNEL_2;
		DMAIRQ &= ~(0x04);
	}
	else if( DMAIRQ & 0x02)
	{
		// channel 1 interrupt
		intChan = CXA_BLE112_DMA_CHANNEL_1;
		DMAIRQ &= ~(0x02);
	}
	else if( DMAIRQ & 0x01 )
	{
		// channel 0 interrupt
		intChan = CXA_BLE112_DMA_CHANNEL_0;
		DMAIRQ &= ~(0x01);
	}
	if( intChan == CXA_BLE112_DMA_CHANNEL_UNKNOWN ) return;

	// call our callback if it exists
	dmaChannelEntry_t* targetEntry = getChannelEntry_byChannel(intChan);
	if( targetEntry->cb_complete != NULL )
	{
		struct dma_conf* targetConf = &dma[intChan];

		uint16_t dest_raw = (((uint16_t)targetConf->dst_hi) << 8) | ((uint16_t)targetConf->dst_lo);
		size_t numBytes = (((uint16_t)(targetConf->len_hi & 0x1F)) << 8) | ((uint16_t)targetConf->len_lo);
		targetEntry->cb_complete(intChan, (void*)dest_raw, numBytes, targetEntry->userVar);
	}

	// clear our DMA interrupt flag
	DMAIF = 0;
}
