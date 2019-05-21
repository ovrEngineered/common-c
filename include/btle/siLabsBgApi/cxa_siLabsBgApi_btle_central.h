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
#ifndef CXA_SILABSBGAPI_BTLE_CENTRAL_H_
#define CXA_SILABSBGAPI_BTLE_CENTRAL_H_


// ******** includes ********
#include <gecko_bglib.h>

#include <cxa_btle_central.h>
#include <cxa_btle_uuid.h>
#include <cxa_eui48.h>
#include <cxa_ioStream_peekable.h>
#include <cxa_siLabsBgApi_btle_connection.h>


// ******** global macro definitions ********
#ifndef CXA_SILABSBGAPI_BTLE_CENTRAL_MAXNUM_CONNS
	#define CXA_SILABSBGAPI_BTLE_CENTRAL_MAXNUM_CONNS						2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_siLabsBgApi_btle_central cxa_siLabsBgApi_btle_central_t;


/**
 * @private
 */
struct cxa_siLabsBgApi_btle_central
{
	cxa_btle_central_t super;

	int threadId;

	cxa_siLabsBgApi_btle_connection_t conns[CXA_SILABSBGAPI_BTLE_CENTRAL_MAXNUM_CONNS];
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_siLabsBgApi_btle_central_init(cxa_siLabsBgApi_btle_central_t *const btlecIn, int threadIdIn);

/**
 * @protected
 *
 * @return true if this event was handled
 */
bool cxa_siLabsBgApi_btle_central_handleBgEvent(cxa_siLabsBgApi_btle_central_t *const btlecIn, struct gecko_cmd_packet *evt);

#endif
