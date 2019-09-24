/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_BGM_USART_H_
#define CXA_BGM_USART_H_


// ******** includes ********
#include <stdbool.h>
#include <stdio.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>

#include <em_usart.h>
#include <em_gpio.h>


// ******** global macro definitions ********
#ifndef CXA_BGM_USART_RX_FIFO_SIZE_BYTES
#define CXA_BGM_USART_RX_FIFO_SIZE_BYTES		32
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_bgm_usart_t object
 */
typedef struct cxa_bgm_usart cxa_bgm_usart_t;


/**
 * @private
 */
struct cxa_bgm_usart
{
	cxa_usart_t super;

	USART_TypeDef* uartId;

	cxa_fixedFifo_t fifo_rx;
	uint8_t fifo_rx_raw[CXA_BGM_USART_RX_FIFO_SIZE_BYTES];

	bool rxOverflow;
	bool rxFifoOverflow;
	bool rxUnderflow;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified USART for no hardware handshaking using the specified baud rate.
 *
 * @param[in] usartIn pointer to a pre-allocated USART object
 * @param[in] uartIdIn the UARTID specifying the desired UART
 * @param[in] baudRate_bpsIn the desired baud rate, in bits-per-second
 */
void cxa_bgm_usart_init_noHH(cxa_bgm_usart_t *const usartIn, USART_TypeDef* uartIdIn,
							 const uint32_t baudRate_bpsIn,
							 const GPIO_Port_TypeDef txPortNumIn, const unsigned int txPinNumIn, uint32_t txLocIn,
							 const GPIO_Port_TypeDef rxPortNumIn, const unsigned int rxPinNumIn, uint32_t rxLocIn);

/**
 * @public
 * @brief Initializes the specified USART for hardware handshaking using the specified baud rate.
 *
 * @param[in] usartIn pointer to a pre-allocated USART object
 * @param[in] uartIdIn the UARTID specifying the desired UART
 * @param[in] baudRate_bpsIn the desired baud rate, in bits-per-second
 */
void cxa_bgm_usart_init_HH(cxa_bgm_usart_t *const usartIn, USART_TypeDef* uartIdIn,
						   const uint32_t baudRate_bpsIn,
						   const GPIO_Port_TypeDef txPortNumIn, const unsigned int txPinNumIn, uint32_t txLocIn,
						   const GPIO_Port_TypeDef rxPortNumIn, const unsigned int rxPinNumIn, uint32_t rxLocIn,
						   const GPIO_Port_TypeDef rtsPortNumIn, const unsigned int rtsPinNumIn, uint32_t rtsLocIn,
						   const GPIO_Port_TypeDef ctsPortNumIn, const unsigned int ctsPinNumIn, uint32_t ctsLocIn);

#endif
