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
	DELETE_TEST = (1 << 1),
	PAUSE = (1 << 2),
	RESUME = (1 << 3),
	AUTO = (1 << 4),
	MANUAL = (1 << 5),
	START = (1 << 6),
	STOP = (1 << 7)
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

	// void parseIncomingJson(JsonVariant json)
	// {
	// 	// Reset all tests as inactive
	// 	for(int i = 0; i < MAX_TEST; i++)
	// 	{
	// 		_testList[i].isActive = false;
	// 	}

	// 	// Check if JSON is an array
	// 	if(json.is<JsonArray>())
	// 	{
	// 		JsonArray testArray = json.as<JsonArray>();
	// 		for(JsonVariant value: testArray)
	// 		{
	// 			if(value.is<JsonObject>())
	// 			{
	// 				JsonObject jsonObj = value.as<JsonObject>();

	// 				// Validate required fields
	// 				if(jsonObj.containsKey("testName") && jsonObj.containsKey("loadLevel"))
	// 				{
	// 					// Process the JSON object
	// 					parseTestJson(jsonObj);
	// 				}
	// 				else
	// 				{
	// 					logger.log(LogLevel::ERROR, "field missing:testName or loadLevel");
	// 				}
	// 			}
	// 			else
	// 			{
	// 				logger.log(LogLevel::ERROR, "Expected JSON objects in array.");
	// 			}
	// 		}

	// 		// Check for any deleted tests
	// 		checkForDeletedTests();
	// 	}
	// 	else if(json.is<JsonObject>())
	// 	{
	// 		JsonObject jsonObj = json.as<JsonObject>();
	// 		if(jsonObj.containsKey("startCommand"))
	// 		{
	// 			// Handle startCommand if needed
	// 			// parseStartJson(jsonObj);
	// 		}
	// 		else if(jsonObj.containsKey("stopCommand"))
	// 		{
	// 			// Handle stopCommand if needed
	// 			// parseStopJson(jsonObj);
	// 		}
	// 		else
	// 		{
	// 			logger.log(LogLevel::ERROR, "Unknown JSON format");
	// 		}
	// 	}
	// 	else
	// 	{
	// 		logger.log(LogLevel::ERROR, "Error: Invalid JSON format.");
	// 	}
	// }

	void parseIncomingJson(JsonVariant json)
	{
		// Reset all tests as inactive
		for(int i = 0; i < MAX_TEST; i++)
		{
			_testList[i].isActive = false;
		}

		// If the incoming JSON is an array (already handled by the lambda function)
		if(json.is<JsonObject>())
		{
			logger.log(LogLevel::TEST, "TEST SYNC FUNCTION RECEIVING THIS");
			serializeJsonPretty(json, Serial);
			JsonObject jsonObj = json.as<JsonObject>();

			// Validate required fields
			if(jsonObj.containsKey("testName") && jsonObj.containsKey("loadLevel"))
			{
				parseTestJson(jsonObj);
			}
			else
			{
				logger.log(LogLevel::ERROR, "Required fields missing : testName or loadLevel");
			}
		}
		else
		{
			logger.log(LogLevel::ERROR, "UNKnown JSON format!!!");
		}
	}

  private:
	TestSync() : _currentState{State::DEVICE_ON}
	{
	}

	static const EventBits_t ALL_TEST_BITS = (1 << MAX_TEST) - 1;
	static const EventBits_t ALL_CMD_BITS = (1 << MAX_USER_COMMAND) - 1;
	std::atomic<State> _currentState{State::DEVICE_ON};
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

	void parseTestJson(JsonObject jsonObj)
	{
		bool isExistingTest = false;
		String testName = jsonObj["testName"];
		String loadLevel = jsonObj["loadLevel"];
		TestType testType = getTestTypeFromString(testName);
		LoadPercentage loadPercentage = getLoadLevelFromString(loadLevel);

		if(testType == static_cast<TestType>(0))
		{
			logger.log(LogLevel::ERROR, "Unknown testName: ", testName);
			return;
		}

		if(loadPercentage ==
		   static_cast<LoadPercentage>(-1)) // Assuming -1 is an invalid LoadPercentage
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
					logger.log(LogLevel::SUCCESS, "Received new test requirement");
					logger.log(LogLevel::INFO,
							   "TestName:", testTypeToString(_testList[i].testType));
					logger.log(LogLevel::INFO, "LoadLevel percent:",
							   loadPercentageToString(_testList[i].loadLevel));

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

		for(int i = 0; i < MAX_TEST; i++)
		{
			if(!_testList[i].isActive && _testList[i].testId != 0)
			{
				// Clear the test entry
				logger.log(LogLevel::WARNING, "Removing test requirement");
				logger.log(LogLevel::INFO, "TestName:", testTypeToString(_testList[i].testType));
				logger.log(LogLevel::INFO,
						   "LoadLevel percent:", loadPercentageToString(_testList[i].loadLevel));
				_testList[i] = {}; // Reset the test entry
				testDeleted = true;
			}
		}

		if(testDeleted)
		{
			handleUserCommand(UserCommand::DELETE_TEST);
		}
	}
	void handleUserCommand(UserCommand command)
	{
		EventBits_t commandBits = static_cast<EventBits_t>(command);

		// Clear opposite commands automatically
		switch(command)
		{
			case UserCommand::NEW_TEST:
				xEventGroupClearBits(eventGroupUser,
									 static_cast<EventBits_t>(UserCommand::DELETE_TEST));
				break;
			case UserCommand::DELETE_TEST:
				xEventGroupClearBits(eventGroupUser,
									 static_cast<EventBits_t>(UserCommand::NEW_TEST));
				break;
			case UserCommand::PAUSE:
				xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::RESUME));
				break;
			case UserCommand::RESUME:
				xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::PAUSE));
				break;
			case UserCommand::AUTO:
				xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::MANUAL));
				break;
			case UserCommand::MANUAL:
				xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::AUTO));
				break;
			case UserCommand::START:
				xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::STOP));
				break;
			case UserCommand::STOP:
				xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::START));
				break;
			default:
				// No need to clear anything for unknown commands
				break;
		}

		// Set the current command
		xEventGroupSetBits(eventGroupUser, commandBits);
		logger.log(LogLevel::INFO, "User command %d triggered", static_cast<int>(command));
	}

	TestSync(const TestSync&) = delete;
	TestSync& operator=(const TestSync&) = delete;
};

#endif // TEST_SYNC_H
