/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_STATE_MACHINE_H_
#define CXA_STATE_MACHINE_H_


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

#ifndef CXA_STATE_MACHINE_MAXNUM_LISTENERS
	#define CXA_STATE_MACHINE_MAXNUM_LISTENERS			1
#endif

#define CXA_STATE_MACHINE_STATE_UNKNOWN					-1


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_stateMachine cxa_stateMachine_t;


/**
 * @public
 */
typedef void (*cxa_stateMachine_cb_entering_t)(cxa_stateMachine_t *const smIn, int prevStateIdIn, void* userVarIn);
typedef void (*cxa_stateMachine_cb_entered_t)(cxa_stateMachine_t *const smIn, int prevStateIdIn, void* userVarIn);
typedef void (*cxa_stateMachine_cb_state_t)(cxa_stateMachine_t *const smIn, void *userVarIn);
typedef void (*cxa_stateMachine_cb_leaving_t)(cxa_stateMachine_t *const smIn, int nextStateIdIn, void* userVarIn);
typedef void (*cxa_stateMachine_cb_left_t)(cxa_stateMachine_t *const smIn, int nextStateIdIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_stateMachine_listenerCb_onTransition_t)(cxa_stateMachine_t *const smIn, int prevStateIdIn, int newStateIdIn, void* userVarIn);


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

	cxa_stateMachine_cb_entering_t cb_entering;
	cxa_stateMachine_cb_entered_t cb_entered;
	cxa_stateMachine_cb_state_t cb_state;
	cxa_stateMachine_cb_leaving_t cb_leaving;
	cxa_stateMachine_cb_left_t cb_left;
	void *userVar;

	#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
		int nextStateId;
		uint32_t stateTime_ms;
	#endif
}cxa_stateMachine_state_t;


/**
 * @private
 */
typedef struct
{
	cxa_stateMachine_listenerCb_onTransition_t cb;
	void* userVar;
}cxa_stateMachine_listenerEntry_t;


/**
 * @public
 */
struct cxa_stateMachine
{
	cxa_stateMachine_state_t* currState;
	cxa_stateMachine_state_t* nextState;

	bool hasStarted;

	cxa_array_t states;
	cxa_stateMachine_state_t states_raw[CXA_STATE_MACHINE_MAXNUM_STATES];

	#ifdef CXA_STATE_MACHINE_ENABLE_LOGGING
		cxa_logger_t logger;
	#endif

	#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
		cxa_timeDiff_t td_timedTransition;
	#endif

	#ifdef CXA_STATE_MACHINE_ENABLE_LISTENERS
		cxa_array_t listeners;
		cxa_stateMachine_listenerEntry_t listeners_raw[CXA_STATE_MACHINE_MAXNUM_LISTENERS];
	#endif
};


// ******** global function prototypes ********
void cxa_stateMachine_init(cxa_stateMachine_t *const smIn, const char* nameIn, int threadIdIn);

void cxa_stateMachine_addState(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn,
	cxa_stateMachine_cb_entered_t cb_enteredIn, cxa_stateMachine_cb_state_t cb_stateIn, cxa_stateMachine_cb_leaving_t cb_leavingIn,
	void *userVarIn);

void cxa_stateMachine_addState_full(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn,
		cxa_stateMachine_cb_entering_t cb_enteringIn, cxa_stateMachine_cb_entered_t cb_enteredIn,
		cxa_stateMachine_cb_state_t cb_stateIn,
		cxa_stateMachine_cb_leaving_t cb_leavingIn, cxa_stateMachine_cb_left_t cb_leftIn,
		void *userVarIn);

#ifdef CXA_STATE_MACHINE_ENABLE_TIMED_STATES
void cxa_stateMachine_addState_timed(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn, int nextStateIdIn, uint32_t stateTime_msIn,
		cxa_stateMachine_cb_entered_t cb_enteredIn, cxa_stateMachine_cb_state_t cb_stateIn, cxa_stateMachine_cb_leaving_t cb_leavingIn, void *userVarIn);

void cxa_stateMachine_addState_timed_full(cxa_stateMachine_t *const smIn, int idIn, const char* nameIn, int nextStateIdIn, uint32_t stateTime_msIn,
		cxa_stateMachine_cb_entering_t cb_enteringIn, cxa_stateMachine_cb_entered_t cb_enteredIn,
		cxa_stateMachine_cb_state_t cb_stateIn,
		cxa_stateMachine_cb_leaving_t cb_leavingIn, cxa_stateMachine_cb_left_t cb_leftIn,
		void *userVarIn);
#endif

#ifdef CXA_STATE_MACHINE_ENABLE_LISTENERS
void cxa_stateMachine_addListener(cxa_stateMachine_t *const smIn, cxa_stateMachine_listenerCb_onTransition_t cbIn, void *userVarIn);
#endif

void cxa_stateMachine_setInitialState(cxa_stateMachine_t *const smIn, int stateIdIn);

void cxa_stateMachine_transition(cxa_stateMachine_t *const smIn, int stateIdIn);
void cxa_stateMachine_transitionNow(cxa_stateMachine_t *const smIn, int stateIdIn);

int cxa_stateMachine_getCurrentState(cxa_stateMachine_t *const smIn);
const char* cxa_stateMachine_getCurrentState_name(cxa_stateMachine_t *const smIn);


#endif // CXA_STATE_MACHINE_H_
