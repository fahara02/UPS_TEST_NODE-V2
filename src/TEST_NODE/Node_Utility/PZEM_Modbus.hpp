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
	#include <cstring>
	#include <cstdint>
	#include <type_traits>
	#include <WiFiClient.h>

namespace Node_Utility
{
enum class TargetType
{
	INPUT_POWER,
	OUTPUT_POWER,
	SWITCH_CONTROL,
	COIL_READ,
	ANY
};
enum class CoilType
{
	UPS_IN = 0,
	LOAD_BANK_1 = 1,
	LOAD_BANK_2 = 2,
	LOAD_BANK_3 = 3,
	TEST_UPDATE = 4
};
enum class CoilState
{
	OFF = 0,
	ON = 1
};
class ModbusManager
{
  public:
	struct Target
	{
		TargetType type = TargetType::ANY;
		IPAddress target_ip = IPAddress(192, 168, 0, 172);
		uint8_t slave_id = 1;
		Modbus::FunctionCode function_code = FunctionCode::ANY_FUNCTION_CODE;
		uint16_t start_address = UPS_IN_COIL_ADDR;
		uint32_t token;
		uint16_t length = 1;
		uint16_t value = 1;
		uint16_t last_written_value = 0xFFFF;
	};
	struct TesterCoil
	{
		CoilType type;
		uint16_t CoilAddress;
		bool CoilValue;
	};
	struct WatchdogRequest
	{
		CoilType type;
		CoilState expectedState;
		uint32_t timeout;
	};
	static constexpr uint16_t MAX_COILS = 5;
	static constexpr uint16_t MAX_RETRIES = 4;
	static constexpr size_t WATCHDOG_QUEUE_SIZE = 10; // Adjust as need
	static constexpr uint16_t UPS_IN_COIL_ADDR = 0;
	static constexpr uint16_t LOAD_BANK_1_COIL_ADDR = 1;
	static constexpr uint16_t LOAD_BANK_2_COIL_ADDR = 2;
	static constexpr uint16_t LOAD_BANK_3_COIL_ADDR = 3;
	static constexpr uint16_t TEST_UPDATE_COIL_ADDR = 4;

  private:
	WiFiClient theClient;
	std::unique_ptr<ModbusClientTCP> MBClient;
	uint32_t _timeout;
	uint32_t _interval;
	std::vector<Target> _targets;
	std::array<TesterCoil, MAX_COILS> _coils;

	TaskHandle_t pollingTaskHandle = NULL;
	TaskHandle_t watchDogTaskHandle = NULL;
	QueueHandle_t coilWatchdogQueue = NULL;
	IPAddress pzemServerIP = IPAddress(192, 168, 0, 160);
	uint8_t coilserver_id = 0;
	uint8_t ipd_id = 0;
	uint8_t opd_id = 0;
	uint32_t _currentToken;
	Node_Core::OutBox inputPower = OutBox();
	Node_Core::OutBox outputPower = OutBox();

	bool allTaskCreated = false;
	bool updateSingleCoil = true;
	bool enablePolling = true;

	// Private constructor for singleton pattern
	ModbusManager(uint32_t tm = 4000, uint32_t inter = 2000,
				  IPAddress serverIP = IPAddress(192, 168, 0, 172), u8_t coilServerId = 1,
				  uint8_t inputPowerId = 1, uint8_t outputPowerId = 2) :
		MBClient(std::make_unique<ModbusClientTCP>(theClient)),
		_timeout(tm), _interval(inter), pzemServerIP(serverIP), coilserver_id(coilServerId),
		ipd_id(inputPowerId), opd_id(outputPowerId), _currentToken(0)
	{
		init();
		configASSERT(createAllTask());
		// Create the queue
		coilWatchdogQueue = xQueueCreate(WATCHDOG_QUEUE_SIZE, sizeof(WatchdogRequest));
		configASSERT(coilWatchdogQueue);
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
			TesterCoil{CoilType::TEST_UPDATE, TEST_UPDATE_COIL_ADDR, false},
		};

		createSwitchControls();
	}
	// Generate a unique token
	uint32_t generateUniqueToken()
	{
		return ++_currentToken;
	}

