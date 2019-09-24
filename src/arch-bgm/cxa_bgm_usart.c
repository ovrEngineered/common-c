/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include <cxa_bgm_usart.h>


// ******** includes ********
#include <stdbool.h>

#include <cxa_array.h>
#include <cxa_assert.h>
#include <cxa_delay.h>
#include <cxa_numberUtils.h>

#include "em_cmu.h"

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	USART_TypeDef *usart_raw;
	cxa_bgm_usart_t* usart_bgm;
}rawUsartToBgmMapEntry_t;


// ******** local function prototypes ********
static void initSystem(void);
static void commonInit(cxa_bgm_usart_t *const usartIn, USART_TypeDef* uartIdIn,
		   const uint32_t baudRate_bpsIn,
		   const GPIO_Port_TypeDef txPortNumIn, const unsigned int txPinNumIn, uint32_t txLocIn,
		   const GPIO_Port_TypeDef rxPortNumIn, const unsigned int rxPinNumIn, uint32_t rxLocIn,
		   bool useHwFlowControlIn,
		   const GPIO_Port_TypeDef rtsPortNumIn, const unsigned int rtsPinNumIn, uint32_t rtsLocIn,
		   const GPIO_Port_TypeDef ctsPortNumIn, const unsigned int ctsPinNumIn, uint32_t ctsLocIn);

static void setBgmForUsart(USART_TypeDef *usart_rawIn, cxa_bgm_usart_t* usart_bgmIn);
static cxa_bgm_usart_t* getBgmForUsart(USART_TypeDef *usart_rawIn);

static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);

static void handleIsr_rx(USART_TypeDef *usart_rawIn);


// ********  local variable declarations *********
static bool isSystemInit = false;
static cxa_array_t rawUsartToBgmMap;
static rawUsartToBgmMapEntry_t rawUsartToBgmMap_raw[3];


// ******** global function implementations ********
void cxa_bgm_usart_init_noHH(cxa_bgm_usart_t *const usartIn, USART_TypeDef* uartIdIn,
							 const uint32_t baudRate_bpsIn,
							 const GPIO_Port_TypeDef txPortNumIn, const unsigned int txPinNumIn, uint32_t txLocIn,
							 const GPIO_Port_TypeDef rxPortNumIn, const unsigned int rxPinNumIn, uint32_t rxLocIn)
{
	commonInit(usartIn, uartIdIn,
				   baudRate_bpsIn,
				   txPortNumIn, txPinNumIn, txLocIn,
				   rxPortNumIn, rxPinNumIn, rxLocIn,
				   false,
				   0, 0, 0,
				   0, 0, 0);
}


void cxa_bgm_usart_init_HH(cxa_bgm_usart_t *const usartIn, USART_TypeDef* uartIdIn,
						   const uint32_t baudRate_bpsIn,
						   const GPIO_Port_TypeDef txPortNumIn, const unsigned int txPinNumIn, uint32_t txLocIn,
						   const GPIO_Port_TypeDef rxPortNumIn, const unsigned int rxPinNumIn, uint32_t rxLocIn,
						   const GPIO_Port_TypeDef rtsPortNumIn, const unsigned int rtsPinNumIn, uint32_t rtsLocIn,
						   const GPIO_Port_TypeDef ctsPortNumIn, const unsigned int ctsPinNumIn, uint32_t ctsLocIn)
{
	commonInit(usartIn, uartIdIn,
			   baudRate_bpsIn,
			   txPortNumIn, txPinNumIn, txLocIn,
			   rxPortNumIn, rxPinNumIn, rxLocIn,
			   true,
			   rtsPortNumIn, rtsPinNumIn, rtsLocIn,
			   ctsPortNumIn, ctsPinNumIn, ctsLocIn);
}


// ******** local function implementations ********
static void initSystem(void)
{
	cxa_array_initStd(&rawUsartToBgmMap, rawUsartToBgmMap_raw);
	isSystemInit = true;
}


