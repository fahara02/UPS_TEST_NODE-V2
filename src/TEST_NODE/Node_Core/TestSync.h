#ifndef TEST_SYNC_H
#define TEST_SYNC_H

#include "Arduino.h"
#include "Logger.h"
#include "TestData.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <atomic>
#include "StateMachine.h"
#include "NodeConstants.h"

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

	void triggerEvent(Event event)
	{
		stateMachine.handleEvent(event);
	}

	const State refreshState()
	{
		_currentState.store(stateMachine.getCurrentState());
		return _currentState.load();
	}

  private:
	TestSync() : _currentState{State::DEVICE_ON}
	{
	}
	std::atomic<State> _currentState{State::DEVICE_ON};

	static const EventBits_t ALL_TEST_BITS = (1 << MAX_TEST) - 1;
	static const EventBits_t ALL_CMD_BITS = (1 << MAX_USER_COMMAND) - 1;
	void resetAllBits()
	{
		xEventGroupClearBits(eventGroupTest, ALL_TEST_BITS);
		xEventGroupClearBits(eventGroupUser, ALL_CMD_BITS);
	}
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
	TestSync(const TestSync&) = delete;
	TestSync& operator=(const TestSync&) = delete;
};

#endif // TEST_SYNC_H
