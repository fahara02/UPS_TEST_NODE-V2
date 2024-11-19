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

const char* ToString::State(Node_Core::PZEMState state)
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