#ifndef EVENT_HELPER_H
#define EVENT_HELPER_H

#include "StateDefines.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

namespace Node_Core
{
// Define enum classes for different event types
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

enum class UserEvent : EventBits_t
{
	AUTO_TEST_CMD = 1 << 0,
	MANUAL_OVERRIDE = 1 << 1,
	USER_TUNE = 1 << 2,
	MANUAL_DATA_ENTRY = 1 << 3,
	USER_PAUSED = 1 << 4,
	USER_RESUME = 1 << 5
};

enum class DataEvent : EventBits_t
{
	SAVE = 1 << 0,
	JSON_READY = 1 << 1
};

class EventHelper
{
  public:
	// Set bits for specific event types
	static void setBits(SystemEvent e)
	{
		xEventGroupSetBits(systemEventGroup, static_cast<EventBits_t>(e));
	}
	static void setBits(SystemInitEvent e)
	{
		xEventGroupSetBits(systemInitEventGroup, static_cast<EventBits_t>(e));
	}
	static void setBits(TestEvent e)
	{
		xEventGroupSetBits(testEventGroup, static_cast<EventBits_t>(e));
	}
	static void setBits(UserEvent e)
	{
		xEventGroupSetBits(userEventGroup, static_cast<EventBits_t>(e));
	}
	static void setBits(DataEvent e)
	{
		xEventGroupSetBits(dataEventGroup, static_cast<EventBits_t>(e));
	}

	// Clear bits for specific event types
	static void clearBits(SystemEvent e)
	{
		xEventGroupClearBits(systemEventGroup, static_cast<EventBits_t>(e));
	}
	static void clearBits(SystemInitEvent e)
	{
		xEventGroupClearBits(systemInitEventGroup, static_cast<EventBits_t>(e));
	}
	static void clearBits(TestEvent e)
	{
		xEventGroupClearBits(testEventGroup, static_cast<EventBits_t>(e));
	}
	static void clearBits(UserEvent e)
	{
		xEventGroupClearBits(userEventGroup, static_cast<EventBits_t>(e));
	}
	static void clearBits(DataEvent e)
	{
		xEventGroupClearBits(dataEventGroup, static_cast<EventBits_t>(e));
	}

	// Reset all bits in specific event groups
	static void resetSystemEventBits()
	{
		xEventGroupClearBits(systemEventGroup, SYSTEM_EVENT_BITS_MASK);
	}
	static void resetSystemInitEventBits()
	{
		xEventGroupClearBits(systemInitEventGroup, SYSTEM_INIT_EVENT_BITS_MASK);
	}
	static void resetTestEventBits()
	{
		xEventGroupClearBits(testEventGroup, TEST_EVENT_BITS_MASK);
	}
	static void resetUserEventBits()
	{
		xEventGroupClearBits(userEventGroup, USER_EVENT_BITS_MASK);
	}
	static void resetDataEventBits()
	{
		xEventGroupClearBits(dataEventGroup, DATA_EVENT_BITS_MASK);
	}

  private:
	// Initialize FreeRTOS event groups (call this once)
	static void initializeEventGroups()
	{
		systemEventGroup = xEventGroupCreate();
		systemInitEventGroup = xEventGroupCreate();
		testEventGroup = xEventGroupCreate();
		userEventGroup = xEventGroupCreate();
		dataEventGroup = xEventGroupCreate();
	}

	// Clean up FreeRTOS event groups (call this before shutdown)
	static void cleanupEventGroups()
	{
		if(systemEventGroup)
			vEventGroupDelete(systemEventGroup);
		if(systemInitEventGroup)
			vEventGroupDelete(systemInitEventGroup);
		if(testEventGroup)
			vEventGroupDelete(testEventGroup);
		if(userEventGroup)
			vEventGroupDelete(userEventGroup);
		if(dataEventGroup)
			vEventGroupDelete(dataEventGroup);
	}

	// Static event groups
	static EventGroupHandle_t systemEventGroup;
	static EventGroupHandle_t systemInitEventGroup;
	static EventGroupHandle_t testEventGroup;
	static EventGroupHandle_t userEventGroup;
	static EventGroupHandle_t dataEventGroup;

	// Event masks
	static const EventBits_t SYSTEM_EVENT_BITS_MASK;
	static const EventBits_t SYSTEM_INIT_EVENT_BITS_MASK;
	static const EventBits_t TEST_EVENT_BITS_MASK;
	static const EventBits_t USER_EVENT_BITS_MASK;
	static const EventBits_t DATA_EVENT_BITS_MASK;
};

// Define static members
EventGroupHandle_t EventHelper::systemEventGroup = nullptr;
EventGroupHandle_t EventHelper::systemInitEventGroup = nullptr;
EventGroupHandle_t EventHelper::testEventGroup = nullptr;
EventGroupHandle_t EventHelper::userEventGroup = nullptr;
EventGroupHandle_t EventHelper::dataEventGroup = nullptr;

const EventBits_t EventHelper::SYSTEM_EVENT_BITS_MASK =
	static_cast<EventBits_t>(SystemEvent::NONE) | static_cast<EventBits_t>(SystemEvent::ERROR) |
	static_cast<EventBits_t>(SystemEvent::SYSTEM_FAULT) |
	static_cast<EventBits_t>(SystemEvent::FAULT_CLEARED) |
	static_cast<EventBits_t>(SystemEvent::NETWORK_DISCONNECTED) |
	static_cast<EventBits_t>(SystemEvent::RESTART);

const EventBits_t EventHelper::SYSTEM_INIT_EVENT_BITS_MASK =
	static_cast<EventBits_t>(SystemInitEvent::SETTING_LOADED) |
	static_cast<EventBits_t>(SystemInitEvent::SELF_CHECK_OK) |
	static_cast<EventBits_t>(SystemInitEvent::LOAD_BANK_CHECKED);

const EventBits_t EventHelper::TEST_EVENT_BITS_MASK =
	static_cast<EventBits_t>(TestEvent::TEST_ONGOING) |
	static_cast<EventBits_t>(TestEvent::TEST_TIME_END) |
	static_cast<EventBits_t>(TestEvent::DATA_CAPTURED) |
	static_cast<EventBits_t>(TestEvent::VALID_DATA) |
	static_cast<EventBits_t>(TestEvent::TEST_FAILED) | static_cast<EventBits_t>(TestEvent::RETEST) |
	static_cast<EventBits_t>(TestEvent::TEST_LIST_EMPTY) |
	static_cast<EventBits_t>(TestEvent::PENDING_TEST_FOUND);

const EventBits_t EventHelper::USER_EVENT_BITS_MASK =
	static_cast<EventBits_t>(UserEvent::AUTO_TEST_CMD) |
	static_cast<EventBits_t>(UserEvent::MANUAL_OVERRIDE) |
	static_cast<EventBits_t>(UserEvent::USER_TUNE) |
	static_cast<EventBits_t>(UserEvent::MANUAL_DATA_ENTRY) |
	static_cast<EventBits_t>(UserEvent::USER_PAUSED) |
	static_cast<EventBits_t>(UserEvent::USER_RESUME);

const EventBits_t EventHelper::DATA_EVENT_BITS_MASK =
	static_cast<EventBits_t>(DataEvent::SAVE) | static_cast<EventBits_t>(DataEvent::JSON_READY);

} // namespace Node_Core

#endif // EVENT_HELPER_H
