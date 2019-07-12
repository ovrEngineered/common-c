/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ESP32_USART_H_
#define CXA_ESP32_USART_H_


// ******** includes ********
#include <stdbool.h>
#include <stdio.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>

#include <driver/gpio.h>
#include <driver/uart.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_esp32_usart_t object
 */
typedef struct cxa_esp32_usart cxa_esp32_usart_t;


/**
 * @private
 */
struct cxa_esp32_usart
{
	cxa_usart_t super;

	uart_port_t uartId;
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
void cxa_esp32_usart_init_noHH(cxa_esp32_usart_t *const usartIn, uart_port_t uartIdIn, const uint32_t baudRate_bpsIn,
						  	  const gpio_num_t txPinIn, const gpio_num_t rxPinIn);

void cxa_esp32_usart_init_HH(cxa_esp32_usart_t *const usartIn, uart_port_t uartIdIn, const uint32_t baudRate_bpsIn,
							const gpio_num_t txPinIn, const gpio_num_t rxPinIn,
							const gpio_num_t rtsPinIn, const gpio_num_t ctsPinIn);


#endif
