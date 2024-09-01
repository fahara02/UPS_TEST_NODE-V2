#include "StateMachine.h"
#include "Arduino.h"
#include "Logger.h"
#include "StateDefines.h"
#include <cstring>
#include <iostream>

using namespace Node_Core;
extern Logger& logger;
extern xSemaphoreHandle state_mutex;
namespace Node_Core
{

StateMachine::StateMachine() :
	_old_state(State::DEVICE_ON), current_state(State::DEVICE_ON), _dataCapturedFlag(false),
	_retryCount(0)
{
	// state_mutex = xSemaphoreCreateMutex();
	// if (state_mutex == NULL) {
	//   logger.log(LogLevel::ERROR, "State mutex creation failed!");
	// }
}

StateMachine& StateMachine::getInstance()
{
	static StateMachine instance;
	return instance;
}

void StateMachine::setState(State new_state)
{
	current_state.store(new_state);

	// Take the mutex to ensure exclusive access
	// if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
	//   // Perform the atomic state update
	//   current_state.store(new_state);
	//   xSemaphoreGive(state_mutex);
	// } else {
	//   // Handle error: Failed to take mutex
	//   // For example, use a watchdog or error log
	// }
}

State StateMachine::getCurrentState() const
{
	State state;
	state = current_state.load();
	// Take the mutex to ensure exclusive access
	// if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
	//   // Read the atomic state
	//   state = current_state.load();
	//   xSemaphoreGive(state_mutex);
	// } else {
	//   // Handle error: Failed to take mutex
	//   state = State::DEVICE_ON;  // or some default state
	// }
	return state;
}
bool StateMachine::isAutoMode() const
{
	return _deviceMode.load() == TestMode::AUTO;
}

bool StateMachine::isManualMode() const
{
	return _deviceMode.load() == TestMode::MANUAL;
}

void StateMachine::NotifyStateChanged(State state)
{
	logger.log(LogLevel::INFO, "Notifying others for new %s state", stateToString(state));
}

void StateMachine::handleEvent(Event event)
{
	logger.log(LogLevel::INFO, "Handling event %s", eventToString(event));

	State old_state = current_state.load(); // Ensure atomic access to current_state

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
		if(transition.current_state == current_state && transition.event == event)
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
				if(event == Event::TEST_TIME_END && current_state == State::CURRENT_TEST_CHECK)
				{
					logger.log(LogLevel::INFO, "TEST_TIME_END handled in CURRENT_TEST_CHECK");
				}

				return;
			}
		}
	}

	// No valid transition found
	logger.log(LogLevel::WARNING, "No transition found for event %s in state %s",
			   eventToString(event), stateToString(current_state.load()));
}

void StateMachine::handleError()
{
	// updateStateEventGroup(State::LOG_ERROR, true);
}

void StateMachine::handleReport()
{
	// update report
}

// Static member function definitions
std::array<StateMachine::Transition, 18> StateMachine::createRegularTransitions()
{
	return {
		Row<State::DEVICE_ON, Event::SELF_CHECK_OK, State::DEVICE_OK,
			Event::NONE>::get_transition(),
		Row<State::DEVICE_OK, Event::SETTING_LOADED, State::DEVICE_SETUP,
			Event::NONE>::get_transition(),
		Row<State::DEVICE_SETUP, Event::LOAD_BANK_CHECKED, State::DEVICE_READY,
			Event::NONE>::get_transition(),
		Row<State::DEVICE_READY, Event::NEW_TEST, State::READY_TO_PROCEED,
			Event::NONE>::get_transition(),
		Row<State::READY_TO_PROCEED, Event::START, State::TEST_START,
			Event::NONE>::get_transition(),
		Row<State::TEST_START, Event::TEST_RUN_OK, State::TEST_RUNNING,
			Event::NONE>::get_transition(),
		Row<State::TEST_START, Event::TEST_FAILED, State::USER_CHECK_REQUIRED,
			Event::NONE>::get_transition(),
		Row<State::TEST_RUNNING, Event::TEST_FAILED, State::RETEST, Event::NONE>::get_transition(),
		Row<State::USER_CHECK_REQUIRED, Event::START, State::TEST_START,
			Event::NONE>::get_transition(),
		Row<State::CURRENT_TEST_CHECK, Event::VALID_DATA, State::CURRENT_TEST_OK,
			Event::NONE>::get_transition(),
		Row<State::CURRENT_TEST_CHECK, Event::TEST_FAILED, State::RETEST,
			Event::NONE>::get_transition(),
		Row<State::CURRENT_TEST_OK, Event::SAVE, State::READY_NEXT_TEST,
			Event::NONE>::get_transition(),
		Row<State::READY_NEXT_TEST, Event::PENDING_TEST_FOUND, State::TEST_START,
			Event::NONE>::get_transition([]() {
			return StateMachine::getInstance().isAutoMode(); // Custom guard function
		}),
		Row<State::READY_NEXT_TEST, Event::PENDING_TEST_FOUND, State::WAITING_FOR_USER,
			Event::NONE>::get_transition([]() {
			return StateMachine::getInstance().isManualMode(); // Custom guard function
		}),
		Row<State::READY_NEXT_TEST, Event::TEST_LIST_EMPTY, State::ALL_TEST_DONE,
			Event::NONE>::get_transition(),
		Row<State::CURRENT_TEST_OK, Event::TEST_FAILED, State::RECOVER_DATA,
			Event::NONE>::get_transition(),
		Row<State::RECOVER_DATA, Event::SAVE, State::START_FROM_SAVE,
			Event::NONE>::get_transition(),
		Row<State::ALL_TEST_DONE, Event::JSON_READY, State::TRANSPORT_DATA,
			Event::NONE>::get_transition()};
}

std::array<StateMachine::Transition, 11> StateMachine::createSpecialCaseTransitions()
{
	return {
		Row<State::TEST_RUNNING, Event::TEST_TIME_END, State::CURRENT_TEST_CHECK,
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
		Row<State::SYSTEM_TUNING, Event::AUTO, State::RECOVER_DATA, Event::NONE>::get_transition(
			[]() {
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
		})};
}

const std::array<StateMachine::Transition, 29> StateMachine::transition_table = [] {
	std::array<Transition, 29> result = {}; // Initialize an empty array of 29 transitions

	std::copy(createRegularTransitions().begin(), createRegularTransitions().end(), result.begin());
	std::copy(createSpecialCaseTransitions().begin(), createSpecialCaseTransitions().end(),
			  result.begin() + createRegularTransitions().size());

	return result;
}();

} // namespace Node_Core
