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
enum class CoilType
{
	UPS_IN = 0,
	LOAD_BANK_1 = 1,
	LOAD_BANK_2 = 2,
	LOAD_BANK_3 = 3
};

class ModbusManager
{
  public:
	struct Target
	{
		TargetType type = TargetType::ANY;
		uint32_t token;
		IPAddress target_ip = IPAddress(192, 168, 0, 160);
		uint8_t slave_id = 1;
		Modbus::FunctionCode function_code = FunctionCode::ANY_FUNCTION_CODE;
		uint16_t start_address = UPS_IN_COIL_ADDR;
		uint16_t length = 1;
	};
	struct TesterCoil
	{
		CoilType type;
		uint16_t CoilAddress;
		bool CoilValue;
	};
	static constexpr uint16_t MAX_COILS = 4;
	static constexpr uint16_t UPS_IN_COIL_ADDR = 1000;
	static constexpr uint16_t LOAD_BANK_1_COIL_ADDR = 1001;
	static constexpr uint16_t LOAD_BANK_2_COIL_ADDR = 1002;
	static constexpr uint16_t LOAD_BANK_3_COIL_ADDR = 1003;

  private:
	std::unique_ptr<ModbusClientTCP> MBClient;
	uint32_t _timeout;
	uint32_t _interval;
	std::vector<Target> _targets;
	std::array<TesterCoil, MAX_COILS> _coils;
	uint32_t _currentToken;
	TaskHandle_t modbusTaskHandle = NULL;
	uint8_t inputPowerDevice_SlaveId = 0;
	uint8_t outputPowerDevice_SlaveId = 0;
	Node_Core::JobCard inputPowerDeviceCard = JobCard();
	Node_Core::JobCard outputPowerDeviceCard = JobCard();
	Node_Core::powerMeasure inputPowerMeasure = powerMeasure();
	Node_Core::powerMeasure outputPowerMeasure = powerMeasure();
	bool updateSingleCoil = true;

	// Private constructor for singleton pattern
	ModbusManager(std::unique_ptr<ModbusClientTCP> MClient, uint32_t tm = 4000,
				  uint32_t inter = 2000) :
		MBClient(std::move(MClient)),
		_timeout(tm), _interval(inter), _currentToken(0)
	{
		ModbusManager::getInstance().init();
		MBClient->begin(); // Start ModbusTCP background task
	}
	void init()
	{
		MBClient->setTimeout(_timeout, _interval);
		MBClient->onDataHandler([this](ModbusMessage response, uint32_t token) {
			this->handleData(response, token);
		});
		MBClient->onErrorHandler([this](Error error, uint32_t token) {
			this->handleError(error, token);
		});
		_coils = {
			TesterCoil{CoilType::UPS_IN, UPS_IN_COIL_ADDR, false},
			TesterCoil{CoilType::LOAD_BANK_1, LOAD_BANK_1_COIL_ADDR, false},
			TesterCoil{CoilType::LOAD_BANK_2, LOAD_BANK_2_COIL_ADDR, false},
			TesterCoil{CoilType::LOAD_BANK_3, LOAD_BANK_3_COIL_ADDR, false},
		};
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
				if(target.type == TargetType::INPUT_POWER ||
				   target.type == TargetType::OUTPUT_POWER)
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
			}

