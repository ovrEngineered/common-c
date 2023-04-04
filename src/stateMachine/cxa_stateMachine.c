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

	#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
	cxa_timeDiff_init(&smIn->td_timedTransition);
	#endif

	#ifdef CXA_STATE_MACHINE_ENABLE_LISTENERS
	cxa_array_initStd(&smIn->listeners, smIn->listeners_raw);
	#endif

	// register for run loop execution
	cxa_runLoop_addEntry(threadIdIn, cb_onRunLoopUpdate, cb_onRunLoopUpdate, (void*)smIn);
}


void cxa_stateMachine_addState(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn,
	cxa_stateMachine_cb_entered_t cb_enteredIn, cxa_stateMachine_cb_state_t cb_stateIn, cxa_stateMachine_cb_leaving_t cb_leavingIn,
	void *userVarIn)
{
	cxa_stateMachine_addState_full(smIn, idIn, nameIn, NULL, cb_enteredIn, cb_stateIn, cb_leavingIn, NULL, userVarIn);
}


void cxa_stateMachine_addState_full(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn,
		cxa_stateMachine_cb_entering_t cb_enteringIn, cxa_stateMachine_cb_entered_t cb_enteredIn,
		cxa_stateMachine_cb_state_t cb_stateIn,
		cxa_stateMachine_cb_leaving_t cb_leavingIn, cxa_stateMachine_cb_left_t cb_leftIn,
		void *userVarIn)
{
	cxa_assert(smIn);
	cxa_assert(nameIn);
	cxa_assert(!smIn->hasStarted);
	cxa_assert(idIn != CXA_STATE_MACHINE_STATE_UNKNOWN);

	// make sure we don't already have this state added
	cxa_assert_msg((getState_byId(smIn, idIn) == NULL), "duplicate state");

	// create our new state
	cxa_stateMachine_state_t newState = {
											.type=CXA_STATE_MACHINE_STATE_TYPE_NORMAL,
											.stateId=idIn,
											.stateName=nameIn,
											.cb_entering=cb_enteringIn,
											.cb_entered=cb_enteredIn,
											.cb_state=cb_stateIn,
											.cb_leaving=cb_leavingIn,
											.cb_left=cb_leftIn,
											.userVar=userVarIn
										};

	// add the new state to our array of states
	cxa_assert_msg(cxa_array_append(&smIn->states, &newState), "increase 'CXA_STATE_MACHINE_MAXNUM_STATES'");
}


#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
void cxa_stateMachine_addState_timed(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn, int nextStateIdIn, uint32_t stateTime_msIn,
		cxa_stateMachine_cb_entered_t cb_enteredIn, cxa_stateMachine_cb_state_t cb_stateIn, cxa_stateMachine_cb_leaving_t cb_leavingIn, void *userVarIn)
{
	cxa_stateMachine_addState_timed_full(smIn, idIn, nameIn, nextStateIdIn, stateTime_msIn, NULL, cb_enteredIn, cb_stateIn, cb_leavingIn, NULL, userVarIn);
}


void cxa_stateMachine_addState_timed_full(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn, int nextStateIdIn, uint32_t stateTime_msIn,
		cxa_stateMachine_cb_entering_t cb_enteringIn, cxa_stateMachine_cb_entered_t cb_enteredIn,
		cxa_stateMachine_cb_state_t cb_stateIn,
		cxa_stateMachine_cb_leaving_t cb_leavingIn, cxa_stateMachine_cb_left_t cb_leftIn,
		void *userVarIn)
{
	cxa_assert(smIn);
	cxa_assert(nameIn);
	cxa_assert(!smIn->hasStarted);
	cxa_assert(idIn != CXA_STATE_MACHINE_STATE_UNKNOWN);

	// make sure we don't already have this state added
	cxa_assert(getState_byId(smIn, idIn) == NULL);

	// create our new state
	cxa_stateMachine_state_t newState = {
											.type=CXA_STATE_MACHINE_STATE_TYPE_TIMED,
											.stateId=idIn,
											.stateName=nameIn,
											.nextStateId=nextStateIdIn,
											.stateTime_ms=stateTime_msIn,
											.cb_entering=cb_enteringIn,
											.cb_entered=cb_enteredIn,
											.cb_state=cb_stateIn,
											.cb_leaving=cb_leavingIn,
											.cb_left=cb_leftIn,
											.userVar=userVarIn
										};

	// add the new state to our array of states
	cxa_assert(cxa_array_append(&smIn->states, &newState));
}
#endif