static void commonInit(cxa_bgm_usart_t *const usartIn, USART_TypeDef* uartIdIn,
		   const uint32_t baudRate_bpsIn,
		   const GPIO_Port_TypeDef txPortNumIn, const unsigned int txPinNumIn, uint32_t txLocIn,
		   const GPIO_Port_TypeDef rxPortNumIn, const unsigned int rxPinNumIn, uint32_t rxLocIn,
		   bool useHwFlowControlIn,
		   const GPIO_Port_TypeDef rtsPortNumIn, const unsigned int rtsPinNumIn, uint32_t rtsLocIn,
		   const GPIO_Port_TypeDef ctsPortNumIn, const unsigned int ctsPinNumIn, uint32_t ctsLocIn)
{
	cxa_assert(usartIn);
	cxa_assert(uartIdIn);

	// make sure our system is initialized
	if( !isSystemInit ) initSystem();

	// save our references
	usartIn->uartId = uartIdIn;
	setBgmForUsart(usartIn->uartId, usartIn);
	usartIn->rxOverflow = false;
	usartIn->rxFifoOverflow = false;
	usartIn->rxUnderflow = false;

	// initialize our hardware
	USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
	init.baudrate = baudRate_bpsIn;
	if( useHwFlowControlIn ) init.hwFlowControl = usartHwFlowControlCtsAndRts;

	// Enable oscillator to GPIO and USART modules
	CMU_ClockEnable(cmuClock_GPIO, true);
	if( usartIn->uartId == USART0 )
	{
		CMU_ClockEnable(cmuClock_USART0, true);
	}
	else if( usartIn->uartId == USART1 )
	{
		CMU_ClockEnable(cmuClock_USART1, true);
	}
#ifdef USART2
	else if( usartIn->uartId == USART2 )
	{
		CMU_ClockEnable(cmuClock_USART2, true);
	}
#endif

	// set pin modes for USART TX and RX pins
	GPIO_PinModeSet(txPortNumIn, txPinNumIn, gpioModePushPull, 1);
	GPIO_PinModeSet(rxPortNumIn, rxPinNumIn, gpioModeInput, 0);

	// set pin modes for USART CTS and RTS pins (if needed)
	if( useHwFlowControlIn )
	{
		GPIO_PinModeSet(rtsPortNumIn, rtsPinNumIn, gpioModePushPull, 1);
		GPIO_PinModeSet(ctsPortNumIn, ctsPinNumIn, gpioModeInput, 0);
	}

	// Initialize USART asynchronous mode and route pins
	USART_InitAsync(usartIn->uartId, &init);
	usartIn->uartId->ROUTEPEN |= USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_RXPEN;
	usartIn->uartId->ROUTELOC0 = rxLocIn | txLocIn;

	// route CTS and RTS pins
	if( useHwFlowControlIn )
	{
		usartIn->uartId->ROUTEPEN |= USART_ROUTEPEN_CTSPEN | USART_ROUTEPEN_RTSPEN;
		usartIn->uartId->ROUTELOC1 = ctsLocIn | rtsLocIn;
		usartIn->uartId->CTRLX |= USART_CTRLX_CTSEN;
	}

	// setup our fifo
	cxa_fixedFifo_initStd(&usartIn->fifo_rx, CXA_FF_ON_FULL_DROP, usartIn->fifo_rx_raw);

	// setup our ioStream
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);

	// setup our interrupts
	USART_IntEnable(usartIn->uartId, USART_IEN_RXDATAV | USART_IEN_RXOF | USART_IEN_RXUF);

	// enable the USART Interrupts
	if( usartIn->uartId == USART0 )
	{
		NVIC_EnableIRQ(USART0_RX_IRQn);
	}
	else if( usartIn->uartId == USART1 )
	{
		NVIC_EnableIRQ(USART1_RX_IRQn);
	}
