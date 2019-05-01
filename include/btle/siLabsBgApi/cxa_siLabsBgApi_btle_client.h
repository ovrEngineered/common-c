/**
 * @file
 * @copyright 2019 opencxa.org
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
#ifndef CXA_SILABSBGAPI_BTLE_CLIENT_H_
#define CXA_SILABSBGAPI_BTLE_CLIENT_H_


// ******** includes ********
#include <cxa_btle_client.h>
#include <cxa_btle_uuid.h>
#include <cxa_eui48.h>
#include <cxa_ioStream_peekable.h>
#include <cxa_logger_header.h>
#include <cxa_siLabsBgApi_btle_connection.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********
#ifndef CXA_SILABSBGAPI_BTLE_CLIENT_MAXNUM_CONNS
	#define CXA_SILABSBGAPI_BTLE_CLIENT_MAXNUM_CONNS						2
#endif




// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_siLabsBgApi_btle_client cxa_siLabsBgApi_btle_client_t;


/**
 * @private
 */
struct cxa_siLabsBgApi_btle_client
{
	cxa_btle_client_t super;

	cxa_ioStream_peekable_t ios_usart;
	bool hasBootFailed;

	cxa_siLabsBgApi_btle_connection_t conns[CXA_SILABSBGAPI_BTLE_CLIENT_MAXNUM_CONNS];

	cxa_stateMachine_t stateMachine;
	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_siLabsBgApi_btle_client_init(cxa_siLabsBgApi_btle_client_t *const btlecIn, cxa_ioStream_t *const ioStreamIn, int threadIdIn);


#endif
