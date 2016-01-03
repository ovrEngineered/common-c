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
#ifndef CXA_ESP8266_NETWORK_FACTORY_H_
#define CXA_ESP8266_NETWORK_FACTORY_H_


// ******** includes ********
#include <cxa_network_factory.h>
#include <cxa_esp8266_network_tcpClient.h>
#include <cxa_esp8266_network_tcpServer.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
void cxa_esp8266_network_factory_init(void);
cxa_esp8266_network_tcpClient_t* cxa_esp8266_network_factory_getTcpClientByEspConn(struct espconn *const connIn);
cxa_esp8266_network_tcpServer_t* cxa_esp8266_network_factory_getTcpServerByListeningPortNum(int portNumIn);
void cxa_esp8266_network_factory_update(void);


#endif // CXA_ESP8266_NETWORK_FACTORY_H_