	// Internal method to start the polling task
	bool createAllTask()
	{
		if(!allTaskCreated)
		{
			if(xTaskCreatePinnedToCore(
				   [](void* pvParameters) {
					   static_cast<ModbusManager*>(pvParameters)->pollingTask();
				   },
				   "ModbusPollingTask",
				   modbus_Stack, // Stack size
				   this, // Task parameter
				   modbus_Priority, // Priority
				   &pollingTaskHandle, // Task handle
				   modbus_CORE))
			{
				if(xTaskCreatePinnedToCore(
					   [](void* param) {
						   static_cast<ModbusManager*>(param)->coilWatchdogTask();
					   },
					   "CoilWatchdogTask",
					   modbus_Stack, // Stack size
					   this, // Parameter
					   modbus_Priority, // Priority
					   &watchDogTaskHandle, // Task handle
					   modbus_CORE // Core ID
					   ))
				{
					allTaskCreated = true;
				}
			}
		}

		return allTaskCreated;
	}

	void startPollingTask()
	{
		enablePolling = true;
	}
	void stopPollingTask()
	{
		enablePolling = false;
	}
	// Polling task implementation
	void pollingTask()
	{
		while(true)
		{
			if(enablePolling)
			{
				for(const auto& target: _targets)
				{
					if(target.type == TargetType::INPUT_POWER ||
					   target.type == TargetType::OUTPUT_POWER ||
					   target.type == TargetType::COIL_READ)
					{
						MBClient->setTarget(target.target_ip, 502, _timeout, _interval);

						Modbus::Error modbusError = MBClient->addRequest(
							target.token, target.slave_id, target.function_code,
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
			}

			vTaskDelay(pdMS_TO_TICKS(1000)); // Delay before the next polling cycle
		}
	}
	void coilWatchdogTask()
	{
		WatchdogRequest request;

		while(true)
		{
			// Wait for a new request
			if(xQueueReceive(coilWatchdogQueue, &request, portMAX_DELAY) == pdTRUE)
			{
				uint16_t retries = 0;
				bool matchFound = false;

				while(retries <= MAX_RETRIES)
				{
					// Check the current state of the coil
					auto coilIt =
						std::find_if(_coils.begin(), _coils.end(), [&](const TesterCoil& coil) {
							return coil.type == request.type;
						});

					if(coilIt != _coils.end())
					{
						// Check if the coil's state matches the expected state
						bool expectedValue = (request.expectedState == CoilState::ON);
						if(coilIt->CoilValue == expectedValue)
						{
							Serial.printf("Watchdog: Coil %s successfully updated to %s\n",
										  getCoilTypeName(request.type).c_str(),
										  expectedValue ? "ON" : "OFF");
							matchFound = true;
							break; // Exit retries loop after success
						}
					}

					// If not matched, invoke `readCoil`
					Modbus::Error error = readCoil(coilserver_id, coilIt->CoilAddress);
					if(error != Modbus::SUCCESS)
					{
						Serial.printf("Watchdog: Failed to read coil %s: Error %02X\n",
									  getCoilTypeName(request.type).c_str(),
									  static_cast<int>(error));
					}

					// Increment retries and delay
					retries++;
					vTaskDelay(pdMS_TO_TICKS(500)); // Adjust delay as needed
				}

				if(!matchFound)
				{
					Serial.printf("Watchdog: Timeout or retries exhausted for coil %s\n",
								  getCoilTypeName(request.type).c_str());
				}

				// Reset state and wait for the next request
			}
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
				if(!validateAndProcessPowerData(response, &inputPower))
				{
					Serial.println("Failed to process INPUT_POWER data.");
				}
				break;
			}

			case TargetType::OUTPUT_POWER:
			{
				if(!validateAndProcessPowerData(response, &outputPower))
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
	static ModbusManager& getInstance(uint32_t tm = 4000, uint32_t inter = 2000)
	{
		static ModbusManager instance(tm, inter);
		return instance;
	}

	// Base case (end of recursion)
	void processTarget(bool startPolling)
	{
	}

	// Recursive case
	template<typename First, typename... Rest>
	void processTarget(bool startPolling, First&& first, Rest&&... rest)
	{
		if(startPolling)
			addTarget(std::forward<First>(first));
		else
			removeTarget(std::forward<First>(first));

		processTarget(startPolling, std::forward<Rest>(rest)...); // Recurse for remaining targets
	}

	template<typename... Args>
	void autopoll(bool startPolling, Args&&... targetsToAdd)
	{
		processTarget(startPolling, std::forward<Args>(targetsToAdd)...);

		if(startPolling)
			startPollingTask();
		else
			stopPollingTask();
	}

	OutBox& getInputPower()
	{
		return inputPower;
	}
	OutBox& getoutputPower()
	{
		return outputPower;
	}

	void TriggerCoil(CoilType type, CoilState state)
	{
		// Find the coil object
		auto coilIt = std::find_if(_coils.begin(), _coils.end(), [&](const TesterCoil& coil) {
			return coil.type == type;
		});

		if(coilIt == _coils.end())
		{
			Serial.printf("Invalid Coil Type: %d\n", static_cast<int>(type));
			return;
		}

		// Set the coil value
		uint8_t serverID = coilserver_id;
		uint16_t address = coilIt->CoilAddress;
		bool value = (state == CoilState::ON);

		Modbus::Error error = setCoil(serverID, address, value);

		if(error != Modbus::SUCCESS)
		{
			Serial.printf("Failed to set coil %s: Error %02X\n", getCoilTypeName(type).c_str(),
						  static_cast<int>(error));
			return;
		}

		// Prepare the watchdog request
		WatchdogRequest request = {type, state, 5000}; // 5-second timeout

		// Send the request to the queue
		if(xQueueSend(coilWatchdogQueue, &request, pdMS_TO_TICKS(100)) != pdTRUE)
		{
			Serial.println("Failed to enqueue watchdog request.");
		}
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
	void removeTarget(const Target& target)
	{
		// Find the target using the same criteria as addTarget
		auto it = std::find_if(_targets.begin(), _targets.end(), [&](const Target& t) {
			return t.type == target.type && t.token == target.token;
		});

		if(it != _targets.end())
		{
			_targets.erase(it);
			Serial.printf("Target removed: Token=%08X, Type=%d, IP=%s\n", target.token,
						  static_cast<int>(target.type), target.target_ip.toString().c_str());
		}
		else
		{
			Serial.printf("Target not found for removal: Token=%08X\n", target.token);
		}
	}

	//
	void createSwitchControls()
	{
		// All coil targets
		Target ups_in_trigger = {
			TargetType::SWITCH_CONTROL, // type
			pzemServerIP, // target_ip
			coilserver_id, // slave_id
			Modbus::FunctionCode::WRITE_COIL, // function_code
			ModbusManager::UPS_IN_COIL_ADDR, // start_address
			generateUniqueToken(), // token
			1, // length
			1, // value
			0xFFFF // last_written_value
		};

		Target load_bank_1_trigger = {
			TargetType::SWITCH_CONTROL, // type
			pzemServerIP, // target_ip
			coilserver_id, // slave_id
			Modbus::FunctionCode::WRITE_COIL, // function_code
			ModbusManager::LOAD_BANK_1_COIL_ADDR, // start_address
			generateUniqueToken(), // token
			1, // length
			1, // value
			0xFFFF // last_written_value
		};

		Target load_bank_2_trigger = {
			TargetType::SWITCH_CONTROL, // type
			pzemServerIP, // target_ip
			3, // slave_id
			Modbus::FunctionCode::WRITE_COIL, // function_code
			ModbusManager::LOAD_BANK_2_COIL_ADDR, // start_address
			generateUniqueToken(), // token
			1, // length
			1, // value
			0xFFFF // last_written_value
		};

		Target load_bank_3_trigger = {
			TargetType::SWITCH_CONTROL, // type
			pzemServerIP, // target_ip
			3, // slave_id
			Modbus::FunctionCode::WRITE_COIL, // function_code
			ModbusManager::LOAD_BANK_3_COIL_ADDR, // start_address
			generateUniqueToken(), // token
			1, // length
			1, // value
			0xFFFF // last_written_value
		};

		Target test_update_trigger = {
			TargetType::SWITCH_CONTROL, // type
			pzemServerIP, // target_ip
			3, // slave_id
			Modbus::FunctionCode::WRITE_COIL, // function_code
			ModbusManager::TEST_UPDATE_COIL_ADDR, // start_address
			generateUniqueToken(), // token
			1, // length
			1, // value
			0xFFFF // last_written_value
		};
		addTarget(ups_in_trigger);
		addTarget(load_bank_1_trigger);
		addTarget(load_bank_2_trigger);
		addTarget(load_bank_3_trigger);
		addTarget(test_update_trigger);
	}

	Modbus::Error setCoil(uint8_t serverID, uint16_t address, bool value)
	{
		uint16_t new_value = static_cast<uint16_t>(value ? 0xFF00 : 0x0000);

		// Check if the value is already set to avoid unnecessary Modbus writes
		for(auto& target: _targets)
		{
			if(target.start_address == address && target.slave_id == serverID)
			{
				if(target.last_written_value == new_value)
				{
					// No need to write, the value hasn't changed
					return Modbus::SUCCESS;
				}
				target.last_written_value = new_value; // Update last written value
				break;
			}
		}

		Target target_write = {TargetType::SWITCH_CONTROL,
							   pzemServerIP,
							   serverID,
							   Modbus::FunctionCode::WRITE_COIL,
							   address,
							   generateUniqueToken(),
							   1,
							   new_value};

		Modbus::Error modbusError = Modbus::Error::UNDEFINED_ERROR;

		// Retry logic for Modbus request
		for(int attempt = 0; attempt < MAX_RETRIES; ++attempt)
		{
			// Create a new Modbus message for each attempt
			ModbusMessage write_request;
			modbusError =
				write_request.setMessage(target_write.slave_id, target_write.function_code,
										 target_write.start_address, target_write.value);

			if(modbusError != Modbus::SUCCESS)
			{
				// Log this error if necessary
				continue; // Retry the message creation
			}

			// Send the request using MBClient->addRequest
			modbusError = MBClient->addRequest(write_request, target_write.token);

			if(modbusError == Modbus::SUCCESS)
			{
				return Modbus::SUCCESS; // Successful request, no further retries needed
			}

			// Small delay before retry (adjust if needed)
			vTaskDelay(pdMS_TO_TICKS(100));
		}

		return modbusError; // Return the last error after all retries failed
	}
	Modbus::Error readCoil(uint8_t serverID, uint16_t address)
	{
		Target target_read = {TargetType::COIL_READ,		   pzemServerIP, serverID,
							  Modbus::FunctionCode::READ_COIL, address,		 generateUniqueToken()};
		addTarget(target_read);
		Modbus::Error modbusError = Modbus::Error::UNDEFINED_ERROR;

		// Retry logic for Modbus request
		for(int attempt = 0; attempt < MAX_RETRIES; ++attempt)
		{
			// Create a new Modbus message for each attempt
			ModbusMessage read_request;
			modbusError = read_request.setMessage(target_read.slave_id, target_read.function_code,
												  target_read.start_address, target_read.value);

			if(modbusError != Modbus::SUCCESS)
			{
				// Log this error if necessary
				continue; // Retry the message creation
			}

			// Send the request using MBClient->addRequest
			modbusError = MBClient->addRequest(read_request, target_read.token);

			if(modbusError == Modbus::SUCCESS)
			{
				return Modbus::SUCCESS; // Successful request, no further retries needed
			}

			// Small delay before retry (adjust if needed)
			vTaskDelay(pdMS_TO_TICKS(100));
		}
		return modbusError;
	}

	// Helper method to validate and process power data
	bool validateAndProcessPowerData(ModbusMessage& response, OutBox* outbox)
	{
		if(!validateExtractPowerData(response, outbox))
		{
			return false;
		}

		// Update power measure based on processed data
		if(outbox->type == MeasureType::INPUT_POWER)
		{
		}
		else if(outbox->type == MeasureType::OUTPUT_POWER)
		{
		}

		return true;
	}
	bool validateExtractPowerData(ModbusMessage& response, OutBox* outbox)
	{
		if(response.size() < 20)
		{
			Serial.println("Response size is insufficient for parsing.");
			return false;
		}
		const uint16_t* message = reinterpret_cast<const uint16_t*>(response.data() + 3);
		size_t messageLength = (response.size() - 3) / 2;

		return parseOutBox(message, messageLength, outbox);
	}
	uint16_t toHostEndian16(uint16_t value)
	{
		return (value >> 8) | (value << 8);
	}

	// Helper template to trigger static_assert for unsupported types
	template<typename T>
	struct UnsupportedTypeForReadData
	{
		static constexpr bool value = false;
	};

	// Generic template function to read data
	template<typename T>
	T readData(const uint16_t* data, size_t& index)
	{
		static_assert(std::is_enum<T>::value || std::is_integral<T>::value ||
						  std::is_floating_point<T>::value || UnsupportedTypeForReadData<T>::value,
					  "Unsupported type for readData");

		auto toHostEndian16 = [](uint16_t value) {
			return static_cast<uint16_t>((value >> 8) | (value << 8));
		};

		if(std::is_enum<T>::value)
		{
			return static_cast<T>(toHostEndian16(data[index++]));
		}
		else if(std::is_integral<T>::value && sizeof(T) == sizeof(int64_t))
		{
			uint32_t high = (toHostEndian16(data[index]) << 16) | toHostEndian16(data[index + 1]);
			uint32_t low =
				(toHostEndian16(data[index + 2]) << 16) | toHostEndian16(data[index + 3]);
			index += 4;
			return static_cast<T>((static_cast<int64_t>(high) << 32) | low);
		}
		else if(std::is_floating_point<T>::value)
		{
			uint32_t intBits =
				(toHostEndian16(data[index]) << 16) | toHostEndian16(data[index + 1]);
			index += 2;
			return *reinterpret_cast<T*>(&intBits);
		}
		else if(std::is_integral<T>::value)
		{
			return static_cast<T>(toHostEndian16(data[index++]));
		}

		// static_assert(UnsupportedTypeForReadData<T>::value, "Unsupported type for readData");
	}
	bool parseOutBox(const uint16_t* data, size_t length, OutBox* outbox)
	{
		if(length < 15)
		{
			Serial.println("Insufficient data for parsing.");
			return false;
		}

		size_t index = 0;

		outbox->device_id = readData<uint16_t>(data, index);
		outbox->type = readData<MeasureType>(data, index);
		outbox->voltage = readData<float>(data, index);
		outbox->current = readData<float>(data, index);
		outbox->power = readData<float>(data, index);
		outbox->energy = readData<float>(data, index);
		outbox->powerfactor = readData<float>(data, index);
		outbox->frequency = readData<float>(data, index);
		outbox->isValid = readData<boolean>(data, index);

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
			case CoilType::TEST_UPDATE:
				return "TEST_UPDATE";
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

// bool parseJobCardData(const uint16_t* data, size_t length, JobCard* jobCard)
// {
// 	if(length < 28)
// 	{
// 		Serial.println("Insufficient data for parsing.");
// 		return false;
// 	}

// 	size_t index = 0;

// 	// Parse NamePlate information
// 	jobCard->info.model = readData<PZEMModel>(data, index);
// 	jobCard->info.id = readData<uint8_t>(data, index);
// 	jobCard->info.slaveAddress = readData<uint8_t>(data, index);
// 	jobCard->info.lineNo = readData<uint8_t>(data, index);
// 	jobCard->info.phase = readData<Phase>(data, index);

// 	// Parse meter name (ASCII-encoded in 16-bit registers)
// 	std::array<char, 9> meterName = {0};
// 	for(size_t i = 0; i < 9 && index < length; ++i)
// 	{
// 		uint16_t word = readData<uint16_t>(data, index);
// 		char highByte = static_cast<char>(word >> 8);
// 		char lowByte = static_cast<char>(word & 0xFF);
// 		if(i < meterName.size())
// 		{
// 			meterName[i++] = highByte;
// 		}
// 		if(i < meterName.size())
// 		{
// 			meterName[i++] = lowByte;
// 		}
// 	}
// 	jobCard->info.meterName = meterName;

// 	// Parse power measurements
// 	auto& pm = jobCard->pm;
// 	pm.voltage = readData<float>(data, index);
// 	pm.current = readData<float>(data, index);
// 	pm.power = readData<float>(data, index);
// 	pm.energy = readData<float>(data, index);
// 	pm.frequency = readData<float>(data, index);
// 	pm.pf = readData<float>(data, index);

// 	// Parse timestamps
// 	jobCard->poll_us = readData<int64_t>(data, index);
// 	jobCard->lastUpdate_us = readData<int64_t>(data, index);
// 	jobCard->dataAge_ms = readData<int64_t>(data, index);

// 	// Parse remaining fields
// 	jobCard->dataStale = readData<uint16_t>(data, index) != 0;
// 	jobCard->deviceState = readData<PZEMState>(data, index);

// 	pm.isValid = true;
// 	return true;
// }