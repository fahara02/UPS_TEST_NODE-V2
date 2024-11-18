#ifndef PZEM_MEASURE_HPP
#define PZEM_MEASURE_HPP
#include "cstdint"
#include <array>

namespace Node_Core
{
enum class PZEMModel
{
	PZEM004T,
	PZEM003,
	NOT_SET
};
enum class Phase
{
	SINGLE_PHASE,
	RED_PHASE,
	YELLOW_PHASE,
	BLUE_PHASE
};
enum class PZEMState
{
	INIT,
	ACTIVE,
	IDLE,
	BUSY,
	POLL,
	UPDATE_FAIL,
	NOLOAD,
	NO_VOLTAGE
};
struct powerMeasure
{
	float voltage;
	float frequency;
	float pf;
	float alarms;
	float current;
	float power;
	float energy;
	bool isValid;
	int64_t last_measured_ms;
	powerMeasure() :
		voltage(0), frequency(0), pf(0), alarms(0), current(0), power(0), energy(0), isValid(false),
		last_measured_ms(0)
	{
	}
};

struct NamePlate
{
	PZEMModel model;
	uint8_t id;
	uint8_t slaveAddress;
	uint8_t lineNo;
	Phase phase;
	std::array<char, 9> meterName;
	NamePlate() :
		model(PZEMModel::NOT_SET), id(255), slaveAddress(0), lineNo(0), phase(Phase::SINGLE_PHASE),
		meterName{}
	{
		snprintf(meterName.data(), meterName.size(), "PZEM-123");
	}
	~NamePlate() = default;
};
struct JobCard

{
	NamePlate info;
	powerMeasure pm;
	mutable int64_t poll_us = 0;
	int64_t lastUpdate_us = 0;
	int64_t dataAge_ms = 0;
	bool dataStale = false;
	PZEMState deviceState;

	JobCard() :
		info(), pm(), poll_us(0), lastUpdate_us(0), dataAge_ms(0), dataStale(false),
		deviceState{PZEMState::INIT}
	{
	}
};

} // namespace Node_Core

#endif