			vTaskDelay(pdMS_TO_TICKS(1000)); // Delay before the next polling cycle
		}
	}

	// Data and error handlers
	void handleData(ModbusMessage response, uint32_t token)
	{
		// Debug logging for full response details
		Serial.printf("Response: ServerID=%d, FC=%d, Token=%08X, Length=%d\n",
					  response.getServerID(), response.getFunctionCode(), token, response.size());

		// Hex dump of response (optional, can be commented out in production)
		for(auto& byte: response)
		{
			Serial.printf("%02X ", byte);
		}
		Serial.println();

		// Find the matching target by token
		auto targetIt = std::find_if(_targets.begin(), _targets.end(), [&](const Target& t) {
			return t.token == token;
		});

		if(targetIt == _targets.end())
		{
			Serial.printf("Unknown target token %08X, ignoring response.\n", token);
			return;
		}
		Target& target = *targetIt;

		// Process based on target type with improved error handling
		switch(target.type)
		{
			case TargetType::INPUT_POWER:
			{
				if(!validateAndProcessPowerData(response, &inputPowerDeviceCard))
				{
					Serial.println("Failed to process INPUT_POWER data.");
				}
				break;
			}

			case TargetType::OUTPUT_POWER:
			{
				if(!validateAndProcessPowerData(response, &outputPowerDeviceCard))
				{
					Serial.println("Failed to process OUTPUT_POWER data.");
				}
				break;
			}

			case TargetType::SWITCH_CONTROL:
			{
				processCoilSwitchResponse(response, target);
				break;
			}

			default:
			{
				Serial.printf("Unhandled target type: %d\n", static_cast<int>(target.type));
				break;
			}
		}
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
	Modbus::Error TriggerCoil(Target target, CoilType type)
	{
		if(target.type != TargetType::SWITCH_CONTROL)
		{
			return Modbus::Error::FC_MISMATCH; // Ensure the target is for switch control
		}

		// Ensure unique token for each request
		target.token = generateUniqueToken();
		ModbusManager::getInstance().addTarget(target);
		MBClient->setTarget(target.target_ip, 502, _timeout, _interval);

		uint16_t coilAddress = 0;

		switch(type)
		{
			case CoilType::UPS_IN:
				coilAddress = UPS_IN_COIL_ADDR;
				break;
			case CoilType::LOAD_BANK_1:
				coilAddress = LOAD_BANK_1_COIL_ADDR;
				break;
			case CoilType::LOAD_BANK_2:
				coilAddress = LOAD_BANK_2_COIL_ADDR;
				break;
			case CoilType::LOAD_BANK_3:
				coilAddress = LOAD_BANK_3_COIL_ADDR;
				break;
			default:
				return Modbus::Error::ILLEGAL_DATA_VALUE; // Handle invalid type
		}

		// Send the Modbus request
		Modbus::Error err = MBClient->addRequest(target.token, target.slave_id,
												 FunctionCode::WRITE_COIL, coilAddress, 1);

		if(err != Modbus::SUCCESS)
		{
			ModbusError e(err);
			Serial.printf("Failed to activate switch: %02X - %s\n", (int)e, (const char*)e);
		}

		return err;
	}

  private:
	// Add a target to the list
	void addTarget(const Target& target)
	{
		// Check for duplicates
		auto it = std::find_if(_targets.begin(), _targets.end(), [&](const Target& t) {
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
	bool validateExtractPowerData(ModbusMessage& response, JobCard* jobCard)
	{
		if(response.size() < 20)
		{
			Serial.println("Response size is insufficient for parsing.");
			return false;
		}
		const uint16_t* message = reinterpret_cast<const uint16_t*>(response.data() + 3);
		size_t messageLength = (response.size() - 3) / 2;

		return parseJobCardData(message, messageLength, jobCard);
	}
	bool parseJobCardData(const uint16_t* data, size_t length, JobCard* jobCard)
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
	// Helper method to validate and process power data
	bool validateAndProcessPowerData(ModbusMessage& response, JobCard* jobCard)
	{
		if(!validateExtractPowerData(response, jobCard))
		{
			return false;
		}

		// Update power measure based on processed data
		if(jobCard == &inputPowerDeviceCard)
		{
			inputPowerMeasure = jobCard->pm;
		}
		else if(jobCard == &outputPowerDeviceCard)
		{
			outputPowerMeasure = jobCard->pm;
		}

		return true;
	}

	// method to process coil switch responses for single and multiple coils
	void processCoilSwitchResponse(ModbusMessage& response, Target& target)
	{
		uint16_t startCoilAddress = 0, numberOfCoils = 0;

		// Determine coil update type and validate the response
		if(!validateExtractCoilData(response, startCoilAddress, numberOfCoils))
			return;

		// Update coils based on the extracted address and value(s)
		updateCoils(response, startCoilAddress, numberOfCoils);
	}
	bool validateExtractCoilData(ModbusMessage& response, uint16_t& startCoilAddress,
								 uint16_t& numberOfCoils)
	{
		// Set coil update type based on function code
		if(response.getFunctionCode() == 0x05) // Write Single Coil
		{
			updateSingleCoil = true;
			if(response.size() < 6)
			{
				Serial.println("Invalid Write Single Coil response size.");
				return false;
			}
			startCoilAddress = (response[2] << 8) | response[3];
			numberOfCoils = 1; // Single coil
		}
		else if(response.getFunctionCode() == 0x0F) // Write Multiple Coils
		{
			updateSingleCoil = false;
			if(response.size() < 6)
			{
				Serial.println("Invalid Write Multiple Coils response size.");
				return false;
			}
			startCoilAddress = (response[2] << 8) | response[3];
			numberOfCoils = (response[4] << 8) | response[5];
			if(numberOfCoils > MAX_COILS)
			{
				Serial.println("maximum coil limit exceeded.");
				return false;
			}
		}
		else
		{
			Serial.printf("Unexpected function code %02X for SWITCH_CONTROL\n",
						  response.getFunctionCode());
			return false;
		}
		return true;
	}

	// Update the status of coils in the response
	void updateCoils(ModbusMessage& response, uint16_t startCoilAddress, uint16_t numberOfCoils)
	{
		// Iterate through the coils and update their status
		for(uint16_t i = 0; i < numberOfCoils; ++i)
		{
			uint16_t coilAddress = startCoilAddress + i;
			bool isCoilOn = getCoilValue(response, i);

			// Find and update the corresponding coil
			auto coilIt = std::find_if(_coils.begin(), _coils.end(), [&](const TesterCoil& coil) {
				return coil.CoilAddress == coilAddress;
			});

			if(coilIt != _coils.end())
			{
				coilIt->CoilValue = isCoilOn;
				Serial.printf("Updated %s (Address=0x%04X) to %s\n",
							  getCoilTypeName(coilIt->type).c_str(), coilAddress,
							  isCoilOn ? "ON" : "OFF");
			}
			else
			{
				Serial.printf("Unknown coil address: 0x%04X\n", coilAddress);
			}
		}
	}

	// Extract the coil value for a specific index in the response
	bool getCoilValue(ModbusMessage& response, uint16_t index)
	{
		if(updateSingleCoil)
		{
			return (response[4] << 8 | response[5]) != 0;
		}
		else
		{
			return (response[6 + (index / 8)] & (1 << (index % 8))) != 0; // Write Multiple Coils
		}
	}
	std::string getCoilTypeName(CoilType type)
	{
		switch(type)
		{
			case CoilType::UPS_IN:
				return "UPS_IN";
			case CoilType::LOAD_BANK_1:
				return "LOAD_BANK_1";
			case CoilType::LOAD_BANK_2:
				return "LOAD_BANK_2";
			case CoilType::LOAD_BANK_3:
				return "LOAD_BANK_3";
			default:
				return "UNKNOWN";
		}
	}
	// Prevent copying
	ModbusManager(const ModbusManager&) = delete;
	ModbusManager& operator=(const ModbusManager&) = delete;
};

} // namespace Node_Utility

#endif // PZEM_MODBUS_HPP

// // Print the updated JobCard values
// Serial.printf("Updated %s Power Device  of model %s and id %d : Voltage=%.2f V, "
// 			  "Current=%.3f A, Power=%.2f W, "
// 			  "Energy=%.2f kWh, Frequency=%.2f Hz, Power Factor=%.3f, Alarms=%.0f\n",
// 			  (target.type == TargetType::INPUT_POWER ? "Input" : "Output"),
// 			  Node_Utility::ToString::model(jobCard->info.model), jobCard->info.id,
// 			  jobCard->pm.voltage, jobCard->pm.current, jobCard->pm.power,
// 			  jobCard->pm.energy, jobCard->pm.frequency, jobCard->pm.pf,
// 			  jobCard->pm.alarms);