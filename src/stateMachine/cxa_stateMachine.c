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
#include <cxa_config.h>
#include "cxa_stateMachine.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_stateMachine_state_t* getState_byId(cxa_stateMachine_t *const smIn, int idIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_stateMachine_init(cxa_stateMachine_t *const smIn, const char* nameIn)
{
	cxa_assert(smIn);
	cxa_assert(nameIn);
	
	// set some sensible default
	smIn->currState = NULL;
	smIn->nextState = NULL;
	
	// setup our internal state
	cxa_array_init(&smIn->states, sizeof(*smIn->states_raw), (void*)smIn->states_raw, sizeof(smIn->states_raw));
	
	// setup our logger if it's enabled
	#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
	cxa_logger_vinit(&smIn->logger, "fsm::%s", nameIn);
	#endif
	
	// a timediff was _not_ supplied so we cannot do timed states
	// even if they are enabled
	#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
	smIn->timedStatesEnabled = false;
	#endif
}


#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
void cxa_stateMachine_init_timedStates(cxa_stateMachine_t *const smIn, const char* nameIn, cxa_timeBase_t *const timeBaseIn)
{
	cxa_assert(smIn);
	
	// set some sensible default
	smIn->currState = NULL;
	smIn->nextState = NULL;
	
	// setup our internal state
	cxa_array_init(&smIn->states, sizeof(*smIn->states_raw), (void*)smIn->states_raw, sizeof(smIn->states_raw));
	
	// setup our logger if it's enabled
	#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
	cxa_logger_vinit(&smIn->logger, "fsm::%s", nameIn);
	#endif
	
	// setup our timediff for timed states
	cxa_timeDiff_init(&smIn->td_timedTransition, timeBaseIn);
	smIn->timedStatesEnabled = true;
}
#endif


void cxa_stateMachine_addState(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn,
	cxa_stateMachine_cb_t cb_enterIn, cxa_stateMachine_cb_t cb_stateIn, cxa_stateMachine_cb_t cb_leaveIn,
	void *userVarIn)
{
	cxa_assert(smIn);
	cxa_assert(nameIn);

	// make sure we don't already have this state added
	cxa_assert(getState_byId(smIn, idIn) == NULL);

	// create our new state
	cxa_stateMachine_state_t newState = {.type=CXA_STATE_MACHINE_STATE_TYPE_NORMAL, .stateId=idIn, .stateName=nameIn,
		.cb_enter=cb_enterIn, .cb_state=cb_stateIn, .cb_leave=cb_leaveIn, .userVar=userVarIn};

	// add the new state to our array of states
	cxa_assert(cxa_array_append(&smIn->states, &newState));
	
	// if we're currently not in a known state, enter this state (when update is called)
	if( smIn->currState == NULL ) cxa_stateMachine_transition(smIn, idIn);
}


#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
void cxa_stateMachine_addState_timed(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn, int nextStateIdIn, uint32_t stateTime_msIn,
	cxa_stateMachine_cb_t cb_enterIn, cxa_stateMachine_cb_t cb_stateIn, cxa_stateMachine_cb_t cb_leaveIn, void *userVarIn)
{
	cxa_assert(smIn);
	cxa_assert(nameIn);
	cxa_assert(smIn->timedStatesEnabled);

	// make sure we don't already have this state added
	cxa_assert(getState_byId(smIn, idIn) == NULL);

	// create our new state
	cxa_stateMachine_state_t newState = {.type=CXA_STATE_MACHINE_STATE_TYPE_TIMED, .stateId=idIn, .stateName=nameIn,
		.nextStateId=nextStateIdIn, .stateTime_ms=stateTime_msIn,
		.cb_enter=cb_enterIn, .cb_state=cb_stateIn, .cb_leave=cb_leaveIn, .userVar=userVarIn};

	// add the new state to our array of states
	cxa_assert(cxa_array_append(&smIn->states, &newState));

	// if we're currently not in a known state, enter this state (when update is called)
	if( smIn->currState == NULL ) cxa_stateMachine_transition(smIn, idIn);	
}
#endif


void cxa_stateMachine_transition(cxa_stateMachine_t *const smIn, int stateIdIn)
{
	cxa_assert(smIn);
	
	// get our next state
	cxa_stateMachine_state_t *newNextState = getState_byId(smIn, stateIdIn);
	cxa_assert(newNextState != NULL);
	
	// we have a valid new state...mark for transition
	smIn->nextState = newNextState;
}


int cxa_stateMachine_getCurrentState(cxa_stateMachine_t *const smIn)
{
	cxa_assert(smIn);
	
	return (smIn->currState != NULL) ? smIn->currState->stateId : -1;
}


void cxa_stateMachine_update(cxa_stateMachine_t *const smIn)
{
	cxa_assert(smIn);
	
	// see if we should transition
	if( smIn->nextState != NULL )
	{
		// call the leave function of our old state
		if((smIn->currState != NULL) && (smIn->currState->cb_leave != NULL)) smIn->currState->cb_leave(smIn, smIn->currState->userVar);
				
		// actually do our transition
		smIn->currState = smIn->nextState;
		smIn->nextState = NULL;
		
		#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
			cxa_logger_info(&smIn->logger, "new state: '%s'", smIn->currState->stateName);
		#endif
				
		// call the enter function of our new state
		if( smIn->currState->cb_enter != NULL ) smIn->currState->cb_enter(smIn, smIn->currState->userVar);
		
		#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
			if( smIn->timedStatesEnabled && (smIn->currState->type == CXA_STATE_MACHINE_STATE_TYPE_TIMED) ) cxa_timeDiff_setStartTime_now(&smIn->td_timedTransition);
		#endif
	}
	#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
	else if( smIn->timedStatesEnabled && (smIn->currState->type == CXA_STATE_MACHINE_STATE_TYPE_TIMED) )
	{
		// see if our state's time has expired...if so, transition into our next state
		if( cxa_timeDiff_isElapsed_ms(&smIn->td_timedTransition, smIn->currState->stateTime_ms) ) cxa_stateMachine_transition(smIn, smIn->currState->nextStateId);
	}
	#endif
	else
	{
		// we have some sort of state, and we're gonna stay there
		if( (smIn->currState != NULL) && (smIn->currState->cb_state != NULL) ) smIn->currState->cb_state(smIn, smIn->currState->userVar);
	}
}


// ******** local function implementations ********
static cxa_stateMachine_state_t* getState_byId(cxa_stateMachine_t *const smIn, int idIn)
{
	cxa_assert(smIn);
	
	for( size_t i = 0; i < cxa_array_getSize_elems(&smIn->states); i++ )
	{
		cxa_stateMachine_state_t* currState = (cxa_stateMachine_state_t*)cxa_array_get(&smIn->states, i);
		if( currState == NULL ) continue;
		
		if( currState->stateId == idIn ) return currState;
	}
	
	return NULL;
}
