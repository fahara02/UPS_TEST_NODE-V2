#ifndef TEST_SYNC_H
#define TEST_SYNC_H

#include <atomic>
#include <queue>
#include <set>
#include <string>
#include "Arduino.h"
#include "ArduinoJson.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include "Logger.h"
#include "TestData.h"
#include "StateMachine.h"
#include "NodeConstants.h"
#include "EventHelper.h"

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

	void handleUserUpdate(UserUpdateEvent update);
	void handleSyncCommand(SyncCommand command);

	void acknowledgeCMD();
	void acknowledgeCMDReset();
	void enableCurrentTest();
	void disableCurrentTest();
	bool iscmdAcknowledged();
	bool isTestEnabled();

	void RequestStartTest(TestType testType, int testIndex);
	void RequestStopTest(TestType testType, int testIndex);
	void UserStopTest();

	State getState();
	TestMode getMode();
	void updateState(State state);
	void updateMode(TestMode mode);
	TaskHandle_t testObserverTaskHandle = nullptr;

  private:
	TestSync();

	bool _cmdAcknowledged = false;
	bool _enableCurrentTest = false;
	bool parsingOngoing = false;

	std::atomic<State> _currentState{State::DEVICE_ON};
	std::atomic<TestMode> _deviceMode{TestMode::MANUAL};

	std::pair<String, String> uniqueTests[MAX_UNIQUE_TESTS];
	RequiredTest _testList[MAX_TEST];

	int uniqueTestCount = 0;
	int _testCount = 0;
	int _testID[MAX_TEST];
	int _currentTestIndex = 0;

	SwitchTestData _swData;
	BackupTestData _btData;

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

	bool isTestUnique(const String& testName, const String& loadLevel);
	void addUniqueTest(const String& testName, const String& loadLevel);
	void removeTest(const String& testName, const String& loadLevel);

	TaskHandle_t commandObserverTaskHandle = nullptr;
	TaskHandle_t updateObserverTaskHandle = nullptr;

	TestSync(const TestSync&) = delete;
	TestSync& operator=(const TestSync&) = delete;
};

#endif // TEST_SYNC_H
