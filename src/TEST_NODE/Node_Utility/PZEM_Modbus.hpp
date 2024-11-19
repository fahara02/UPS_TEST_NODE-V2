#ifndef PZEM_MODBUS_HPP
#define PZEM_MODBUS_HPP

#include <ModbusClientTCP.h>
#include <IPAddress.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>
#include "HPTSettings.h"
#include "PZEM_Measure.hpp"
#include "NodeUtility.hpp"
#include "string.h"

namespace Node_Utility
{
enum class TargetType
{
	INPUT_POWER,
	OUTPUT_POWER,
	SWITCH_CONTROL,
	ANY
};
class ModbusManager
{
  public:
	struct PollingTarget
	{
		TargetType type = TargetType::ANY;
		uint32_t token;
		IPAddress target_ip;
		uint8_t slave_id;
		Modbus::FunctionCode function_code;
		uint16_t start_address;
		uint16_t length;
	};

  private:
	std::unique_ptr<ModbusClientTCP> MBClient;
	uint32_t _timeout;
	uint32_t _interval;
	std::vector<PollingTarget> _targets;
	uint32_t _currentToken;
	TaskHandle_t modbusTaskHandle = NULL;
	uint8_t inputPowerDevice_SlaveId = 0;
	uint8_t outputPowerDevice_SlaveId = 0;
	Node_Core::JobCard inputPowerDeviceCard = JobCard();
	Node_Core::JobCard outputPowerDeviceCard = JobCard();
	Node_Core::powerMeasure inputPowerMeasure = powerMeasure();
	Node_Core::powerMeasure outputPowerMeasure = powerMeasure();

	// Private constructor for singleton pattern
	ModbusManager(std::unique_ptr<ModbusClientTCP> MClient, uint32_t tm = 4000,
				  uint32_t inter = 2000) :
		MBClient(std::move(MClient)),
		_timeout(tm), _interval(inter), _currentToken(0)
	{
		MBClient->onDataHandler([this](ModbusMessage response, uint32_t token) {
			this->handleData(response, token);
		});
		MBClient->onErrorHandler([this](Error error, uint32_t token) {
			this->handleError(error, token);
		});
		MBClient->setTimeout(_timeout, _interval);
		MBClient->begin(); // Start ModbusTCP background task
	}

	// Generate a unique token
	uint32_t generateUniqueToken()
	{
		return ++_currentToken;
	}

	// Internal method to start the polling task
	void startPollingTask()
	{
		xTaskCreatePinnedToCore(
			[](void* pvParameters) {
				static_cast<ModbusManager*>(pvParameters)->pollingTask();
			},
			"ModbusPollingTask",
			modbus_Stack, // Stack size
			this, // Task parameter
			modbus_Priority, // Priority
			&modbusTaskHandle, // Task handle
			modbus_CORE);
	}

	// Polling task implementation
	void pollingTask()
	{
		while(true)
		{
			for(const auto& target: _targets)
			{
				MBClient->setTarget(target.target_ip, 502, _timeout, _interval);

				Modbus::Error modbusError =
					MBClient->addRequest(target.token, target.slave_id, target.function_code,
										 target.start_address, target.length);

				if(modbusError != Modbus::SUCCESS)
				{
					ModbusError e(modbusError);
					Serial.printf("Error creating request for token %08X: %02X - %s\n",
								  target.token, (int)e, (const char*)e);
				}

				vTaskDelay(pdMS_TO_TICKS(500)); // Delay between requests
			}

			vTaskDelay(pdMS_TO_TICKS(1000)); // Delay before the next polling cycle
		}
	}

	// Data and error handlers
	void handleData(ModbusMessage response, uint32_t token)
	{
		// Serial.printf("\nResponse: serverID=%d, FC=%d, Token=%08X, length=%d:\n",
		// 			  response.getServerID(), response.getFunctionCode(), token, response.size());
		// for(auto& byte: response)
		// {
		// 	Serial.printf("%02X ", byte);
		// }
		// Serial.println("\n");

		// Find the matching target by token
		auto targetIt = std::find_if(_targets.begin(), _targets.end(), [&](const PollingTarget& t) {
			return t.token == token;
		});

		if(targetIt == _targets.end())
		{
			Serial.println("Unknown target token, ignoring response.");
			return;
		}
		PollingTarget& target = *targetIt;

		// Determine the JobCard to update
		JobCard* jobCard = nullptr;
		if(target.type == TargetType::INPUT_POWER)
		{
			jobCard = &inputPowerDeviceCard;
		}
		else if(target.type == TargetType::OUTPUT_POWER)
		{
			jobCard = &outputPowerDeviceCard;
		}
		else
		{
			Serial.println("Unhandled target type.");
			return;
		}

		// Ensure the response size is valid
		if(response.size() < 20)
		{
			Serial.println("Response size is insufficient for parsing.");
			return;
		}
		const uint16_t* message = reinterpret_cast<const uint16_t*>(response.data() + 3);
		size_t messageLength = (response.size() - 3) / 2;
		// Parse the Modbus message
		if(!parseModbusMessage(message, messageLength, jobCard))
		{
			Serial.println("Failed to parse Modbus message.");
			return;
		}
		else
		{
			if(target.type == TargetType::INPUT_POWER)
			{
				inputPowerMeasure = inputPowerDeviceCard.pm;
			}
			else if(target.type == TargetType::OUTPUT_POWER)
			{
				outputPowerMeasure = outputPowerDeviceCard.pm;
			}
		}

		// // Print the updated JobCard values
		// Serial.printf("Updated %s Power Device  of model %s and id %d : Voltage=%.2f V, "
		// 			  "Current=%.3f A, Power=%.2f W, "
		// 			  "Energy=%.2f kWh, Frequency=%.2f Hz, Power Factor=%.3f, Alarms=%.0f\n",
		// 			  (target.type == TargetType::INPUT_POWER ? "Input" : "Output"),
		// 			  Node_Utility::ToString::model(jobCard->info.model), jobCard->info.id,
		// 			  jobCard->pm.voltage, jobCard->pm.current, jobCard->pm.power,
		// 			  jobCard->pm.energy, jobCard->pm.frequency, jobCard->pm.pf,
		// 			  jobCard->pm.alarms);
	}

