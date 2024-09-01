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
	_old_state(State::DEVICE_ON), current_state(State::DEVICE_ON),_dataCapturedFlag(false),_retryCount(0)
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
	//update report
}
// Convert State enum to string

} // namespace Node_Core
