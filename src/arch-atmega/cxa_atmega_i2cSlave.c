/**
 * @copyright 2018 opencxa.org
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
#include "cxa_atmega_i2cSlave.h"


// ******** includes ********
#include <util/twi.h>
#include <avr/interrupt.h>

#include <cxa_array.h>
#include <cxa_assert.h>


#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAX_NUM_TWI_UNITS			2


// ******** local type definitions ********
typedef struct
{
	cxa_atmega_i2cSlave_twi_t twi;
	cxa_atmega_i2cSlave_t* i2cSlave;
}twi_mapEntry_t;


// ******** local function prototypes ********
static cxa_atmega_i2cSlave_t* getI2cSlaveFromTwi(const cxa_atmega_i2cSlave_twi_t twiIn);


// ********  local variable declarations *********
static bool isInit = false;
static cxa_array_t twiMap;
static twi_mapEntry_t twiMap_raw[MAX_NUM_TWI_UNITS];


// ******** global function implementations ********
void cxa_atmega_i2cSlave_init(cxa_atmega_i2cSlave_t *const i2cIn,
							  const cxa_atmega_i2cSlave_twi_t twiIn,
							  uint8_t address_7bitIn)
{
	cxa_assert(i2cIn);

	// make sure our internal state is initialized
	if( !isInit )
	{
		cxa_array_initStd(&twiMap, twiMap_raw);
		isInit = true;
	}

	// save our references and initialize our object's state
	i2cIn->twiIn = twiIn;
	cxa_fixedByteBuffer_initStd(&i2cIn->buffer, i2cIn->buffer_raw);
	i2cIn->state = CXA_ATM_I2CSLAVE_STATE_IDLE;
	i2cIn->currTxByteIndex = 0;

	// initialize our super class
	cxa_i2cSlave_init(&i2cIn->super);

	// record our object for handling interrupts
	twi_mapEntry_t newEntry = {.twi = twiIn, .i2cSlave = i2cIn};
	cxa_assert(cxa_array_append(&twiMap, (void *const)&newEntry));

	// initialize our hardware
	switch( i2cIn->twiIn )
	{
		case CXA_ATM_I2CSLAVE_TWI0:
			break;

		case CXA_ATM_I2CSLAVE_TWI1:
			// load address into TWI address register
			TWAR1 = address_7bitIn << 1;

			// set the TWCR to enable address matching and enable TWI, clear TWINT, enable TWI interrupt
			TWCR1 = (1 << TWIE) | (1 << TWEA) | (1 << TWEN);
			break;
	}
}


// ******** local function implementations ********
static cxa_atmega_i2cSlave_t* getI2cSlaveFromTwi(const cxa_atmega_i2cSlave_twi_t twiIn)
{
	if( !isInit ) return NULL;

	cxa_array_iterate(&twiMap, currEntry, twi_mapEntry_t)
	{
		if( (currEntry != NULL) && (currEntry->twi == twiIn) ) return currEntry->i2cSlave;
	}

	// if we made it here, we couldn't find a matching entry
	return NULL;
}


ISR(TWI1_vect)
{
	cxa_atmega_i2cSlave_t* i2cIn = getI2cSlaveFromTwi(CXA_ATM_I2CSLAVE_TWI1);
	if( i2cIn != NULL )
	{
		switch( TWSR1 & TW_STATUS_MASK )
		{
			case 0x60:
				// Own SLA+W has been received; ACK has been returned
				i2cIn->state = CXA_ATM_I2CSLAVE_STATE_WAIT_COMMAND_BYTE;
				// TWCR1 < TWINT=1 | TWEA=1  receive data byte with ACK
				// TWCR1 < TWINT=1 | TWEA=0  receive data byte with NACK
				TWCR1 |= (1 << TWINT) | (1 << TWEA);
				break;

			case 0x80:
			{
				// Previously addressed with own SLA+W; data has been received; ACK has been returned
				bool shouldAckNext = false;
				if( i2cIn->state == CXA_ATM_I2CSLAVE_STATE_WAIT_COMMAND_BYTE )
				{
					// just got first command byte...don't know if this will be a read or write...
					// reset our rx buffer just in case
					i2cIn->currCommandByte = TWDR1;
					i2cIn->state = CXA_ATM_I2CSLAVE_STATE_GOT_COMMAND_BYTE;
					cxa_fixedByteBuffer_clear(&i2cIn->buffer);
					shouldAckNext = true;
				}
				else if( (i2cIn->state == CXA_ATM_I2CSLAVE_STATE_GOT_COMMAND_BYTE) )
				{
					// just got first data byte...must be a write...
					i2cIn->state = CXA_ATM_I2CSLAVE_STATE_RX_BYTES;

					// make sure we know about this command and the number of bytes
					i2cIn->expectedNumRxBytes = cxa_i2cSlave_validatePreWrite(&i2cIn->super, i2cIn->currCommandByte);
					if( (i2cIn->expectedNumRxBytes == -1) || (i2cIn->expectedNumRxBytes > CXA_ATM_I2CSLAVE_MAXOP_SIZE_BYTES) )
					{
						// something isn't right...NACK at our earliest opportunity
						i2cIn->state = CXA_ATM_I2CSLAVE_STATE_NACK_REST;
						shouldAckNext = false;
					}
					else
					{
						// everything looks good...store our first byte
						cxa_fixedByteBuffer_append_uint8(&i2cIn->buffer, TWDR1);
						shouldAckNext = true;
					}
				}
				else if( i2cIn->state == CXA_ATM_I2CSLAVE_STATE_RX_BYTES )
				{
					// still in a normal write
					cxa_fixedByteBuffer_append_uint8(&i2cIn->buffer, TWDR1);
					shouldAckNext = true;
				}
				// TWCR1 < TWINT=1 | TWEA=1  receive data byte with ACK
				// TWCR1 < TWINT=1 | TWEA=0  receive data byte with NACK
				TWCR1 = (TWCR1 & ~((1 << TWINT) | (1 << TWEA))) | (1 << TWINT) | (shouldAckNext << TWEA);
				break;
			}

			case 0x88:
				// Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
				TWCR1 |= (1 << TWINT) | (1 << TWEA);
				break;

			case 0xA0:
				// A STOP condition or repeated START condition has been received while still addressed as Slave
				if( i2cIn->state == CXA_ATM_I2CSLAVE_STATE_RX_BYTES )
				{
					// this was a write...notify our higher-level
					cxa_i2cSlave_notifyWrite(&i2cIn->super, i2cIn->currCommandByte, &i2cIn->buffer);
					i2cIn->state = CXA_ATM_I2CSLAVE_STATE_IDLE;
				}
				else if( i2cIn->state == CXA_ATM_I2CSLAVE_STATE_TX_BYTES )
				{
					// this is the end of a read...we're done
					i2cIn->state = CXA_ATM_I2CSLAVE_STATE_IDLE;
				}
				// TWCR1 < TWINT=1 | TWEA=1  Switch to the not addressed Slave mode; own SLA will be recognized;
				TWCR1 |= (1 << TWINT) | (1 << TWEA);
				break;

			case 0xA8:
			{
				// Own SLA+R has been received; ACK has been returned
				bool shouldAckNext = false;

				i2cIn->state = CXA_ATM_I2CSLAVE_STATE_TX_BYTES;
				i2cIn->currTxByteIndex = 0;
				cxa_fixedByteBuffer_clear(&i2cIn->buffer);

				if( cxa_i2cSlave_notifyRead(&i2cIn->super, i2cIn->currCommandByte, &i2cIn->buffer) )
				{
					uint8_t txByte = 0;
					shouldAckNext = cxa_fixedByteBuffer_get_uint8(&i2cIn->buffer, i2cIn->currTxByteIndex++, txByte) &&
									(i2cIn->currTxByteIndex < cxa_fixedByteBuffer_getSize_bytes(&i2cIn->buffer));
					TWDR1 = txByte;
				}

				// TWCR1 < TWINT=1 | TWEA=0  Last Data byte will be transmitted and NOT ACK should be received
				// TWCR1 < TWINT=1 | TWEA=1  Data byte will be transmitted and ACK should be received
				TWCR1 = (TWCR1 & ~((1 << TWINT) | (1 << TWEA))) | (1 << TWINT) | (shouldAckNext << TWEA);
				break;
			}

			case 0xB8:
			{
				// Data byte in TWDRn has been transmitted; ACK has been received
				bool shouldAckNext = false;

				uint8_t txByte = 0;
				shouldAckNext = cxa_fixedByteBuffer_get_uint8(&i2cIn->buffer, i2cIn->currTxByteIndex++, txByte) &&
								(i2cIn->currTxByteIndex < cxa_fixedByteBuffer_getSize_bytes(&i2cIn->buffer));
				TWDR1 = txByte;

				// TWCR1 < TWINT=1 | TWEA=0  Last Data byte will be transmitted and NOT ACK should be received
				// TWCR1 < TWINT=1 | TWEA=1  Data byte will be transmitted and ACK should be received
				TWCR1 = (TWCR1 & ~((1 << TWINT) | (1 << TWEA))) | (1 << TWINT) | (shouldAckNext << TWEA);
				break;
			}

			case 0xC0:
				// Data byte in TWDRn has been transmitted; NOT ACK has been received
				// TWCR1 < TWINT=1 | TWEA=1  Switch to the not addressed Slave mode; own SLA will be recognized;
				TWCR1 |= (1 << TWINT) | (1 << TWEA);
				break;

			case 0xC8:
				// Last data byte in TWDRn has been transmitted (TWEA = “0”); ACK has been received
				// TWCR1 < TWINT=1 | TWEA=1  Switch to the not addressed Slave mode; own SLA will be recognized;
				TWCR1 |= (1 << TWINT) | (1 << TWEA);
				break;

			case 0xF8:		// "no relevant state information"
			default:
				if( TWCR1 & ~(1 << TWINT) ) TWCR1 |= (1 << TWINT);
				break;
		}
	}
}
