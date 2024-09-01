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
	static void setBits(SystemEvent e);
	static void setBits(SystemInitEvent e);
	static void setBits(TestEvent e);
	static void setBits(UserEvent e);
	static void setBits(DataEvent e);
	static void clearBits(SystemEvent e);
	static void clearBits(SystemInitEvent e);
	static void clearBits(TestEvent e);
	static void clearBits(UserEvent e);
	static void clearBits(DataEvent e);
	static void resetSystemEventBits();
	static void resetSystemInitEventBits();
	static void resetTestEventBits();
	static void resetUserEventBits();
	static void resetDataEventBits();

	static EventGroupHandle_t systemEventGroup;
	static EventGroupHandle_t systemInitEventGroup;
	static EventGroupHandle_t testEventGroup;
	static EventGroupHandle_t userEventGroup;
	static EventGroupHandle_t dataEventGroup;

  private:
	static void initializeEventGroups();
	static void cleanupEventGroups();

	static const EventBits_t SYSTEM_EVENT_BITS_MASK;
	static const EventBits_t SYSTEM_INIT_EVENT_BITS_MASK;
	static const EventBits_t TEST_EVENT_BITS_MASK;
	static const EventBits_t USER_EVENT_BITS_MASK;
	static const EventBits_t DATA_EVENT_BITS_MASK;
};

} // namespace Node_Core

#endif // EVENT_HELPER_H