#ifdef CXA_STATE_MACHINE_ENABLE_LISTENERS
void cxa_stateMachine_addListener(cxa_stateMachine_t *const smIn,
								  cxa_stateMachine_listenerCb_beforeExecution_t cb_beforeExecutionIn,
								  cxa_stateMachine_listenerCb_onTransition_t cb_onTransitionIn,
								  cxa_stateMachine_listenerCb_afterExecution_t cb_afterExecutionIn,
								  void *userVarIn)
{
	cxa_stateMachine_listenerEntry_t newEntry = {
			.cb_beforeExecution = cb_beforeExecutionIn,
			.cb_onTransition = cb_onTransitionIn,
			.cb_afterExecution = cb_afterExecutionIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&smIn->listeners, &newEntry));
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


const char* cxa_stateMachine_getCurrentState_name(cxa_stateMachine_t *const smIn)
{
	cxa_assert(smIn);

	return (smIn->currState != NULL) ? smIn->currState->stateName : NULL;
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_stateMachine_t* smIn = (cxa_stateMachine_t*)userVarIn;
	cxa_assert(smIn);

	// make sure we've been marked as started
	if( !smIn->hasStarted ) smIn->hasStarted = true;

	// notify our listeners
	#ifdef CXA_STATE_MACHINE_ENABLE_LISTENERS
	cxa_array_iterate(&smIn->listeners, currListener, cxa_stateMachine_listenerEntry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_beforeExecution != NULL ) currListener->cb_beforeExecution(smIn, currListener->userVar);
	}
	#endif

	// see if we should transition
	if( smIn->nextState != NULL )
	{
		// call the leaving function of our old state
		if( smIn->currState != NULL )
		{
			if( smIn->currState->cb_leaving != NULL ) smIn->currState->cb_leaving(smIn, smIn->nextState->stateId, smIn->currState->userVar);
		}

		// call the entering function of our new state
		if( smIn->nextState->cb_entering != NULL ) smIn->nextState->cb_entering(smIn, ((smIn->currState != NULL) ? smIn->currState->stateId : CXA_STATE_MACHINE_STATE_UNKNOWN), smIn->nextState->userVar);

		// actually do our transition
		cxa_stateMachine_state_t* prevState = smIn->currState;
		smIn->currState = smIn->nextState;
		smIn->nextState = NULL;

		#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
			cxa_logger_info(&smIn->logger, "new state: '%s'", smIn->currState->stateName);
		#endif

		// call the left function of our previous state
		if( prevState != NULL )
		{
			if( prevState->cb_left != NULL ) prevState->cb_left(smIn, smIn->currState->stateId, prevState->userVar);
		}

		// call the entered function of our new state
		if( smIn->currState->cb_entered != NULL ) smIn->currState->cb_entered(smIn, ((prevState != NULL) ? prevState->stateId : CXA_STATE_MACHINE_STATE_UNKNOWN), smIn->currState->userVar);

		#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
			if( smIn->currState->type == CXA_STATE_MACHINE_STATE_TYPE_TIMED ) cxa_timeDiff_setStartTime_now(&smIn->td_timedTransition);
		#endif

		// notify our listeners last
		#ifdef CXA_STATE_MACHINE_ENABLE_LISTENERS
		cxa_array_iterate(&smIn->listeners, currListener, cxa_stateMachine_listenerEntry_t)
		{
			if( currListener == NULL ) continue;

			if( currListener->cb_onTransition != NULL ) currListener->cb_onTransition(smIn, (prevState != NULL) ? prevState->stateId : CXA_STATE_MACHINE_STATE_UNKNOWN, smIn->currState->stateId, currListener->userVar);
		}
		#endif
	}
	else
	{
		#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
			// see if our state's time has expired...if so, transition into our next state
			if( (smIn->currState->type == CXA_STATE_MACHINE_STATE_TYPE_TIMED) &&
				cxa_timeDiff_isElapsed_ms(&smIn->td_timedTransition, smIn->currState->stateTime_ms) )
			{
				cxa_stateMachine_transition(smIn, smIn->currState->nextStateId);
				return;
			}
		#endif

		// keep updating our state
		if( (smIn->currState != NULL) && (smIn->currState->cb_state != NULL) ) smIn->currState->cb_state(smIn, smIn->currState->userVar);
	}

	// notify our listeners
	#ifdef CXA_STATE_MACHINE_ENABLE_LISTENERS
	cxa_array_iterate(&smIn->listeners, currListener, cxa_stateMachine_listenerEntry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_afterExecution != NULL ) currListener->cb_afterExecution(smIn, currListener->userVar);
	}
	#endif
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
