/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_stateMachine.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>
#include <cxa_timeBase.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_onRunLoopUpdate(void* userVarIn);

static cxa_stateMachine_state_t* getState_byId(cxa_stateMachine_t *const smIn, int idIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_stateMachine_init(cxa_stateMachine_t *const smIn, const char* nameIn, int threadIdIn)
{
	cxa_assert(smIn);
	cxa_assert(nameIn);

	// set some sensible defaults
	smIn->currState = NULL;
	smIn->nextState = NULL;
	smIn->hasStarted = false;

	// setup our internal state
	cxa_array_init(&smIn->states, sizeof(*smIn->states_raw), (void*)smIn->states_raw, sizeof(smIn->states_raw));

	// setup our logger if it's enabled
	#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
	cxa_logger_init_formattedString(&smIn->logger, "fsm::%s", nameIn);
	#endif

	// a timediff was _not_ supplied so we cannot do timed states
	// even if they are enabled
	#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
	cxa_timeDiff_init(&smIn->td_timedTransition);
	smIn->timedStatesEnabled = true;
	#endif

	// register for run loop execution
	cxa_runLoop_addEntry(threadIdIn, NULL, cb_onRunLoopUpdate, (void*)smIn);
}


void cxa_stateMachine_addState(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn,
	cxa_stateMachine_cb_enter_t cb_enterIn, cxa_stateMachine_cb_state_t cb_stateIn, cxa_stateMachine_cb_leave_t cb_leaveIn,
	void *userVarIn)
{
	cxa_assert(smIn);
	cxa_assert(nameIn);
	cxa_assert(!smIn->hasStarted);
	cxa_assert(idIn != CXA_STATE_MACHINE_STATE_UNKNOWN);

	// make sure we don't already have this state added
	cxa_assert_msg((getState_byId(smIn, idIn) == NULL), "duplicate state");

	// create our new state
	cxa_stateMachine_state_t newState = {.type=CXA_STATE_MACHINE_STATE_TYPE_NORMAL, .stateId=idIn, .stateName=nameIn,
		.cb_enter=cb_enterIn, .cb_state=cb_stateIn, .cb_leave=cb_leaveIn, .userVar=userVarIn};

	// add the new state to our array of states
	cxa_assert_msg(cxa_array_append(&smIn->states, &newState), "increase 'CXA_STATE_MACHINE_MAXNUM_STATES'");
}


#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
void cxa_stateMachine_addState_timed(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn, int nextStateIdIn, uint32_t stateTime_msIn,
	cxa_stateMachine_cb_enter_t cb_enterIn, cxa_stateMachine_cb_state_t cb_stateIn, cxa_stateMachine_cb_leave_t cb_leaveIn, void *userVarIn)
{
	cxa_assert(smIn);
	cxa_assert(nameIn);
	cxa_assert(!smIn->hasStarted);
	cxa_assert(smIn->timedStatesEnabled);
	cxa_assert(idIn != CXA_STATE_MACHINE_STATE_UNKNOWN);

	// make sure we don't already have this state added
	cxa_assert(getState_byId(smIn, idIn) == NULL);

	// create our new state
	cxa_stateMachine_state_t newState = {.type=CXA_STATE_MACHINE_STATE_TYPE_TIMED, .stateId=idIn, .stateName=nameIn,
		.nextStateId=nextStateIdIn, .stateTime_ms=stateTime_msIn,
		.cb_enter=cb_enterIn, .cb_state=cb_stateIn, .cb_leave=cb_leaveIn, .userVar=userVarIn};

	// add the new state to our array of states
	cxa_assert(cxa_array_append(&smIn->states, &newState));
}
#endif


void cxa_stateMachine_setInitialState(cxa_stateMachine_t *const smIn, int stateIdIn)
{
	cxa_assert(smIn);
	cxa_assert(!smIn->hasStarted);

	// get our next state
	cxa_stateMachine_state_t *newNextState = getState_byId(smIn, stateIdIn);
	cxa_assert(newNextState != NULL);

	// we have a valid new state...mark for transition
	smIn->nextState = newNextState;
}


void cxa_stateMachine_transition(cxa_stateMachine_t *const smIn, int stateIdIn)
{
	cxa_assert(smIn);
#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
	if( !smIn->hasStarted )
	{
		cxa_logger_error(&smIn->logger, "trying to transition to stateId '%d' before runloop has started", stateIdIn);
		cxa_assert(false);
	}
#else
	cxa_assert_msg(smIn->hasStarted, "attempt to transition before runLoop has started");
#endif

	// get our next state
	cxa_stateMachine_state_t *newNextState = getState_byId(smIn, stateIdIn);
	cxa_assert(newNextState != NULL);

	// we have a valid new state...mark for transition
	smIn->nextState = newNextState;
}


void cxa_stateMachine_transitionNow(cxa_stateMachine_t *const smIn, int stateIdIn)
{
	cxa_assert(smIn);
	cxa_assert(smIn->hasStarted);

	cxa_stateMachine_transition(smIn, stateIdIn);
	cb_onRunLoopUpdate((void*)smIn);
}


int cxa_stateMachine_getCurrentState(cxa_stateMachine_t *const smIn)
{
	cxa_assert(smIn);

	return (smIn->currState != NULL) ? smIn->currState->stateId : CXA_STATE_MACHINE_STATE_UNKNOWN;
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_stateMachine_t* smIn = (cxa_stateMachine_t*)userVarIn;
	cxa_assert(smIn);

	// make sure we've been marked as started
	if( !smIn->hasStarted ) smIn->hasStarted = true;

	// see if we should transition
	if( smIn->nextState != NULL )
	{
		// call the leave function of our old state
		int prevStateId = CXA_STATE_MACHINE_STATE_UNKNOWN;
		if( smIn->currState != NULL )
		{
			prevStateId = smIn->currState->stateId;
			if( smIn->currState->cb_leave != NULL ) smIn->currState->cb_leave(smIn, smIn->nextState->stateId, smIn->currState->userVar);
		}

		// actually do our transition
		smIn->currState = smIn->nextState;
		smIn->nextState = NULL;

		#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
			cxa_logger_info(&smIn->logger, "new state: '%s'", smIn->currState->stateName);
		#endif

		// call the enter function of our new state
		if( smIn->currState->cb_enter != NULL ) smIn->currState->cb_enter(smIn, prevStateId, smIn->currState->userVar);

		#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
			if( smIn->timedStatesEnabled && (smIn->currState->type == CXA_STATE_MACHINE_STATE_TYPE_TIMED) ) cxa_timeDiff_setStartTime_now(&smIn->td_timedTransition);
		#endif
	}
	else
	{
		#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
			// see if our state's time has expired...if so, transition into our next state
			if(  smIn->timedStatesEnabled &&
				(smIn->currState->type == CXA_STATE_MACHINE_STATE_TYPE_TIMED) &&
				 cxa_timeDiff_isElapsed_ms(&smIn->td_timedTransition, smIn->currState->stateTime_ms) )
			{
				cxa_stateMachine_transition(smIn, smIn->currState->nextStateId);
				return;
			}
		#endif

		// keep updating our state
		if( (smIn->currState != NULL) && (smIn->currState->cb_state != NULL) ) smIn->currState->cb_state(smIn, smIn->currState->userVar);
	}
}


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
