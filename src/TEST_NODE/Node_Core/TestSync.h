#ifndef TEST_SYNC_H
#define TEST_SYNC_H

#include "Arduino.h"
#include "ArduinoJson.h"
#include "Logger.h"
#include "TestData.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <atomic>
#include "StateMachine.h"
#include "NodeConstants.h"
#include <string>

using namespace Node_Core;
extern Logger& logger;
extern StateMachine& stateMachine;
extern EventGroupHandle_t eventGroupTest;
extern EventGroupHandle_t eventGroupUser;

enum class UserCommand : EventBits_t
{
	NEW_TEST = (1 << 0),
	TEST_DELETED = (1 << 1),
	PROCEED = (1 << 2),
	PAUSE = (1 << 3),
	RESUME = (1 << 4),
	AUTO = (1 << 5),
	MANUAL = (1 << 6),
};

class TestSync
{
  public:
	static TestSync& getInstance()
	{
		static TestSync instance;
		return instance;
	}
	void init()
	{
		refreshState();
		eventGroupTest = xEventGroupCreate();
		eventGroupUser = xEventGroupCreate();
		resetAllBits();
		logger.log(LogLevel::INFO, "testSync initialisation");
	}

	void triggerEvent(Event event)
	{
		stateMachine.handleEvent(event);
	}

	const State refreshState()
	{
		_currentState.store(stateMachine.getCurrentState());
		return _currentState.load();
	}

	void parseIncomingJson(JsonObject json)
	{
		// Reset all tests as inactive
		for(int i = 0; i < MAX_TEST; i++)
		{
			_testList[i].isActive = false;
		}

		if(json.containsKey("tests") && json["tests"].is<JsonArray>())
		{
			JsonArray testArray = json["tests"].as<JsonArray>();
			for(JsonVariant value: testArray)
			{
				if(value.is<JsonObject>())
				{
					JsonObject jsonObj = value.as<JsonObject>();

					// Validate and delegate processing to parseTestJson
					if(jsonObj.containsKey("testName") && jsonObj.containsKey("loadLevel"))
					{
						parseTestJson(jsonObj);
					}
					else
					{
						logger.log(LogLevel::ERROR,
								   "Required fields missing in JSON: testName or loadLevel");
					}
				}
			}

			// Check for any deleted tests
			checkForDeletedTests();
		}
		else if(json.containsKey("startCommand"))
		{
			// parseStartJson(json);
		}
		else if(json.containsKey("stopCommand"))
		{
			// parseStopJson(json);
		}
		else
		{
			logger.log(LogLevel::ERROR, "Unknown JSON format");
		}
	}

  private:
	TestSync() : _currentState{State::DEVICE_ON}
	{
	}
	std::atomic<State> _currentState{State::DEVICE_ON};

	static const EventBits_t ALL_TEST_BITS = (1 << MAX_TEST) - 1;
	static const EventBits_t ALL_CMD_BITS = (1 << MAX_USER_COMMAND) - 1;
	RequiredTest _testList[MAX_TEST];

	void resetAllBits()
	{
		xEventGroupClearBits(eventGroupTest, ALL_TEST_BITS);
		xEventGroupClearBits(eventGroupUser, ALL_CMD_BITS);
	}
	void startTest(TestType test)
	{
		EventBits_t test_eventbits = static_cast<EventBits_t>(test);
		xEventGroupSetBits(eventGroupTest, test_eventbits);
		logger.log(LogLevel::WARNING, "test %s will be started", testTypeToString(test));
	};

	void stopTest(TestType test)
	{
		EventBits_t test_eventbits = static_cast<EventBits_t>(test);
		xEventGroupClearBits(eventGroupTest, test_eventbits);
		logger.log(LogLevel::WARNING, "test %s will be stopped", testTypeToString(test));
		vTaskDelay(pdMS_TO_TICKS(50));
	};
	void handleUserCommand(UserCommand command)
	{
		EventBits_t commandBits = static_cast<EventBits_t>(command);
		xEventGroupSetBits(eventGroupTest, commandBits);
		logger.log(LogLevel::INFO, "User command %d triggered", static_cast<int>(command));
	}

	void clearUserCommand(UserCommand command)
	{
		EventBits_t commandBits = static_cast<EventBits_t>(command);
		xEventGroupClearBits(eventGroupTest, commandBits);
		logger.log(LogLevel::INFO, "User command %d cleared", static_cast<int>(command));
	}

	void parseTestJson(JsonObject jsonObj)
	{
		bool isExistingTest = false;

		// Extract testName and loadLevel as const char*
		const char* testName = jsonObj["testName"];
		const char* loadLevel = jsonObj["loadLevel"];

		// Convert to enums using the updated functions
		TestType testType = getTestTypeFromString(testName);
		LoadPercentage loadPercentage = getLoadLevelFromString(loadLevel);

		// Check for invalid enums
		if(testType == static_cast<TestType>(0))
		{
			logger.log(LogLevel::ERROR, "Unknown testName: ", testName);
			return;
		}

		if(loadPercentage == static_cast<LoadPercentage>(-1))
		{
			logger.log(LogLevel::ERROR, "Invalid loadLevel: ", loadLevel);
			return;
		}

		// Check if the test already exists in the list
		for(int i = 0; i < MAX_TEST; i++)
		{
			if(_testList[i].testType == testType)
			{
				isExistingTest = true;
				_testList[i].loadLevel = loadPercentage;
				_testList[i].isActive = true; // Mark as active
				break;
			}
		}

		// Add new test if it doesn't exist
		if(!isExistingTest)
		{
			// Find the first available slot for a new test
			for(int i = 0; i < MAX_TEST; i++)
			{
				if(!_testList[i].isActive)
				{
					_testList[i].testId = i + 1; // Assuming testId is sequential
					_testList[i].testType = testType;
					_testList[i].loadLevel = loadPercentage;
					_testList[i].isActive = true;

					handleUserCommand(UserCommand::NEW_TEST); // Trigger NEW_TEST command
					return; // Exit after adding the new test
				}
			}

			logger.log(LogLevel::WARNING, "Test list is full, unable to add new test.");
		}
	}

	void checkForDeletedTests()
	{
		bool testDeleted = false;

		// Loop through the test list and check for inactive tests
		for(int i = 0; i < MAX_TEST; i++)
		{
			if(!_testList[i].isActive && _testList[i].testId != 0)
			{
				// Clear the test entry
				Serial.printf("Removing Test %d: Type = %s\n", _testList[i].testId,
							  testTypeToString(_testList[i].testType));
				_testList[i] = {}; // Reset the test entry
				testDeleted = true;
			}
		}

		if(testDeleted)
		{
			handleUserCommand(UserCommand::TEST_DELETED);
		}
		else
		{
			clearUserCommand(UserCommand::TEST_DELETED);
		}
	}

	TestSync(const TestSync&) = delete;
	TestSync& operator=(const TestSync&) = delete;
};

#endif // TEST_SYNC_H
