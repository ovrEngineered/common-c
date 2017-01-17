/**
 * Copyright 2013 opencxa.org
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
 */
#ifndef CXA_STATE_MACHINE_H_
#define CXA_STATE_MACHINE_H_


/**
 * @file <description>
 *
 * Configuration Options:
 *		CXA_STATE_MACHINE_MAXNUM_STATES
 *		CXA_STATE_MACHINE_ENABLE_LOGGING
 *		CXA_STATE_MACHINE_ENABLE_TIMED_STATES
 *
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdint.h>
#include <cxa_array.h>

#include <cxa_config.h>
#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
	#include <cxa_logger_header.h>
#endif
#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
	#include <cxa_timeDiff.h>
#endif


// ******** global macro definitions ********
#ifndef CXA_STATE_MACHINE_MAXNUM_STATES
	#define CXA_STATE_MACHINE_MAXNUM_STATES				16
#endif

#define CXA_STATE_MACHINE_STATE_UNKNOWN						-1


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_stateMachine cxa_stateMachine_t;


/**
 * @public
 */
typedef void (*cxa_stateMachine_cb_enter_t)(cxa_stateMachine_t *const smIn, int prevStateIdIn, void* userVarIn);
typedef void (*cxa_stateMachine_cb_state_t)(cxa_stateMachine_t *const smIn, void *userVarIn);
typedef void (*cxa_stateMachine_cb_leave_t)(cxa_stateMachine_t *const smIn, int nextStateIdIn, void* userVarIn);


/**
 * @private
 */
typedef enum
{
	CXA_STATE_MACHINE_STATE_TYPE_NORMAL,
	#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
		CXA_STATE_MACHINE_STATE_TYPE_TIMED
	#endif
}cxa_stateMachine_stateType_t;


/**
 * @private
 */
typedef struct  
{
	cxa_stateMachine_stateType_t type;
	
	int stateId;
	const char* stateName;
	
	cxa_stateMachine_cb_enter_t cb_enter;
	cxa_stateMachine_cb_state_t cb_state;
	cxa_stateMachine_cb_leave_t cb_leave;
	void *userVar;
	
	#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
		int nextStateId;
		uint32_t stateTime_ms;
	#endif
}cxa_stateMachine_state_t;


/**
 * @public
 */
struct cxa_stateMachine
{
	cxa_stateMachine_state_t* currState;
	cxa_stateMachine_state_t* nextState;
	
	cxa_array_t states;
	cxa_stateMachine_state_t states_raw[CXA_STATE_MACHINE_MAXNUM_STATES];
	
	#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
		cxa_logger_t logger;
	#endif
	
	#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
		bool timedStatesEnabled;
		cxa_timeDiff_t td_timedTransition;
	#endif
};


// ******** global function prototypes ********
void cxa_stateMachine_init(cxa_stateMachine_t *const smIn, const char* nameIn);

void cxa_stateMachine_addState(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn,
	cxa_stateMachine_cb_enter_t cb_enterIn, cxa_stateMachine_cb_state_t cb_stateIn, cxa_stateMachine_cb_leave_t cb_leaveIn,
	void *userVarIn);
	
#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
void cxa_stateMachine_addState_timed(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn, int nextStateIdIn, uint32_t stateTime_msIn,
	cxa_stateMachine_cb_enter_t cb_enterIn, cxa_stateMachine_cb_state_t cb_stateIn, cxa_stateMachine_cb_leave_t cb_leaveIn, void *userVarIn);
#endif

void cxa_stateMachine_setInitialState(cxa_stateMachine_t *const smIn, int stateIdIn);
	
void cxa_stateMachine_transition(cxa_stateMachine_t *const smIn, int stateIdIn);
void cxa_stateMachine_transitionNow(cxa_stateMachine_t *const smIn, int stateIdIn);

int cxa_stateMachine_getCurrentState(cxa_stateMachine_t *const smIn);


#endif // CXA_STATE_MACHINE_H_
