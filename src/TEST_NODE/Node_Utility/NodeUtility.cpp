#include "NodeUtility.hpp"

#include <cstring>
using namespace Node_Utility;
const char* ToString::model(Node_Core::PZEMModel model)
{
	const char* modelStr;
	switch(model)
	{
		case Node_Core::PZEMModel::PZEM004T:
			modelStr = "PZEM004T";
			break;
		case Node_Core::PZEMModel::PZEM003:
			modelStr = "PZEM003";
			break;
		case Node_Core::PZEMModel::NOT_SET:
			modelStr = "NOT_SET";
			break;
		default:
			modelStr = "ERROR";
			break;
	}
	return modelStr;
}

const char* ToString::Phase(Node_Core::Phase phase)
{
	const char* phaseStr;
	switch(phase)
	{
		case Node_Core::Phase::SINGLE_PHASE:
			phaseStr = "SINGLE PHASE";
			break;
		case Node_Core::Phase::RED_PHASE:
			phaseStr = "RED PHASE";
			break;
		case Node_Core::Phase::YELLOW_PHASE:
			phaseStr = "YELLOW PHASE";
			break;
		case Node_Core::Phase::BLUE_PHASE:
			phaseStr = "BLUE PHASE";
			break;
		default:
			phaseStr = "UNKNOWN PHASE";
			break;
	}

	return phaseStr;
}

const char* ToString::PZEMState(Node_Core::PZEMState state)
{
	const char* stateStr;
	switch(state)
	{
		case Node_Core::PZEMState::INIT:
			stateStr = "INIT";
			break;
		case Node_Core::PZEMState::ACTIVE:
			stateStr = "ACTIVE";
			break;
		case Node_Core::PZEMState::IDLE:
			stateStr = "IDLE";
			break;
		case Node_Core::PZEMState::BUSY:
			stateStr = "BUSY";
			break;
		case Node_Core::PZEMState::POLL:
			stateStr = "POLLING";
			break;
		case Node_Core::PZEMState::UPDATE_FAIL:
			stateStr = "UPDATE FAIL";
			break;
		case Node_Core::PZEMState::NOLOAD:
			stateStr = "NO LOAD";
			break;
		case Node_Core::PZEMState::NO_VOLTAGE:
			stateStr = "NO VOLTAGE";
			break;
		default:
			stateStr = "UNKNOWN";
			break;
	}

	return stateStr;
}

const char* ToString::wsPowerDataType(Node_Core::wsPowerDataType type)
{
	switch(type)
	{
		case Node_Core::wsPowerDataType::INPUT_POWER:
			return "InputPower";
		case Node_Core::wsPowerDataType::INPUT_VOLT:
			return "InputVoltage";
		case Node_Core::wsPowerDataType::INPUT_CURRENT:
			return "InputCurrent";
		case Node_Core::wsPowerDataType::INPUT_PF:
			return "InputPowerFactor";
		case Node_Core::wsPowerDataType::OUTPUT_POWER:
			return "OutputPower";
		case Node_Core::wsPowerDataType::OUTPUT_VOLT:
			return "OutputVoltage";
		case Node_Core::wsPowerDataType::OUTPUT_CURRENT:
			return "OutputCurrent";
		case Node_Core::wsPowerDataType::OUTPUT_PF:
			return "OutputPowerFactor";
		default:
			return "Invalid Data";
	}
}

 const char* ToString:: setting(Node_Core::SettingType setting)
{
	switch(setting)
	{
		// System Events
		case Node_Core::SettingType::ALL:
			return "all";
		case Node_Core::SettingType::SPEC:
			return "spec";
		case Node_Core::SettingType::TEST:
			return "test";
		case Node_Core::SettingType::TASK:
			return "task";
		case Node_Core::SettingType::TASK_PARAMS:
			return "task_params";
		case Node_Core::SettingType::REPORT:
			return "report";
		case Node_Core::SettingType::HARDWARE:
			return "hardware";
		case Node_Core::SettingType::MODBUS:
			return "modbus";
		case Node_Core::SettingType::NETWORK:
			return "network";
		default:
			return "NONE";
	}
}

