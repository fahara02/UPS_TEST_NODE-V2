#ifndef EVENT_HELPER_H
#define EVENT_HELPER_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "StateDefines.h"
#include "TestData.h"

namespace Node_Core
{

enum class SystemEvent : EventBits_t
{
	NONE = 1 << 0,
	ERROR = 1 << 1,
	SYSTEM_FAULT = 1 << 2,
	FAULT_CLEARED = 1 << 3,
	NETWORK_DISCONNECTED = 1 << 4,
	RESTART = 1 << 5
};

enum class SystemInitEvent : EventBits_t
{
	SETTING_LOADED = 1 << 0,
	SELF_CHECK_OK = 1 << 1,
	LOAD_BANK_CHECKED = 1 << 2
};

enum class TestEvent : EventBits_t
{
	TEST_ONGOING = 1 << 0,
	TEST_TIME_END = 1 << 1,
	DATA_CAPTURED = 1 << 2,
	VALID_DATA = 1 << 3,
	TEST_FAILED = 1 << 4,
	RETEST = 1 << 5,
	TEST_LIST_EMPTY = 1 << 6,
	PENDING_TEST_FOUND = 1 << 7
};

enum class UserCommandEvent : EventBits_t
{
	START = 1 << 0,
	STOP = 1 << 1,
	AUTO = 1 << 2,
	MANUAL = 1 << 3,
	PAUSE = 1 << 4,
	RESUME = 1 << 5
};
enum class UserUpdateEvent : EventBits_t
{
	USER_TUNE = 1 << 0,
	DATA_ENTRY = 1 << 1,
	NEW_TEST = 1 << 2,
	DELETE_TEST = 1 << 3,
};
enum class DataEvent : EventBits_t
{
	SAVE = 1 << 0,
	JSON_READY = 1 << 1
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

class EventHelper
{
  public:
	static void setBits(SystemEvent e);
	static void setBits(SystemInitEvent e);
	static void setBits(TestEvent e);
	static void setBits(UserCommandEvent e);
	static void setBits(UserUpdateEvent e);
	static void setBits(DataEvent e);
	static void clearBits(SystemEvent e);
	static void clearBits(SystemInitEvent e);
	static void clearBits(TestEvent e);
	static void clearBits(UserCommandEvent e);
	static void clearBits(UserUpdateEvent e);
	static void clearBits(DataEvent e);
	static void resetSystemEventBits();
	static void resetSystemInitEventBits();
	static void resetTestEventBits();
	static void resetUserCommandEventBits();
	static void resetUserUpdateEventBits();
	static void resetDataEventBits();
	static void resetAllTestBits();

	static EventGroupHandle_t systemEventGroup;
	static EventGroupHandle_t systemInitEventGroup;
	static EventGroupHandle_t testEventGroup;
	static EventGroupHandle_t userCommandEventGroup;
	static EventGroupHandle_t userUpdateEventGroup;
	static EventGroupHandle_t dataEventGroup;
	static EventGroupHandle_t testControlEvent;

	EventGroupHandle_t getEventGroupTestControl() const
	{
		return testControlEvent;
	}
	EventGroupHandle_t getEventGroupUserCommand() const
	{
		return userCommandEventGroup;
	}
	EventGroupHandle_t getEventGroupUserUpdate() const
	{
		return userCommandEventGroup;
	}

  private:
	static void initializeEventGroups();
	static void cleanupEventGroups();

	static const EventBits_t SYSTEM_EVENT_BITS_MASK;
	static const EventBits_t SYSTEM_INIT_EVENT_BITS_MASK;
	static const EventBits_t TEST_EVENT_BITS_MASK;
	static const EventBits_t USER_COMMAND_EVENT_BITS_MASK;
	static const EventBits_t USER_UPDATE_EVENT_BITS_MASK;
	static const EventBits_t DATA_EVENT_BITS_MASK;
	static const EventBits_t ALL_TEST_BITS_MASK;
};

} // namespace Node_Core

#endif // EVENT_HELPER_H
