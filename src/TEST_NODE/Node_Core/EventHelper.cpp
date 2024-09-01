#include "EventHelper.h"

namespace Node_Core
{
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

void EventHelper::initializeEventGroups()
{
	systemEventGroup = xEventGroupCreate();
	systemInitEventGroup = xEventGroupCreate();
	testEventGroup = xEventGroupCreate();
	userEventGroup = xEventGroupCreate();
	dataEventGroup = xEventGroupCreate();
}

// Clean up FreeRTOS event groups (call this before shutdown)
void EventHelper::cleanupEventGroups()
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

void EventHelper::setBits(SystemEvent e)
{
	xEventGroupSetBits(systemEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::setBits(SystemInitEvent e)
{
	xEventGroupSetBits(systemInitEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::setBits(TestEvent e)
{
	xEventGroupSetBits(testEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::setBits(UserEvent e)
{
	xEventGroupSetBits(userEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::setBits(DataEvent e)
{
	xEventGroupSetBits(dataEventGroup, static_cast<EventBits_t>(e));
}

// Clear bits for specific event types
void EventHelper::clearBits(SystemEvent e)
{
	xEventGroupClearBits(systemEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::clearBits(SystemInitEvent e)
{
	xEventGroupClearBits(systemInitEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::clearBits(TestEvent e)
{
	xEventGroupClearBits(testEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::clearBits(UserEvent e)
{
	xEventGroupClearBits(userEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::clearBits(DataEvent e)
{
	xEventGroupClearBits(dataEventGroup, static_cast<EventBits_t>(e));
}

// Reset all bits in specific event groups
void EventHelper::resetSystemEventBits()
{
	xEventGroupClearBits(systemEventGroup, SYSTEM_EVENT_BITS_MASK);
}
void EventHelper::resetSystemInitEventBits()
{
	xEventGroupClearBits(systemInitEventGroup, SYSTEM_INIT_EVENT_BITS_MASK);
}
void EventHelper::resetTestEventBits()
{
	xEventGroupClearBits(testEventGroup, TEST_EVENT_BITS_MASK);
}
void EventHelper::resetUserEventBits()
{
	xEventGroupClearBits(userEventGroup, USER_EVENT_BITS_MASK);
}
void EventHelper::resetDataEventBits()
{
	xEventGroupClearBits(dataEventGroup, DATA_EVENT_BITS_MASK);
}

} // namespace Node_Core