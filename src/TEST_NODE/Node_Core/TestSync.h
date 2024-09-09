#ifndef TEST_SYNC_H
#define TEST_SYNC_H

#include "Arduino.h"
#include "ArduinoJson.h"
#include "Logger.h"
#include "TestData.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <atomic>
#include "StateMachine.h"
#include "NodeConstants.h"
#include <string>
#include "EventHelper.h"
#include <queue>

using namespace Node_Core;
extern Logger& logger;
extern StateMachine& stateMachine;

const EventBits_t ALL_TEST_BITS = (1 << MAX_TEST) - 1;
const EventBits_t ALL_CMD_BITS = (1 << MAX_USER_COMMAND) - 1;

class TestSync
{
  public:
	static TestSync& getInstance();
	void init();

	void reportEvent(Event e);

	void parseIncomingJson(JsonVariant json);
	// void handleUserCommand(UserCommandEvent command);
	void handleUserUpdate(UserUpdateEvent update);
	void handleSyncCommand(SyncCommand command);
	void handleTestEvent(Event e);

	void acknowledgeCMD();
	void acknowledgeCMDReset();
	void enableCurrentTest();
	void disableCurrentTest();
	bool iscmdAcknowledged();
	bool isTestEnabled();

	void RequestStartTest(TestType testType, int testIndex)
	{
		if(testIndex >= 0 && testIndex < MAX_TEST)
			if(_testList[testIndex].testType == testType)
			{
				_currentTestIndex = testIndex;
				startTest(testType);
			}
		logger.log(LogLevel::ERROR, "the test dont exists");
	}

	void RequestStopTest(TestType testType, int testIndex)
	{
		if(testIndex >= 0 && testIndex < MAX_TEST)
			if(_testList[testIndex].testType == testType)
			{
				stopTest(testType);
			}
		logger.log(LogLevel::ERROR, "the test dont exists");
	}
	void UserStopTest()
	{
		TestType testType;
		if(_currentTestIndex >= 0 && _currentTestIndex < MAX_TEST)
		{
			testType = _testList[_currentTestIndex].testType;
			stopTest(testType);
			logger.log(LogLevel::SUCCESS, "USER STOP EXECUTED");
		}
		logger.log(LogLevel::ERROR, "USER STOP FAILED");
	}

	State getState();
	TestMode getMode()
	{
		return stateMachine.isAutoMode() ? TestMode::AUTO : TestMode::MANUAL;
	}

	void updateState(State state)
	{
		_currentState.store(state);
	}
	void updateMode(TestMode mode)
	{
		_deviceMode.store(mode);
	}

  private:
	TestSync();

	bool _cmdAcknowledged = false;
	bool _enableCurrentTest = false;
	bool parsingOngoing = false;

	std::atomic<State> _currentState{State::DEVICE_ON};
	std::atomic<TestMode> _deviceMode{TestMode::MANUAL};

	RequiredTest _testList[MAX_TEST];
	int _testCount = 0;
	int _testID[MAX_TEST];
	int _currentTestIndex = 0;

	std::queue<JsonObject> jsonQueue;

	void resetAllBits();
	void createSynctask();

	static void userCommandTask(void* pvParameters);
	static void userUpdateTask(void* pvParameters);
	static void testSyncTask(void* pvParameters);

	void startTest(TestType test);
	void stopTest(TestType test);
	void stopAllTest();
	void transferTest();

	void parseTestJson(JsonObject jsonObj);
	void processNextJson();
	void checkForDeletedTests();

	TaskHandle_t commandObserverTaskHandle = nullptr;
	TaskHandle_t updateObserverTaskHandle = nullptr;
	TaskHandle_t testObserverTaskHandle = nullptr;

	TestSync(const TestSync&) = delete;
	TestSync& operator=(const TestSync&) = delete;
};

#endif // TEST_SYNC_H
