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
#include <cxa_esp8266_network_factory.h>

#include <cxa_assert.h>
#include <cxa_array.h>
#include <cxa_esp8266_network_tcpClient.h>
#include <cxa_esp8266_network_tcpServer.h>
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAXNUM_CLIENTS		2
#define MAXNUM_SERVERS		2


// ******** local type definitions ********
typedef struct
{
	bool isUsed;
	cxa_esp8266_network_tcpClient_t client;
}clientEntry_t;


typedef struct
{
	bool isUsed;
	cxa_esp8266_network_tcpServer_t server;
}serverEntry_t;


// ******** local function prototypes ********


// ********  local variable declarations *********
static bool isInit = false;
static cxa_array_t clients;
static clientEntry_t clients_raw[MAXNUM_CLIENTS];
static cxa_array_t servers;
static serverEntry_t servers_raw[MAXNUM_SERVERS];


// ******** global function implementations ********
void cxa_esp8266_network_factory_init(void)
{
	cxa_array_init_inPlace(&clients, sizeof(*clients_raw), (sizeof(clients_raw)/sizeof(*clients_raw)), clients_raw, sizeof(clients_raw));
	cxa_array_init_inPlace(&servers, sizeof(*servers_raw), (sizeof(servers_raw)/sizeof(*servers_raw)), servers_raw, sizeof(servers_raw));

	// mark each of our clients as unused
	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( currEntry == NULL ) continue;

		currEntry->isUsed = false;
	}
	// mark each of our servers as unused
	cxa_array_iterate(&servers, currEntry, serverEntry_t)
	{
		if( currEntry == NULL ) continue;

		currEntry->isUsed = false;
	}

	isInit = true;
}


cxa_esp8266_network_tcpClient_t* cxa_esp8266_network_factory_getTcpClientByEspConn(struct espconn *const connIn)
{
	cxa_assert(isInit);

	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( &currEntry->client.espconn == connIn ) return &currEntry->client;
	}

	// if we made it here, we have an unknown connection
	return NULL;
}


cxa_esp8266_network_tcpServer_t* cxa_esp8266_network_factory_getTcpServerByListeningPortNum(int portNumIn)
{
	cxa_assert(isInit);

	cxa_array_iterate(&servers, currEntry, serverEntry_t)
	{
		if( currEntry->server.espconn_listen.proto.tcp->local_port == portNumIn ) return &currEntry->server;
	}

	// if we made it here, we have an unknown connection
	return NULL;
}


void cxa_esp8266_network_factory_update(void)
{
	cxa_assert(isInit);

	// update our clients
	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->isUsed ) cxa_esp8266_network_tcpClient_update(&currEntry->client);
	}

	// update our servers
	cxa_array_iterate(&servers, currEntry, serverEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->isUsed ) cxa_esp8266_network_tcpServer_update(&currEntry->server);
	}
}


cxa_network_tcpClient_t* cxa_network_factory_reserveTcpClient(void)
{
	cxa_assert(isInit);

	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( !currEntry->isUsed )
		{
			currEntry->isUsed = true;
			// initialize our client
			cxa_esp8266_network_tcpClient_init(&currEntry->client);
			return &currEntry->client.super;
		}
	}

	// if we made it here, we don't have any free clients
	return NULL;
}


void cxa_network_factory_freeTcpClient(cxa_network_tcpClient_t *const clientIn)
{
	cxa_assert(isInit);

	cxa_array_iterate(&clients, currEntry, clientEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( &currEntry->client == (cxa_esp8266_network_tcpClient_t*)clientIn )
		{
			currEntry->isUsed = false;
			return;
		}
	}
}


cxa_network_tcpServer_t* cxa_network_factory_reserveTcpServer(void)
{
	cxa_assert(isInit);

	cxa_array_iterate(&servers, currEntry, serverEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( !currEntry->isUsed )
		{
			currEntry->isUsed = true;
			// initialize our client
			cxa_esp8266_network_tcpServer_init(&currEntry->server);
			return &currEntry->server.super;
		}
	}

	// if we made it here, we don't have any free servers
	return NULL;
}


void cxa_network_factory_freeTcpServer(cxa_network_tcpServer_t *const serverIn)
{
	cxa_assert(isInit);

	cxa_array_iterate(&servers, currEntry, serverEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( &currEntry->server == (cxa_esp8266_network_tcpServer_t*)serverIn )
		{
			currEntry->isUsed = false;
			return;
		}
	}
}


// ******** local function implementations ********
