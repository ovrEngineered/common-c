/**
 * @file
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
#ifndef CXA_BLUEGIGA_BTLE_CLIENT_H_
#define CXA_BLUEGIGA_BTLE_CLIENT_H_


// ******** includes ********
#include <cxa_blueGiga_types.h>
#include <cxa_btle_client.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_gpio.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_protocolParser_bgapi.h>
#include <cxa_softWatchDog.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********
#define CXA_BLUEGIGA_BTLE_MAX_PACKET_SIZE		64


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_blueGiga_btle_client cxa_blueGiga_btle_client_t;


/**
 * @private
 */
typedef void (*cxa_blueGiga_btle_client_cb_onResponse_t)(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn);


/**
 * @private
 */
struct cxa_blueGiga_btle_client
{
	cxa_btle_client_t super;

	cxa_gpio_t *gpio_reset;
	cxa_protocolParser_bgapi_t protoParse;

	cxa_fixedByteBuffer_t fbb_rx;
	uint8_t fbb_rx_raw[CXA_BLUEGIGA_BTLE_MAX_PACKET_SIZE];

	cxa_fixedByteBuffer_t fbb_tx;
	uint8_t fbb_tx_raw[CXA_BLUEGIGA_BTLE_MAX_PACKET_SIZE];

	struct
	{
		cxa_softWatchDog_t watchdog;

		cxa_blueGiga_classId_t classId;
		cxa_blueGiga_methodId_t methodId;

		cxa_blueGiga_btle_client_cb_onResponse_t cb_onResponse;
	}inFlightRequest;

	bool isActiveScan;
	cxa_stateMachine_t stateMachine;

	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_blueGiga_btle_client_init(cxa_blueGiga_btle_client_t *const btlecIn, cxa_ioStream_t *const iosIn, cxa_gpio_t *const gpio_resetIn);

#endif