const char* ToString::state(Node_Core::State state)
{
	switch(state)
	{
		case Node_Core::State::DEVICE_ON:
			return "DEVICE_ON";
		case Node_Core::State::DEVICE_OK:
			return "DEVICE_OK";
		case Node_Core::State::DEVICE_SETUP:
			return "DEVICE_SETUP";
		case Node_Core::State::DEVICE_READY:
			return "DEVICE_READY";
		case Node_Core::State::READY_TO_PROCEED:
			return "READY_TO_PROCEED";
		case Node_Core::State::TEST_START:
			return "TEST_START";
		case Node_Core::State::TEST_RUNNING:
			return "TEST_RUNNING";
		case Node_Core::State::CURRENT_TEST_CHECK:
			return "CURRENT_TEST_CHECK";
		case Node_Core::State::CURRENT_TEST_OK:
			return "CURRENT_TEST_OK";
		case Node_Core::State::READY_NEXT_TEST:
			return "READY_NEXT_TEST";
		case Node_Core::State::MANUAL_NEXT_TEST:
			return "MANUAL_NEXT_TEST";
		case Node_Core::State::RETEST:
			return "RETEST";
		case Node_Core::State::SYSTEM_PAUSED:
			return "SYSTEM_PAUSED";
		case Node_Core::State::ALL_TEST_DONE:
			return "ALL_TEST_DONE";
		case Node_Core::State::START_FROM_SAVE:
			return "START_FROM_SAVE";
		case Node_Core::State::RECOVER_DATA:
			return "RECOVER_DATA";
		case Node_Core::State::ADDENDUM_TEST_DATA:
			return "ADDENDUM_TEST_DATA";
		case Node_Core::State::FAILED_TEST:
			return "FAILED_TEST";
		case Node_Core::State::TRANSPORT_DATA:
			return "TRANSPORT_DATA";
		case Node_Core::State::SYSTEM_TUNING:
			return "SYSTEM_TUNING";
		case Node_Core::State::FAULT:
			return "FAULT";
		case Node_Core::State::USER_CHECK_REQUIRED:
			return "USER_CHECK_REQUIRED";
		case Node_Core::State::WAITING_FOR_USER:
			return "WAITING_FOR_USER";
		case Node_Core::State::MAX_STATE:
			return "MAX_STATE";
		default:
			return "UNKNOWN_STATE";
	}
}

 const char* ToString::event(Node_Core::Event event)
{
	switch(event)
	{
		// System Events
		case Node_Core::Event::NONE:
			return "NONE";
		case Node_Core::Event::ERROR:
			return "ERROR";
		case Node_Core::Event::SYSTEM_FAULT:
			return "SYSTEM_FAULT";
		case Node_Core::Event::FAULT_CLEARED:
			return "FAULT_CLEARED";
		case Node_Core::Event::NETWORK_DISCONNECTED:
			return "NETWORK_DISCONNECTED";
		case Node_Core::Event::RESTART:
			return "RESTART";

		// System Init Events
		case Node_Core::Event::SETTING_LOADED:
			return "SETTING_LOADED";
		case Node_Core::Event::SELF_CHECK_OK:
			return "SELF_CHECK_OK";
		case Node_Core::Event::LOAD_BANK_CHECKED:
			return "LOAD_BANK_CHECKED";

		// Test Events
		case Node_Core::Event::TEST_RUN_OK:
			return "TEST_RUN_OK";
		case Node_Core::Event::TEST_TIME_END:
			return "TEST_TIME_END";
		case Node_Core::Event::DATA_CAPTURED:
			return "DATA_CAPTURED";
		case Node_Core::Event::VALID_DATA:
			return "VALID_DATA";
		case Node_Core::Event::TEST_FAILED:
			return "TEST_FAILED";
		case Node_Core::Event::RETEST:
			return "RETEST";
		case Node_Core::Event::TEST_LIST_EMPTY:
			return "TEST_LIST_EMPTY";
		case Node_Core::Event::PENDING_TEST_FOUND:
			return "PENDING_TEST_FOUND";
		case Node_Core::Event::REJECT_CURRENT_TEST:
			return "REJECT_CURRENT_TEST";

		// User Commands
		case Node_Core::Event::START:
			return "START";
		case Node_Core::Event::STOP:
			return "STOP";
		case Node_Core::Event::AUTO:
			return "AUTO";
		case Node_Core::Event::MANUAL:
			return "MANUAL";
		case Node_Core::Event::PAUSE:
			return "PAUSE";
		case Node_Core::Event::RESUME:
			return "RESUME";

		// User Updates
		case Node_Core::Event::USER_TUNE:
			return "USER_TUNE";
		case Node_Core::Event::DATA_ENTRY:
			return "DATA_ENTRY";
		case Node_Core::Event::NEW_TEST:
			return "NEW_TEST";
		case Node_Core::Event::DELETE_TEST:
			return "DELETE_TEST";

		// Data Events
		case Node_Core::Event::SAVE:
			return "SAVE";
		case Node_Core::Event::JSON_READY:
			return "JSON_READY";

		default:
			return "UNKNOWN_EVENT";
	}
}