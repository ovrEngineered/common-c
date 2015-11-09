/**
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
#ifndef CXA_ESP8266_NETWORK_CLIENTFACTORY_H_
#define CXA_ESP8266_NETWORK_CLIENTFACTORY_H_


// ******** includes ********
#include <cxa_timeBase.h>
#include <cxa_network_clientFactory.h>
#include <cxa_esp8266_network_client.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
void cxa_esp8266_network_clientFactory_init(cxa_timeBase_t *const timeBaseIn);
cxa_esp8266_network_client_t* cxa_esp8266_network_clientFactory_getClientByEspConn(struct espconn *const connIn);
void cxa_esp8266_network_clientFactory_update(void);


#endif // CXA_ESP8266_NETWORK_CLIENTFACTORY_H_