#ifdef USART2
	else if( usartIn->uartId == USART2 )
	{
		NVIC_EnableIRQ(USART2_RX_IRQn);
	}
#endif
}


static void setBgmForUsart(USART_TypeDef *usart_rawIn, cxa_bgm_usart_t* usart_bgmIn)
{
	cxa_assert(isSystemInit);
	rawUsartToBgmMapEntry_t newEntry = { .usart_raw = usart_rawIn, .usart_bgm = usart_bgmIn };
	cxa_assert(cxa_array_append(&rawUsartToBgmMap, &newEntry));
}


static cxa_bgm_usart_t* getBgmForUsart(USART_TypeDef *usart_rawIn)
{
	cxa_assert(isSystemInit);

	cxa_bgm_usart_t* retVal = NULL;

	cxa_array_iterate(&rawUsartToBgmMap, currEntry, rawUsartToBgmMapEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->usart_raw == usart_rawIn )
		{
			retVal = currEntry->usart_bgm;
			break;
		}
	}

	return retVal;
}


static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_bgm_usart_t* usartIn = (cxa_bgm_usart_t*)userVarIn;
	cxa_assert(usartIn);

	cxa_ioStream_readStatus_t retVal = CXA_IOSTREAM_READSTAT_NODATA;
	uint8_t readByte;
	if( usartIn->rxOverflow || usartIn->rxFifoOverflow || usartIn->rxUnderflow )
	{
		cxa_logger_stepDebug_msg("%d  %d  %d", usartIn->rxOverflow, usartIn->rxFifoOverflow, usartIn->rxUnderflow);
		usartIn->rxOverflow = false;
		usartIn->rxFifoOverflow = false;
		usartIn->rxUnderflow = false;
		USART_Reset(usartIn->uartId);
		retVal = CXA_IOSTREAM_READSTAT_ERROR;
	}
	else if( cxa_fixedFifo_dequeue(&usartIn->fifo_rx, &readByte) )
	{
		if( byteOut != NULL ) *byteOut = readByte;
		retVal = CXA_IOSTREAM_READSTAT_GOTDATA;
	}

	return retVal;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_bgm_usart_t* usartIn = (cxa_bgm_usart_t*)userVarIn;
	cxa_assert(usartIn);

	for( uint8_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		USART_Tx(usartIn->uartId, ((uint8_t*)buffIn)[i]);
	}

	return true;
}


static void handleIsr_rx(USART_TypeDef *usart_rawIn)
{
	// clear our flags right away

	uint32_t flags;
	flags = USART_IntGet(usart_rawIn);
	USART_IntClear(usart_rawIn, flags);

	cxa_bgm_usart_t* usartIn = getBgmForUsart(usart_rawIn);
	if( usartIn == NULL ) return;
	// if we made it here, we know about this usart and we have an object

	if( flags & USART_IEN_RXDATAV )
	{
		uint8_t rxByte;
		while( usartIn->uartId->STATUS & USART_STATUS_RXDATAV )
		{
			rxByte = (uint8_t)usartIn->uartId->RXDATA;
			if( !cxa_fixedFifo_queue(&usartIn->fifo_rx, &rxByte) )
			{
				usartIn->rxFifoOverflow = true;
				cxa_fixedFifo_clear(&usartIn->fifo_rx);
			}
		}
	}
	else if( flags & USART_IEN_RXOF )
	{
		usartIn->rxOverflow = true;
	}
	else if( flags & USART_IEN_RXUF )
	{
		usartIn->rxUnderflow = true;
	}

	// clear our flags one more time
	flags = USART_IntGet(usart_rawIn);
	USART_IntClear(usart_rawIn, flags);
}


// ******** Interrupt Handlers ********
void USART0_RX_IRQHandler(void) { handleIsr_rx(USART0); }
void USART1_RX_IRQHandler(void) { handleIsr_rx(USART1); }

#ifdef USART2
void USART2_RX_IRQHandler(void) { handleIsr_rx(USART2); }
#endif
