/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ATMEGA_USART_H_
#define CXA_ATMEGA_USART_H_


// ******** includes ********
#include <stdbool.h>
#include <stdio.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_atmega_usart_t object
 */
typedef struct cxa_atmega_usart cxa_atmega_usart_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_USART_ID_0
}cxa_atmega_usart_id_t;


/**
 * @private
 */
struct cxa_atmega_usart
{
	cxa_usart_t super;

	cxa_atmega_usart_id_t usartId;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified USART for no hardware handshaking using the specified baud rate.
 *
 * @param[in] usartIn pointer to a pre-allocated USART object
 * @param[in] usartIdIn the UARTID specifying the desired UART
 * @param[in] baudRate_bpsIn the desired baud rate, in bits-per-second
 */
void cxa_atmega_usart_init_noHH(cxa_atmega_usart_t *const usartIn, cxa_atmega_usart_id_t usartIdIn, const uint32_t baudRate_bpsIn);


#endif
