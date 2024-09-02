#include "EventHelper.h"
#include "Logger.h"
using namespace Node_Core;
extern Logger& logger;
namespace Node_Core
{
EventGroupHandle_t EventHelper::systemEventGroup = nullptr;
EventGroupHandle_t EventHelper::systemInitEventGroup = nullptr;
EventGroupHandle_t EventHelper::testEventGroup = nullptr;
EventGroupHandle_t EventHelper::userCommandEventGroup = nullptr;
EventGroupHandle_t EventHelper::userUpdateEventGroup = nullptr;
EventGroupHandle_t EventHelper::dataEventGroup = nullptr;
EventGroupHandle_t EventHelper::testControlEvent = nullptr;
EventGroupHandle_t EventHelper::syncControlEvent = nullptr;

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

const EventBits_t EventHelper::USER_COMMAND_EVENT_BITS_MASK =
	static_cast<EventBits_t>(UserCommandEvent::START) |
	static_cast<EventBits_t>(UserCommandEvent::STOP) |
	static_cast<EventBits_t>(UserCommandEvent::AUTO) |
	static_cast<EventBits_t>(UserCommandEvent::MANUAL) |
	static_cast<EventBits_t>(UserCommandEvent::PAUSE) |
	static_cast<EventBits_t>(UserCommandEvent::RESUME);
const EventBits_t EventHelper::USER_UPDATE_EVENT_BITS_MASK =
	static_cast<EventBits_t>(UserUpdateEvent::NEW_TEST) |
	static_cast<EventBits_t>(UserUpdateEvent::DELETE_TEST) |
	static_cast<EventBits_t>(UserUpdateEvent::DATA_ENTRY) |
	static_cast<EventBits_t>(UserUpdateEvent::USER_TUNE);
const EventBits_t EventHelper::DATA_EVENT_BITS_MASK =
	static_cast<EventBits_t>(DataEvent::SAVE) | static_cast<EventBits_t>(DataEvent::JSON_READY);
const EventBits_t EventHelper::ALL_TEST_BITS_MASK =
	static_cast<EventBits_t>(TestType::SwitchTest) |
	static_cast<EventBits_t>(TestType::BackupTest) |
	static_cast<EventBits_t>(TestType::EfficiencyTest) |
	static_cast<EventBits_t>(TestType::InputVoltageTest) |
	static_cast<EventBits_t>(TestType::WaveformTest) |
	static_cast<EventBits_t>(TestType::TunePWMTest);

const EventBits_t ALL_SYNC_BITS_MASK = (1 << MAX_SYNC_COMMAND) - 1;

void EventHelper::initializeEventGroups()
{
	if(systemEventGroup == nullptr)
		systemEventGroup = xEventGroupCreate();

	if(systemInitEventGroup == nullptr)
		systemInitEventGroup = xEventGroupCreate();

	if(testEventGroup == nullptr)
		testEventGroup = xEventGroupCreate();

	if(userCommandEventGroup == nullptr)
		userCommandEventGroup = xEventGroupCreate();

	if(userUpdateEventGroup == nullptr)
		userUpdateEventGroup = xEventGroupCreate();

	if(dataEventGroup == nullptr)
		dataEventGroup = xEventGroupCreate();

	if(testControlEvent == nullptr)
		testControlEvent = xEventGroupCreate();
	if(syncControlEvent == nullptr)
		syncControlEvent = xEventGroupCreate();

	logger.log(LogLevel::SUCCESS, "All event groups created!");
}

// Clean up FreeRTOS event groups (call this before shutdown)
void EventHelper::cleanupEventGroups()
{
	if(systemEventGroup != nullptr)
	{
		vEventGroupDelete(systemEventGroup);
		systemEventGroup = nullptr;
	}

	if(systemInitEventGroup != nullptr)
	{
		vEventGroupDelete(systemInitEventGroup);
		systemInitEventGroup = nullptr;
	}

	if(testEventGroup != nullptr)
	{
		vEventGroupDelete(testEventGroup);
		testEventGroup = nullptr;
	}

	if(userCommandEventGroup != nullptr)
	{
		vEventGroupDelete(userCommandEventGroup);
		userCommandEventGroup = nullptr;
	}

	if(userUpdateEventGroup != nullptr)
	{
		vEventGroupDelete(userUpdateEventGroup);
		userUpdateEventGroup = nullptr;
	}

	if(dataEventGroup != nullptr)
	{
		vEventGroupDelete(dataEventGroup);
		dataEventGroup = nullptr;
	}

	if(testControlEvent != nullptr)
	{
		vEventGroupDelete(testControlEvent);
		testControlEvent = nullptr;
	}
	if(syncControlEvent != nullptr)
	{
		vEventGroupDelete(testControlEvent);
		syncControlEvent = nullptr;
	}
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

void EventHelper::setBits(UserCommandEvent e)
{
	xEventGroupSetBits(userCommandEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::setBits(UserUpdateEvent e)
{
	xEventGroupSetBits(userUpdateEventGroup, static_cast<EventBits_t>(e));
}

void EventHelper::setBits(DataEvent e)
{
	xEventGroupSetBits(dataEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::setBits(TestType e)
{
	xEventGroupSetBits(testControlEvent, static_cast<EventBits_t>(e));
}
void EventHelper::setBits(SyncCommand e)
{
	xEventGroupSetBits(syncControlEvent, static_cast<EventBits_t>(e));
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
void EventHelper::clearBits(UserCommandEvent e)
{
	xEventGroupClearBits(userCommandEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::clearBits(UserUpdateEvent e)
{
	xEventGroupClearBits(userUpdateEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::clearBits(DataEvent e)
{
	xEventGroupClearBits(dataEventGroup, static_cast<EventBits_t>(e));
}
void EventHelper::clearBits(TestType e)
{
	xEventGroupClearBits(testControlEvent, static_cast<EventBits_t>(e));
}
void EventHelper::clearBits(SyncCommand e)
{
	xEventGroupClearBits(syncControlEvent, static_cast<EventBits_t>(e));
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
void EventHelper::resetUserCommandEventBits()
{
	xEventGroupClearBits(userCommandEventGroup, USER_COMMAND_EVENT_BITS_MASK);
}
void EventHelper::resetUserUpdateEventBits()
{
	xEventGroupClearBits(userUpdateEventGroup, USER_UPDATE_EVENT_BITS_MASK);
}
void EventHelper::resetDataEventBits()
{
	xEventGroupClearBits(dataEventGroup, DATA_EVENT_BITS_MASK);
}
void EventHelper::resetAllTestBits()
{
	xEventGroupClearBits(testControlEvent, ALL_TEST_BITS_MASK);
}
void EventHelper::resetAllSyncBits()
{
	xEventGroupClearBits(syncControlEvent, ALL_SYNC_BITS_MASK);
}
} // namespace Node_Core