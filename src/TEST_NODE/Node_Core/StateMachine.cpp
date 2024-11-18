#include "StateMachine.h"
#include "Arduino.h"
#include "Logger.h"
#include "StateDefines.h"
#include <cstring>
#include <iostream>
#include "pgmspace.h"
#include "DataHandler.h"
#include "TestManager.h"

using namespace Node_Core;
extern Logger& logger;

namespace Node_Core
{
Preferences preferences;

StateMachine::StateMachine() :
	_old_state(State::DEVICE_ON), _currentState(State::DEVICE_ON), _deviceMode(TestMode::MANUAL),
	_dataCapturedFlag(false), _retryCount(0)
{
}

bool StateMachine::isValidState(uint32_t state)
{
	return state >= static_cast<uint32_t>(State::DEVICE_ON) &&
		   state <= static_cast<uint32_t>(State::MAX_STATE);
}

StateMachine& StateMachine::getInstance()
{
	static StateMachine instance;
	return instance;
}

void StateMachine::setState(State new_state)
{
	_currentState.store(new_state);
	NotifyStateChanged(new_state);
}

State StateMachine::getCurrentState() const
{
	return _currentState.load();
}

bool StateMachine::isAutoMode() const
{
	return _deviceMode.load() == TestMode::AUTO;
}

bool StateMachine::isManualMode() const
{
	return _deviceMode.load() == TestMode::MANUAL;
}

void StateMachine::setMode(TestMode new_mode)
{
	if(new_mode == TestMode::AUTO)
	{
		_deviceMode.store(TestMode::AUTO);

		logger.log(LogLevel::SUCCESS, "DEVICE MODE SET TO AUTO");
	}
	else
	{
		_deviceMode.store(TestMode::MANUAL);
		logger.log(LogLevel::SUCCESS, "DEVICE MODE SET TO MANUAL");
	}

	NotifyModeChanged(new_mode);
}
void StateMachine::NotifyStateChanged(State state)
{
	// if(xSemaphoreTake(notifyStateMutex, portMAX_DELAY) == pdTRUE)
	// {
	DataHandler::getInstance().updateState(state);
	TestSync::getInstance().updateState(state);
	TestManager::getInstance().updateState(state);

	if(isAutoMode())
	{
		NotifyModeChanged(TestMode::AUTO);
	}
	else if(isManualMode())
	{
		NotifyModeChanged(TestMode::MANUAL);
	}

	logger.log(LogLevel::INFO, "Notifying others for new %s state", stateToString(state));
	// xSemaphoreGive(notifyStateMutex);
}

void StateMachine::NotifyModeChanged(TestMode mode)
{
	// if(xSemaphoreTake(notifyModeMutex, portMAX_DELAY) == pdTRUE)
	// {
	DataHandler::getInstance().updateMode(mode);
	TestSync::getInstance().updateMode(mode);
	TestManager::getInstance().updateMode(mode);
	// 	xSemaphoreGive(notifyModeMutex);
	// }
}
void StateMachine::handleMode(TestMode mode)
{
	setMode(mode);
}

void StateMachine::handleEvent(Event event)

{
	// if(xSemaphoreTake(stateActionMutex, portMAX_DELAY) == pdTRUE)
	// {
	logger.log(LogLevel::INFO, "Handling event %s", eventToString(event));

	State old_state = _currentState.load(); // Ensure atomic access to current_state

	if(event == Event::SYSTEM_FAULT)
	{
		_old_state.store(old_state);
		State new_state = State::FAULT;
		setState(new_state);

		return;
	}

	if(event == Event::PAUSE)
	{
		_old_state.store(old_state);
		State new_state = State::SYSTEM_PAUSED;
		setState(new_state);

		logger.log(LogLevel::WARNING, "State now in: %s", stateToString(new_state));
		return;
	}

	if(event == Event::USER_TUNE)
	{
		_old_state.store(old_state);
		State new_state = State::SYSTEM_TUNING;
		setState(new_state);

		return;
	}

	if(event == Event::ERROR)
	{
		handleError();
		return;
	}

	// Manually search for the transition
	for(const auto& transition: transition_table)
	{
		if(transition.current_state == _currentState && transition.event == event)
		{
			if(transition.guard())
			{ // Check guard condition
				_old_state.store(old_state);
				setState(transition.next_state); // Transition to the next state

				// Reset the dataCapturedFlag if necessary
				if(event == Event::TEST_TIME_END)
				{
					_dataCapturedFlag.store(false);
				}

				logger.log(LogLevel::INFO, "State changed from %s to %s", stateToString(old_state),
						   stateToString(transition.next_state));

				transition.action(); // Execute the associated action

				// Additional logging for special cases
				if(event == Event::TEST_TIME_END && _currentState == State::CURRENT_TEST_CHECK)
				{
					logger.log(LogLevel::INFO, "TEST_TIME_END handled in CURRENT_TEST_CHECK");
				}

				return;
			}
		}
	}

	// No valid transition found
	logger.log(LogLevel::WARNING, "No transition found for event %s in state %s",
			   eventToString(event), stateToString(_currentState.load()));
	// 	xSemaphoreGive(stateActionMutex);
	// }
}

void StateMachine::handleError()
{
	// updateStateEventGroup(State::LOG_ERROR, true);
}

void StateMachine::handleReport()
{
	// update report
}
static void DefaultAction()
{
}
static void notifyRejectTest()
{
}
static void notifyStateReverese()
{
}
static void notifyUser(Event e)
{
}
const std::array<StateMachine::Transition, 23> StateMachine::regular_transitions = {{

	// Startup
	Row<State::DEVICE_ON, Event::SELF_CHECK_OK, State::DEVICE_OK, Event::NONE>::get_transition(
		DefaultAction),
	Row<State::DEVICE_OK, Event::SETTING_LOADED, State::DEVICE_SETUP,
		Event::NONE>::get_transition(),
	Row<State::DEVICE_SETUP, Event::LOAD_BANK_CHECKED, State::DEVICE_READY,
		Event::NONE>::get_transition(),
	Row<State::DEVICE_READY, Event::NEW_TEST, State::READY_TO_PROCEED,
		Event::NONE>::get_transition(),
	Row<State::READY_TO_PROCEED, Event::NEW_TEST, State::READY_TO_PROCEED,
		Event::NONE>::get_transition(),
	Row<State::DEVICE_READY, Event::REJECT_CURRENT_TEST, State::DEVICE_READY,
		Event::NONE>::get_transition(),
	Row<State::READY_TO_PROCEED, Event::REJECT_CURRENT_TEST, State::DEVICE_READY,
		Event::NONE>::get_transition(notifyStateReverese),
	// TEST
	Row<State::READY_TO_PROCEED, Event::START, State::TEST_START, Event::NONE>::get_transition(),
	Row<State::TEST_START, Event::STOP, State::READY_TO_PROCEED, Event::NONE>::get_transition(),
	Row<State::TEST_START, Event::TEST_RUN_OK, State::TEST_RUNNING, Event::NONE>::get_transition(),
	Row<State::TEST_RUNNING, Event::STOP, State::READY_TO_PROCEED, Event::NONE>::get_transition(),
	Row<State::TEST_START, Event::TEST_FAILED, State::USER_CHECK_REQUIRED,
		Event::NONE>::get_transition(),
	Row<State::TEST_RUNNING, Event::TEST_FAILED, State::RETEST, Event::NONE>::get_transition(),
	Row<State::USER_CHECK_REQUIRED, Event::START, State::TEST_START, Event::NONE>::get_transition(),
	Row<State::CURRENT_TEST_CHECK, Event::VALID_DATA, State::CURRENT_TEST_OK,
		Event::NONE>::get_transition(),
	Row<State::CURRENT_TEST_CHECK, Event::TEST_FAILED, State::RETEST,
		Event::NONE>::get_transition(),
	Row<State::CURRENT_TEST_OK, Event::SAVE, State::READY_NEXT_TEST, Event::NONE>::get_transition(),
	Row<State::READY_NEXT_TEST, Event::PENDING_TEST_FOUND, State::TEST_START,
		Event::NONE>::get_transition([]() {
		return StateMachine::getInstance().isAutoMode();
	}),
	Row<State::READY_NEXT_TEST, Event::PENDING_TEST_FOUND, State::WAITING_FOR_USER,
		Event::NONE>::get_transition([]() {
		return StateMachine::getInstance().isManualMode();
	}),
	Row<State::READY_NEXT_TEST, Event::TEST_LIST_EMPTY, State::ALL_TEST_DONE,
		Event::NONE>::get_transition(),
	Row<State::CURRENT_TEST_OK, Event::TEST_FAILED, State::RECOVER_DATA,
		Event::NONE>::get_transition(),
	Row<State::RECOVER_DATA, Event::SAVE, State::START_FROM_SAVE, Event::NONE>::get_transition(),
	Row<State::ALL_TEST_DONE, Event::JSON_READY, State::TRANSPORT_DATA,
		Event::NONE>::get_transition()

}};

// Define the special case transitions
const std::array<StateMachine::Transition, 12> StateMachine::special_case_transitions = {
	{Row<State::TEST_RUNNING, Event::TEST_TIME_END, State::CURRENT_TEST_CHECK,
		 Event::NONE>::get_transition([]() {
		 // Static action (e.g., reset flag)
	 }),
	 Row<State::TEST_RUNNING, Event::DATA_CAPTURED, State::CURRENT_TEST_CHECK,
		 Event::NONE>::get_transition([]() {
		 // Static action (e.g., set flag)
	 }),
	 Row<State::CURRENT_TEST_CHECK, Event::TEST_TIME_END, State::CURRENT_TEST_CHECK,
		 Event::NONE>::get_transition(),
	 Row<State::ALL_TEST_DONE, Event::TEST_FAILED, State::RECOVER_DATA,
		 Event::NONE>::get_transition([]() {
		 StateMachine::handleError();
	 }),
	 Row<State::ADDENDUM_TEST_DATA, Event::JSON_READY, State::TRANSPORT_DATA,
		 Event::NONE>::get_transition([]() {
		 StateMachine::handleReport();
	 }),
	 Row<State::ADDENDUM_TEST_DATA, Event::TEST_FAILED, State::RECOVER_DATA,
		 Event::NONE>::get_transition([]() {
		 StateMachine::handleError();
	 }),
	 Row<State::SYSTEM_TUNING, Event::AUTO, State::RECOVER_DATA, Event::NONE>::get_transition([]() {
		 StateMachine::handleReport();
	 }),
	 Row<State::FAULT, Event::FAULT_CLEARED, State::RECOVER_DATA, Event::NONE>::get_transition(
		 []() {
			 StateMachine::handleReport();
		 }),
	 Row<State::SYSTEM_PAUSED, Event::AUTO, State::START_FROM_SAVE, Event::NONE>::get_transition(
		 []() {
			 StateMachine::handleReport();
		 }),
	 Row<State::START_FROM_SAVE, Event::PENDING_TEST_FOUND, State::TEST_START,
		 Event::NONE>::get_transition(),
	 Row<State::FAULT, Event::RESTART, State::DEVICE_ON, Event::NONE>::get_transition([]() {
		 StateMachine::handleReport();
	 })}};

// Define the combined transition table
const std::array<StateMachine::Transition, 35> StateMachine::transition_table = [] {
	std::array<Transition, 35> result = {};

	// Copy the regular transitions
	std::copy(StateMachine::regular_transitions.begin(), StateMachine::regular_transitions.end(),
			  result.begin());

	// Copy the special case transitions
	std::copy(StateMachine::special_case_transitions.begin(),
			  StateMachine::special_case_transitions.end(),
			  result.begin() + StateMachine::regular_transitions.size());

	return result;
}();

// Access functions for the transition tables
const StateMachine::Transition* StateMachine::getRegularTransitions()
{
	return StateMachine::regular_transitions.data();
}

const StateMachine::Transition* StateMachine::getSpecialCaseTransitions()
{
	return StateMachine::special_case_transitions.data();
}

const StateMachine::Transition* StateMachine::getTransitionTable()
{
	return StateMachine::transition_table.data();
}

// Implementation of other StateMachine methods...

} // namespace Node_Core
