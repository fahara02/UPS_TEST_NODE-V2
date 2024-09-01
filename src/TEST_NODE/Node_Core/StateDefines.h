#ifndef STATE_DEFINES_H_
#define STATE_DEFINES_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <cstdint>

namespace Node_Core
{

enum class State
{
	DEVICE_ON,
	DEVICE_OK,
	DEVICE_SETUP,
	DEVICE_READY,
	READY_TO_PROCEED,
	TEST_START,
	TEST_RUNNING,
	CURRENT_TEST_CHECK,
	CURRENT_TEST_OK, // Added CURRENT_TEST_OK
	READY_NEXT_TEST,
	MANUAL_NEXT_TEST,
	RETEST,
	SYSTEM_PAUSED,
	ALL_TEST_DONE,
	START_FROM_SAVE,
	RECOVER_DATA,
	ADDENDUM_TEST_DATA,
	FAILED_TEST,
	TRANSPORT_DATA,
	SYSTEM_TUNING,
	FAULT,
	USER_CHECK_REQUIRED,
	WAITING_FOR_USER,
	MAX_STATE
};

enum class Event
{ // System Events
	NONE,
	ERROR,
	SYSTEM_FAULT,
	FAULT_CLEARED,
	NETWORK_DISCONNECTED,
	RESTART,
	// System init events
	SETTING_LOADED,
	SELF_CHECK_OK,
	LOAD_BANK_CHECKED,
	// TestEvents
	TEST_RUN_OK,
	TEST_TIME_END,
	DATA_CAPTURED,
	VALID_DATA,
	TEST_FAILED,
	RETEST,
	TEST_LIST_EMPTY,
	PENDING_TEST_FOUND,
	// User commands
	START,
	STOP,
	AUTO,
	MANUAL,
	PAUSE,
	RESUME,
	// User Updates
	USER_TUNE,
	DATA_ENTRY,
	NEW_TEST,
	DELETE_TEST,
	// Data Events
	SAVE,
	JSON_READY
};
static const char* stateToString(State state)
{
	switch(state)
	{
		case State::DEVICE_ON:
			return "DEVICE_ON";
		case State::DEVICE_OK:
			return "DEVICE_OK";
		case State::DEVICE_SETUP:
			return "DEVICE_SETUP";
		case State::DEVICE_READY:
			return "DEVICE_READY";

		case State::TEST_START:
			return "TEST_START";
		case State::TEST_RUNNING:
			return "TEST_ONGOING";
		case State::CURRENT_TEST_OK:
			return "CURRENT_TEST_OK";
		case State::READY_NEXT_TEST:
			return "READY_NEXT_TEST";
		case State::RETEST:
			return "RETEST";
		case State::SYSTEM_PAUSED:
			return "SYSTEM_PAUSED";
		case State::ALL_TEST_DONE:
			return "ALL_TEST_DONE";
		case State::START_FROM_SAVE:
			return "START_FROM_SAVE";
		case State::RECOVER_DATA:
			return "RECOVER_DATA";
		case State::ADDENDUM_TEST_DATA:
			return "ADDENDUM_TEST_DATA";
		case State::FAILED_TEST:
			return "FAILED_TEST";
		case State::TRANSPORT_DATA:
			return "TRANSPORT_DATA";
		case State::SYSTEM_TUNING:
			return "SYSTEM_TUNING";
		case State::FAULT:
			return "FAULT";
		// case State::MANUAL_NEXT_TEST:
		//   return "MANUAL_NEXT_TEST";
		// case State::REPORT_AVAILABLE:
		//   return "REPORT_AVAILABLE";
		// case State::NEW_TEST:
		//   return "NEW_TEST";
		// case State::LOG_ERROR:
		//   return "LOG_ERROR";
		default:
			return "UNKNOWN_STATE";
	}
}

static const char* eventToString(Event event)
{
	switch(event)
	{
		// System Events
		case Event::NONE:
			return "NONE";
		case Event::ERROR:
			return "ERROR";
		case Event::SYSTEM_FAULT:
			return "SYSTEM_FAULT";
		case Event::FAULT_CLEARED:
			return "FAULT_CLEARED";
		case Event::NETWORK_DISCONNECTED:
			return "NETWORK_DISCONNECTED";
		case Event::RESTART:
			return "RESTART";

		// Test Initialization Events
		case Event::SETTING_LOADED:
			return "SETTING_LOADED";
		case Event::SELF_CHECK_OK:
			return "SELF_CHECK_OK";
		case Event::LOAD_BANK_CHECKED:
			return "LOAD_BANK_CHECKED";
		case Event::AUTO:
			return "AUTO";
		case Event::PENDING_TEST_FOUND:
			return "PENDING_TEST_FOUND";

		// Test Execution Events
		case Event::TEST_RUN_OK:
			return "TEST_ONGOING";
		case Event::TEST_TIME_END:
			return "TEST_TIME_END"; // Newly added event
		case Event::DATA_CAPTURED:
			return "DATA_CAPTURED";
		case Event::VALID_DATA:
			return "VALID_DATA";
		case Event::TEST_FAILED:
			return "TEST_FAILED";
		case Event::RETEST:
			return "RETEST";
		case Event::TEST_LIST_EMPTY:
			return "TEST_LIST_EMPTY";

		// User Interaction Events
		case Event::USER_TUNE:
			return "USER_TUNE";
		case Event::MANUAL:
			return "MANUAL_OVERRIDE";
		case Event::DATA_ENTRY:
			return "MANUAL_DATA_ENTRY";
		case Event::PAUSE:
			return "USER_PAUSED";
		case Event::RESUME:
			return "USER_RESUME";

		// Data Handling Events
		case Event::SAVE:
			return "SAVE";
		case Event::JSON_READY:
			return "JSON_READY";

		default:
			return "UNKNOWN_EVENT";
	}
}

} // namespace Node_Core

#endif // STATE_DEFINES_H_
