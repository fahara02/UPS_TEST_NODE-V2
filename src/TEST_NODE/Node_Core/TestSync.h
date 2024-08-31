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
extern EventGroupHandle_t eventGroupSync;

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
	WAIT = (1 << 0),
	ACTIVATE = (1 << 1),
	RE_TEST = (1 << 2),
	SKIP_TEST = (1 << 3),
	SAVE = (1 << 4),
	IGNORE = (1 << 5),
	START_TEST = (1 << 6),
	STOP_TEST = (1 << 7)
};

class TestSync
{
  public:
	static TestSync& getInstance();
	void init();
	void triggerEvent(Event event);
	State refreshState();
	void parseIncomingJson(JsonVariant json);
	void handleUserCommand(UserCommand command);
	void handleSyncCommand(SyncCommand command);

  private:
	TestSync();
	static const EventBits_t ALL_TEST_BITS;
	static const EventBits_t ALL_CMD_BITS;
	std::atomic<State> _currentState;
	RequiredTest _testList[MAX_TEST];

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
