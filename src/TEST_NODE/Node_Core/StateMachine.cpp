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
	_old_state(State::DEVICE_ON), current_state(State::DEVICE_ON), retry_count(0), max_retest(0)
{
	EventGroupHandle_t systemStateEventGroup = nullptr;
	EventGroupHandle_t systemEventsEventGroup = nullptr;
	if(systemStateEventGroup == nullptr)
	{
		systemStateEventGroup = xEventGroupCreate();
	}
	if(systemEventsEventGroup == nullptr)
	{
		systemEventsEventGroup = xEventGroupCreate();
	}

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

void StateMachine::updateStateEventGroup(State state, bool set_bits)
{
	EventBits_t bits = static_cast<EventBits_t>(state);

	if(set_bits)
	{
		xEventGroupSetBits(systemStateEventGroup, bits);
	}
	else
	{
		xEventGroupClearBits(systemStateEventGroup, bits);
	}
}
void StateMachine::NotifySystemEventGroup(Event event, bool set_bits)
{
	EventBits_t bits_event = static_cast<EventBits_t>(event);
	logger.log(LogLevel::INFO, "Processing System Event %s", eventToString(event));
	if(set_bits)
	{
		xEventGroupSetBits(systemEventsEventGroup, bits_event);
	}
	else
	{
		xEventGroupClearBits(systemEventsEventGroup, bits_event);
	}
}

void StateMachine::handleEventbits(EventBits_t event_bits)
{
	// StateMachine& instance = StateMachine::getInstance();
	Event event = static_cast<Event>(event_bits);
	handleEvent(event);
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
		updateStateEventGroup(old_state, false);
		updateStateEventGroup(new_state, true);
		return;
	}

	if(event == Event::USER_PAUSED)
	{
		_old_state.store(old_state);
		State new_state = State::SYSTEM_PAUSED;
		setState(new_state);
		updateStateEventGroup(old_state, false);
		updateStateEventGroup(new_state, true);
		logger.log(LogLevel::WARNING, "State now in: %s", stateToString(new_state));
		return;
	}

	if(event == Event::USER_TUNE)
	{
		_old_state.store(old_state);
		State new_state = State::SYSTEM_TUNING;
		setState(new_state);
		updateStateEventGroup(old_state, false);
		updateStateEventGroup(new_state, true);
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
					dataCapturedFlag.store(false);
				}

				// Update the event group and log after setting the new state
				updateStateEventGroup(old_state, false);
				updateStateEventGroup(current_state.load(), true);

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
	updateStateEventGroup(State::REPORT_AVAILABLE, true);
}
// Convert State enum to string

} // namespace Node_Core
