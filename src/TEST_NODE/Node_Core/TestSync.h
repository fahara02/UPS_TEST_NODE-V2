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
	State refreshState();
	void reportEvent(Event e);

	void parseIncomingJson(JsonVariant json);
	void handleUserCommand(UserCommandEvent command);
	void handleUserUpdate(UserUpdateEvent update);
	void handleSyncCommand(SyncCommand command);
	void handleTestEvent(Event e);

	void acknowledgeCMD();
	void acknowledgeCMDReset();
	void enableCurrentTest();
	void disableCurrentTest();
	bool iscmdAcknowledged();
	bool isTestEnabled();

	State getState();
	JsonDocument _testDoc;

  private:
	TestSync();

	bool _cmdAcknowledged = false;
	bool _enableCurrentTest = false;
	bool parsingOngoing = false;

	std::atomic<State> _currentState;

	RequiredTest _testList[MAX_TEST];
	int _testCount = 0;
	int _testID[MAX_TEST];
	std::queue<JsonObject> jsonQueue;

	StateMachine& stateMachine = StateMachine::getInstance();

	void resetAllBits();
	void createSynctask();

	static void userCommandObserverTask(void* pvParameters);
	static void userUpdateObserverTask(void* pvParameters);
	static void testSyncObserverTask(void* pvParameters);

	void startTest(TestType test);
	void stopTest(TestType test);
	void stopAllTest();
	void transferTest();
	void updateMode(TestMode mode);
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
