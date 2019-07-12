/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */

/**
 * @file
 * This file contains prototypes and an architecture-specific implementation of a
 * Universal Serial Asynchronous Receiver/Transmitter (USART) object.
 *
 * @note This file contains functionality restricted to the CXA POSIX implementation.
 *
 * @note This file contains functionality in addition to that already provided in @ref cxa_usart.h
 *
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_posix_usart_t myUsart;
 * // initialize the usart on '/dev/cu.usbserial-FTE3XG86' (nonblocking in OSX) for no hardware
 * // handshaking with a baud rate of 115200bps
 * cxa_posix_usart_init_noHH(&usart, "/dev/cu.usbserial-FTE3XG86", B115200);
 *
 * // read byte
 * uint8_t myByte;
 * cxa_usart_read(&usart, (void*)&myByte, 1, NULL);
 * @endcode
 */
#ifndef CXA_POSIX_USART_H_
#define CXA_POSIX_USART_H_


// ******** includes ********
#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct
{
	cxa_usart_t super;

	int fd;
}cxa_posix_usart_t;


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified serial port for no hardware handshaking using the specified baud rate.
 * Defaults to 8n1 for databits / parity / stopbits respectively.
 *
 * @param[in] usartIn pointer to a pre-allocated USART object
 * @param[in] pathIn path to the target UART file device (eg. /dev/ttyUSB0)
 * @param[in] baudRateIn the desired baud rate as specified in termios.h (eg. B115200)
 *
 * @return true if successfully opened, false if not
 */
bool cxa_posix_usart_init_noHH(cxa_posix_usart_t *const usartIn, char *const pathIn, const int baudRateIn);


/**
 * Closes the specified serial port.
 *
 * @param[in] usartIn pointer to the pre-initialized serial port to close
 */
void cxa_posix_usart_close(cxa_posix_usart_t *const usartIn);


#endif // CXA_POSIX_USART_H_
