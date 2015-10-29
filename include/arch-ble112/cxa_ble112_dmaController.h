/**
 * @file
 *
 * @copyright 2015 opencxa.org
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
#ifndef CXA_BLE112_DMA_H_
#define CXA_BLE112_DMA_H_


// ******** includes ********
#include <stddef.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef enum
{
	CXA_BLE112_DMA_CHANNEL_0=0,
	CXA_BLE112_DMA_CHANNEL_1=1,
	CXA_BLE112_DMA_CHANNEL_2=2,
	CXA_BLE112_DMA_CHANNEL_3=3,
	CXA_BLE112_DMA_CHANNEL_4=4,
	CXA_BLE112_DMA_CHANNEL_UNKNOWN
}cxa_ble112_dma_channel_t;


typedef enum
{
	CXA_BLE112_DMA_TRIGGERSRC_USART0_RX=14,
	CXA_BLE112_DMA_TRIGGERSRC_USART1_RX=16
}cxa_ble112_dma_triggerSource_t;


typedef enum
{
	CXA_BLE112_DMA_INCMODE_NONE=0,
	CXA_BLE112_DMA_INCMODE_PLUS1=1,
	CXA_BLE112_DMA_INCMODE_PLUS2=2,
	CXA_BLE112_DMA_INCMODE_MINUS1=3,
}cxa_ble112_dma_incrementMode_t;


typedef enum
{
	CXA_BLE112_DMA_XFERPRIORITY_LOW=0,
	CXA_BLE112_DMA_XFERPRIORITY_ASSURED=1,
	CXA_BLE112_DMA_XFERPRIORITY_HIGH=2
}cxa_ble112_dma_transferPriority_t;


typedef void (*cxa_ble112_dmaTransferComplete_cb_t)(cxa_ble112_dma_channel_t channelIn, void* destBufferIn, size_t numBytesIn, void* userVarIn);


// ******** global function prototypes ********
void cxa_ble112_dma_init(void);

cxa_ble112_dma_channel_t cxa_ble112_dma_reserveUnusedChannel(void);
void cxa_ble112_dma_freeUsedChannel(cxa_ble112_dma_channel_t channelIn);

void cxa_ble112_dma_armChannelTransfer_fixedLength(cxa_ble112_dma_channel_t channelIn,
												   void* sourceAddrIn, cxa_ble112_dma_incrementMode_t sourceIncrementModeIn,
												   void* destAddrIn, cxa_ble112_dma_incrementMode_t destIncrementModeIn,
												   size_t numBytesIn, cxa_ble112_dma_triggerSource_t triggerSrcIn,
												   cxa_ble112_dma_transferPriority_t priorityIn,
												   cxa_ble112_dmaTransferComplete_cb_t cb_completeIn, void* userVarIn);

void cxa_ble112_dma_abortChannelTransfer(cxa_ble112_dma_channel_t channelIn);


#endif // CXA_BLE112__DMA_H_
