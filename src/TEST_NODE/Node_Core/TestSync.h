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
// extern EventGroupHandle_t eventGroupTest;
// extern EventGroupHandle_t eventGroupUser;
// extern EventGroupHandle_t eventGroupSync;

const EventBits_t ALL_TEST_BITS = (1 << MAX_TEST) - 1;
const EventBits_t ALL_CMD_BITS = (1 << MAX_USER_COMMAND) - 1;
const EventBits_t ALL_SYNC_BITS = (1 << MAX_SYNC_COMMAND) - 1;

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

enum class SyncCommand : EventBits_t
{
	MANAGER_WAIT = (1 << 0),
	MANAGER_ACTIVE = (1 << 1),
	RE_TEST = (1 << 2),
	SKIP_TEST = (1 << 3),
	SAVE = (1 << 4),
	IGNORE = (1 << 5),
	START_OBSERVER = (1 << 6),
	STOP_OBSERVER = (1 << 7)
};

class TestSync
{
  public:
	static TestSync& getInstance();
	void init();
	State refreshState();
	void reportEvent(Event event);

	void parseIncomingJson(JsonVariant json);
	void handleUserCommand(UserCommand command);
	void handleSyncCommand(SyncCommand command);
	void handlelocalEvent(Event event);

	void acknowledgeCMD();
	void acknowledgeCMDReset();
	void enableCurrentTest();
	void diableCurrentTest();
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

	EventGroupHandle_t eventGroupTest = NULL;
	EventGroupHandle_t eventGroupUser = NULL;
	EventGroupHandle_t eventGroupSync = NULL;

	RequiredTest _testList[MAX_TEST];

	StateMachine& stateMachine = StateMachine::getInstance();

	void resetAllBits();
	void createSynctask();

	static void userCommandObserverTask(void* pvParameters);
	static void testSyncTask(void* pvParameters);

	void startTest(TestType test);
	void stopTest(TestType test);
	void parseTestJson(JsonObject jsonObj);
	void checkForDeletedTests();

	TestSync(const TestSync&) = delete;
	TestSync& operator=(const TestSync&) = delete;
};

#endif // TEST_SYNC_H
