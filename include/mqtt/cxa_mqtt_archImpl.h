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
#ifndef CXA_MQTT_ARCHIMPL_H_
#define CXA_MQTT_ARCHIMPL_H_


// ******** includes ********
#include <cxa_timeBase.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct Timer Timer;

struct Timer {
	cxa_timeDiff_t timeDiff;
	uint32_t timeout_ms;
};


typedef struct Network Network;

struct Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};


// ******** global function prototypes ********
void cxa_mqtt_archImpl_init(cxa_timeBase_t *const timeBaseIn);

void InitTimer(Timer*);
char expired(Timer*);
void countdown_ms(Timer*, unsigned int);
void countdown(Timer*, unsigned int);
int left_ms(Timer*);

void NewNetwork(Network*);
int ConnectNetwork(Network*, char*, int);
void cxa_disconnect(Network*);
int cxa_read(Network*, unsigned char*, int, int);
int cxa_write(Network*, unsigned char*, int, int);




#endif // CXA_MQTT_ARCHQIMPL_H_