	void handleError(Error error, uint32_t token)
	{
		ModbusError me(error);
		Serial.printf("Error response: %02X - %s\n", (int)me, (const char*)me);
	}

  public:
	// Singleton access
	static ModbusManager& getInstance(std::unique_ptr<ModbusClientTCP> MClient = nullptr,
									  uint32_t tm = 2000, uint32_t inter = 200)
	{
		static ModbusManager instance(std::move(MClient), tm, inter);
		return instance;
	}

	// Add autopolling targets
	template<typename... Args>
	void autopoll(bool startPolling, Args... targetsToAdd)
	{
		(addTarget(targetsToAdd), ...); // Add all targets
		if(startPolling)
		{
			startPollingTask();
		}
	}
	powerMeasure& getInputPower()
	{
		return inputPowerMeasure;
	}
	powerMeasure& getoutputPower()
	{
		return outputPowerMeasure;
	}

  private:
	// Add a target to the list
	void addTarget(const PollingTarget& target)
	{
		// Check for duplicates
		auto it = std::find_if(_targets.begin(), _targets.end(), [&](const PollingTarget& t) {
			return t.type == target.type && t.token == target.token;
		});

		if(it == _targets.end())
		{
			_targets.push_back(target);
			Serial.printf("Target added: Token=%08X, Type=%d, IP=%s\n", target.token,
						  static_cast<int>(target.type), target.target_ip.toString().c_str());
		}
		else
		{
			Serial.printf("Duplicate target ignored: Token=%08X\n", target.token);
		}
	}

	bool parseModbusMessage(const uint16_t* data, size_t length, JobCard* jobCard)
	{
		size_t index = 0;

		// Ensure minimum length
		if(length < 28)
		{
			Serial.println("Insufficient data for parsing.");
			return false;
		}

		// Endian conversion helpers
		auto toHostEndian16 = [](uint16_t value) -> uint16_t {
			return (value >> 8) | (value << 8);
		};

		auto toHostEndian32 = [](uint32_t value) -> uint32_t {
			return ((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) |
				   ((value & 0x0000FF00) << 8) | ((value & 0x000000FF) << 24);
		};

		auto parseFloat16 = [&](size_t i) -> float {
			uint32_t intBits = (toHostEndian16(data[i]) << 16) | toHostEndian16(data[i + 1]);
			return *reinterpret_cast<float*>(&intBits);
		};

		// Parse NamePlate information
		jobCard->info.model = static_cast<PZEMModel>(toHostEndian16(data[index++]));
		jobCard->info.id = static_cast<uint8_t>(toHostEndian16(data[index++])); // Parse ID
		jobCard->info.slaveAddress = static_cast<uint8_t>(toHostEndian16(data[index++]));
		jobCard->info.lineNo = static_cast<uint8_t>(toHostEndian16(data[index++]));
		jobCard->info.phase = static_cast<Phase>(toHostEndian16(data[index++]));

		// Parse meter name (ASCII-encoded in 16-bit registers)
		std::string meterName;
		for(size_t i = 0; i < 9 && index < length; i += 2)
		{
			char char1 = static_cast<char>(toHostEndian16(data[index]) >> 8); // High byte
			char char2 = static_cast<char>(toHostEndian16(data[index]) & 0xFF); // Low byte
			if(char1 != '\0')
				meterName += char1;
			if(char2 != '\0')
				meterName += char2;
			index++;
		}

		// Parse power measurements
		jobCard->pm.voltage = parseFloat16(index);
		index += 2;
		jobCard->pm.current = parseFloat16(index);
		index += 2;
		jobCard->pm.power = parseFloat16(index);
		index += 2;
		jobCard->pm.energy = parseFloat16(index);
		index += 2;
		jobCard->pm.frequency = parseFloat16(index);
		index += 2;
		jobCard->pm.pf = parseFloat16(index);
		index += 2;

		// Parse 64-bit timestamps
		auto parseInt64 = [&](size_t i) -> int64_t {
			uint32_t high = (toHostEndian16(data[i]) << 16) | toHostEndian16(data[i + 1]);
			uint32_t low = (toHostEndian16(data[i + 2]) << 16) | toHostEndian16(data[i + 3]);
			return (static_cast<int64_t>(high) << 32) | low;
		};

		jobCard->poll_us = parseInt64(index);
		index += 4;
		jobCard->lastUpdate_us = parseInt64(index);
		index += 4;
		jobCard->dataAge_ms = parseInt64(index);
		index += 4;

		// Parse remaining fields
		jobCard->dataStale = toHostEndian16(data[index++]) != 0;
		jobCard->deviceState = static_cast<PZEMState>(toHostEndian16(data[index++]));

		jobCard->pm.isValid = true;
		return true;
	}

	// Prevent copying
	ModbusManager(const ModbusManager&) = delete;
	ModbusManager& operator=(const ModbusManager&) = delete;
};

} // namespace Node_Utility

#endif // PZEM_MODBUS_HPP
