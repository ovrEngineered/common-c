/**
 * Original Source:
 * https://github.com/esp8266/Arduino.git
 *
 * Copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include <cxa_esp8266_network_clientFactory.h>

#include <cxa_assert.h>
#include <cxa_array.h>
#include <cxa_esp8266_network_client.h>
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAXNUM_CLIENTS		3


// ******** local type definitions ********
typedef struct
{
	bool isUsed;
	cxa_esp8266_network_client_t client;
}clientEntry_t;


// ******** local function prototypes ********


// ********  local variable declarations *********
static bool isInit = false;
static cxa_timeBase_t* timeBase;
static cxa_array_t clients;
static clientEntry_t clients_raw[MAXNUM_CLIENTS];


// ******** global function implementations ********
void cxa_esp8266_network_clientFactory_init(cxa_timeBase_t *const timeBaseIn)
{
	cxa_assert(timeBaseIn);

	timeBase = timeBaseIn;
	cxa_array_init_inPlace(&clients, sizeof(*clients_raw), (sizeof(clients_raw)/sizeof(*clients_raw)), clients_raw, sizeof(clients_raw));

	// setup each of our clients
	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( currEntry == NULL ) continue;

		currEntry->isUsed = false;
		cxa_esp8266_network_client_init(&currEntry->client, timeBase);
	}
	isInit = true;
}


cxa_esp8266_network_client_t* cxa_esp8266_network_clientFactory_getClientByEspConn(struct espconn *const connIn)
{
	cxa_assert(isInit);

	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( &currEntry->client.espconn == connIn ) return &currEntry->client;
	}

	// if we made it here, we have an unknown connection
	return NULL;
}


void cxa_esp8266_network_clientFactory_update(void)
{
	cxa_assert(isInit);

	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->isUsed ) cxa_esp8266_network_client_update(&currEntry->client);
	}
}


cxa_network_client_t* cxa_network_clientFactory_reserveClient(void)
{
	cxa_assert(isInit);

	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( !currEntry->isUsed )
		{
			currEntry->isUsed = true;
			return &currEntry->client.super;
		}
	}

	// if we made it here, we don't have any free clients
	return NULL;
}


void cxa_network_clientFactory_freeClient(cxa_network_client_t *const clientIn)
{
	cxa_assert(isInit);

	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( &currEntry->client == (cxa_esp8266_network_client_t*)clientIn )
		{
			currEntry->isUsed = false;
			return;
		}
	}
}


// ******** local function implementations ********
