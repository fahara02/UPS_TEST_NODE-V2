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

using namespace Node_Core;
extern Logger& logger;
extern StateMachine& stateMachine;

const EventBits_t ALL_TEST_BITS = (1 << MAX_TEST) - 1;
const EventBits_t ALL_CMD_BITS = (1 << MAX_USER_COMMAND) - 1;
const EventBits_t ALL_SYNC_BITS = (1 << MAX_SYNC_COMMAND) - 1;

class TestSync
{
  public:
	static TestSync& getInstance();
	void init();
	State refreshState();
	void reportGlobalEvent(Event e);

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
	EventGroupHandle_t getEventGroupTest() const
	{
		return eventGroupTest;
	}
	EventGroupHandle_t getEventGroupUser() const
	{
		return eventGroupUser;
	}
	EventGroupHandle_t getEventGroupSync() const
	{
		return eventGroupSync;
	}

  private:
	TestSync();

	bool _cmdAcknowledged = false;
	bool _enableCurrentTest = false;

	std::atomic<State> _currentState;

	EventGroupHandle_t eventGroupSync = NULL;

	RequiredTest _testList[MAX_TEST];

	StateMachine& stateMachine = StateMachine::getInstance();

	void resetAllBits();
	void createSynctask();

	static void userCommandObserverTask(void* pvParameters);
	static void testSyncTask(void* pvParameters);

	void startTest(TestType test);
	void stopTest(TestType test);
	void stopAllTest();
	void parseTestJson(JsonObject jsonObj);
	void checkForDeletedTests();

	TestSync(const TestSync&) = delete;
	TestSync& operator=(const TestSync&) = delete;
};

#endif // TEST_SYNC_H
