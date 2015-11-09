/**
 * @file
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
#ifndef CXA_ESP8266_NETWORK_CLIENT_H_
#define CXA_ESP8266_NETWORK_CLIENT_H_


// ******** includes ********
#include <stdio.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>
#include <cxa_stateMachine.h>

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include "../arch-common/cxa_network_client.h"


// ******** global macro definitions ********
#ifndef CXA_ESP8266_NETWORK_CLIENT_MAXHOSTNAMELEN_BYTES
	#define CXA_ESP8266_NETWORK_CLIENT_MAXHOSTNAMELEN_BYTES			64
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_esp8266_network_client_t object
 */
typedef struct cxa_esp8266_network_client cxa_esp8266_network_client_t;


/**
 * @private
 */
struct cxa_esp8266_network_client
{
	cxa_network_client_t super;

	char targetHostName[CXA_ESP8266_NETWORK_CLIENT_MAXHOSTNAMELEN_BYTES+1];

	ip_addr_t ip;
	struct espconn espconn;

	cxa_stateMachine_t stateMachine;
};


// ******** global function prototypes ********
/**
 * @private
 */
void cxa_esp8266_network_client_init(cxa_esp8266_network_client_t *const netClientIn, cxa_timeBase_t* const timeBaseIn);


/**
 * @private
 */
void cxa_esp8266_network_client_update(cxa_esp8266_network_client_t *const netClientIn);


#endif // CXA_ESP8266_NETWORKCLIENT_H_
