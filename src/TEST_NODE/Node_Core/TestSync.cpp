#include "TestSync.h"

TestSync& TestSync::getInstance()
{
	static TestSync instance;
	return instance;
}

TestSync::TestSync() :
	_cmdAcknowledged(false), _enableCurrentTest(false), _currentState{State::DEVICE_ON}

{
	for(int i = 0; i < MAX_TEST; ++i)
	{
		_testList[i] = RequiredTest();
	}
}

void TestSync::init()
{
	refreshState();
	Node_Core::EventHelper::initializeEventGroups();
	createSynctask();

	logger.log(LogLevel::INFO, "testSync initialization");
}

void TestSync::createSynctask()
{
	xTaskCreatePinnedToCore(userCommandObserverTask, "commandObserver", 12000, NULL, 3,
							&commandObserverTaskHandle, 0);
}

void TestSync::reportEvent(Event event)
{
	stateMachine.handleEvent(event);
}

State TestSync::refreshState()
{
	_currentState.store(stateMachine.getCurrentState());
	return _currentState.load();
}

bool TestSync::iscmdAcknowledged()
{
	return _cmdAcknowledged;
}
void TestSync::acknowledgeCMD()
{
	_cmdAcknowledged = true;
}
void TestSync::acknowledgeCMDReset()
{
	_cmdAcknowledged = false;
}
void TestSync::enableCurrentTest()
{
	_enableCurrentTest = true;
}
void TestSync::disableCurrentTest()
{
	_enableCurrentTest = false;
}
bool TestSync::isTestEnabled()
{
	return _enableCurrentTest;
}

void TestSync::parseIncomingJson(JsonVariant json)
{
	// Reset all tests as inactive
	for(int i = 0; i < MAX_TEST; i++)
	{
		_testList[i].isActive = false;
	}

	if(json.is<JsonObject>())
	{
		logger.log(LogLevel::TEST, "TEST SYNC FUNCTION RECEIVING THIS");
		serializeJsonPretty(json, Serial);
		JsonObject jsonObj = json.as<JsonObject>();

		if(jsonObj.containsKey("testName") && jsonObj.containsKey("loadLevel"))
		{
			parseTestJson(jsonObj);
		}
		else if(jsonObj.containsKey("startCommand") || jsonObj.containsKey("stopCommand") ||
				jsonObj.containsKey("autoCommand") || jsonObj.containsKey("manualCommand") ||
				jsonObj.containsKey("pauseCommand") || jsonObj.containsKey("resumeCommand"))
		{
			// parseUserCommandJson(jsonObj);
		}
		else
		{
			logger.log(LogLevel::ERROR, "Required fields missing");
		}
	}
	else
	{
		logger.log(LogLevel::ERROR, "Unknown JSON format!!!");
	}
}

void TestSync::startTest(TestType test)
{
	if(isTestEnabled())
	{
		EventHelper::setBits(test);
		logger.log(LogLevel::WARNING, "test %s will be started", testTypeToString(test));
		disableCurrentTest();
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

void TestSync::stopTest(TestType test)
{
	EventHelper::clearBits(test);
	logger.log(LogLevel::WARNING, "test %s will be stopped", testTypeToString(test));
	vTaskDelay(pdMS_TO_TICKS(50));
}

void TestSync::stopAllTest()
{
	EventHelper::resetAllTestBits();
	logger.log(LogLevel::WARNING, "All test stopped");
	vTaskDelay(pdMS_TO_TICKS(50));
}

void TestSync::parseTestJson(JsonObject jsonObj)
{
	bool isExistingTest = false;
	String testName = jsonObj["testName"];
	String loadLevel = jsonObj["loadLevel"];
	TestType testType = getTestTypeFromString(testName);
	LoadPercentage loadPercentage = getLoadLevelFromString(loadLevel);

	if(testType == static_cast<TestType>(0))
	{
		logger.log(LogLevel::ERROR, "Unknown testName: ", testName);
		return;
	}

	if(loadPercentage == static_cast<LoadPercentage>(-1))
	{
		logger.log(LogLevel::ERROR, "Invalid loadLevel: ", loadLevel);
		return;
	}

	for(int i = 0; i < MAX_TEST; i++)
	{
		if(_testList[i].testType == testType)
		{
			isExistingTest = true;
			_testList[i].loadLevel = loadPercentage;
			_testList[i].isActive = true;
			break;
		}
	}

	if(!isExistingTest)
	{
		for(int i = 0; i < MAX_TEST; i++)
		{
			if(!_testList[i].isActive)
			{
				_testList[i].testId = i + 1;
				_testList[i].testType = testType;
				_testList[i].loadLevel = loadPercentage;
				_testList[i].isActive = true;
				logger.log(LogLevel::SUCCESS, "Received new test requirement");
				logger.log(LogLevel::INFO, "TestName:", testTypeToString(_testList[i].testType));
				logger.log(LogLevel::INFO,
						   "LoadLevel percent:", loadPercentageToString(_testList[i].loadLevel));

				handleUserUpdate(UserUpdateEvent::NEW_TEST);
				return;
			}
		}
		logger.log(LogLevel::WARNING, "Test list is full, unable to add new test.");
	}
}

void TestSync::checkForDeletedTests()
{
	bool testDeleted = false;

	for(int i = 0; i < MAX_TEST; i++)
	{
		if(!_testList[i].isActive && _testList[i].testId != 0)
		{
			logger.log(LogLevel::WARNING, "Removing test requirement");
			logger.log(LogLevel::INFO, "TestName:", testTypeToString(_testList[i].testType));
			logger.log(LogLevel::INFO,
					   "LoadLevel percent:", loadPercentageToString(_testList[i].loadLevel));
			_testList[i] = {};
			testDeleted = true;
		}
	}

	if(testDeleted)
	{
		handleUserUpdate(UserUpdateEvent::DELETE_TEST);
	}
}
void TestSync::handleUserUpdate(UserUpdateEvent update)
{
	switch(update)
	{
		case UserUpdateEvent::NEW_TEST:
			EventHelper::setBits(UserUpdateEvent::NEW_TEST);
			EventHelper::clearBits(UserUpdateEvent::DELETE_TEST);
			reportEvent(Event::NEW_TEST);
			refreshState();
			break;
		case UserUpdateEvent::DELETE_TEST:
			EventHelper::setBits(UserUpdateEvent::DELETE_TEST);
			EventHelper::clearBits(UserUpdateEvent::NEW_TEST);
			reportEvent(Event::REJECT_CURRENT_TEST);
			refreshState();
			break;
		case UserUpdateEvent::DATA_ENTRY:
			EventHelper::setBits(UserUpdateEvent::DATA_ENTRY);
			reportEvent(Event::DATA_ENTRY);
			refreshState();
			break;
		case UserUpdateEvent::USER_TUNE:
			EventHelper::setBits(UserUpdateEvent::USER_TUNE);
			reportEvent(Event::USER_TUNE);
			refreshState();
			break;
		default:
			break;
	}
}
void TestSync::handleUserCommand(UserCommandEvent command)
{
	EventBits_t commandBits = static_cast<EventBits_t>(command);
	switch(command)
	{
		case UserCommandEvent ::PAUSE:
			EventHelper::clearBits(UserCommandEvent::RESUME);
			reportEvent(Event::PAUSE);
			refreshState();

			break;
		case UserCommandEvent ::RESUME:
			EventHelper::clearBits(UserCommandEvent::PAUSE);
			reportEvent(Event::RESUME);
			refreshState();
			break;
		case UserCommandEvent ::AUTO:
			EventHelper::clearBits(UserCommandEvent::MANUAL);
			StateMachine::getInstance().setMode(TestMode::AUTO);
			refreshState();
			break;
		case UserCommandEvent ::MANUAL:
			EventHelper::clearBits(UserCommandEvent::AUTO);
			StateMachine::getInstance().setMode(TestMode::MANUAL);
			refreshState();

			break;
		case UserCommandEvent ::START:
			EventHelper::clearBits(UserCommandEvent::STOP);
			reportEvent(Event::START);
			refreshState();
			break;
		case UserCommandEvent ::STOP:
			EventHelper::clearBits(UserCommandEvent::START);
			reportEvent(Event::STOP);
			refreshState();
			break;
		default:
			break;
	}
	xEventGroupSetBits(EventHelper::userCommandEventGroup, commandBits);
}

void TestSync::handleSyncCommand(SyncCommand command)
{
	EventBits_t commandBits = static_cast<EventBits_t>(command);

	switch(command)
	{
		case SyncCommand::MANAGER_WAIT:
			EventHelper::clearBits(SyncCommand::MANAGER_ACTIVE);

			break;
		case SyncCommand::MANAGER_ACTIVE:
			EventHelper::clearBits(SyncCommand::MANAGER_WAIT);

			break;
		case SyncCommand::RE_TEST:
			EventHelper::clearBits(SyncCommand::SKIP_TEST);

			break;
		case SyncCommand::SKIP_TEST:

			EventHelper::clearBits(SyncCommand::RE_TEST);
			break;
		case SyncCommand::START_OBSERVER:

			EventHelper::clearBits(SyncCommand::STOP_OBSERVER);
			break;
		case SyncCommand::STOP_OBSERVER:

			EventHelper::clearBits(SyncCommand::START_OBSERVER);
			break;
		default:

			break;
	}

	xEventGroupSetBits(EventHelper::syncControlEvent, commandBits);
}

void TestSync::handleTestEvent(Event event)
{
	TestSync& instance = TestSync::getInstance();
	logger.log(LogLevel::INFO, "Handling Test event %s", eventToString(event));
	// Event eventBit = static_cast<Event>(event_bits);
	// instance.stateMachine.handleEventbits(eventBit);
}

void TestSync::userCommandObserverTask(void* pvParameters)
{
	TestSync& instance = TestSync::getInstance();

	// Create bit masks for the command and update events
	const EventBits_t CMD_BITS_MASK = static_cast<EventBits_t>(UserCommandEvent::START) |
									  static_cast<EventBits_t>(UserCommandEvent::STOP) |
									  static_cast<EventBits_t>(UserCommandEvent::AUTO) |
									  static_cast<EventBits_t>(UserCommandEvent::MANUAL) |
									  static_cast<EventBits_t>(UserCommandEvent::PAUSE) |
									  static_cast<EventBits_t>(UserCommandEvent::RESUME);

	const EventBits_t UPDATE_BITS_MASK = static_cast<EventBits_t>(UserUpdateEvent::USER_TUNE) |
										 static_cast<EventBits_t>(UserUpdateEvent::DATA_ENTRY) |
										 static_cast<EventBits_t>(UserUpdateEvent::NEW_TEST) |
										 static_cast<EventBits_t>(UserUpdateEvent::DELETE_TEST);
	while(true)
	{
		while(xEventGroupWaitBits(EventHelper::userCommandEventGroup, CMD_BITS_MASK, pdFALSE,
								  pdTRUE, portMAX_DELAY))
		{
			int cmdResult = xEventGroupGetBits(EventHelper::userCommandEventGroup);

			logger.log(LogLevel::SUCCESS, "New User Command Received");

			if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::AUTO)) != 0)
			{
				instance.refreshState();
				instance.handleSyncCommand(SyncCommand::START_OBSERVER);
				instance.acknowledgeCMD();
				return;
			}
			else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::MANUAL)) != 0)
			{
				instance.refreshState();
				instance.handleSyncCommand(SyncCommand::START_OBSERVER);
				instance.acknowledgeCMD();
				return;
			}
			else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::START)) != 0)
			{
				instance.refreshState();
				instance.handleSyncCommand(SyncCommand::MANAGER_ACTIVE);
				if(cmdResult & static_cast<EventBits_t>(UserCommandEvent::MANUAL))
				{
					instance.startTest(instance._testList[0].testType);
				}
				instance.acknowledgeCMD();
				return;
			}
			else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::STOP)) != 0)
			{
				instance.refreshState();
				instance.handleSyncCommand(SyncCommand::STOP_OBSERVER);
				if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::MANUAL)) != 0)
				{
					instance.stopTest(instance._testList[0].testType);
				}
				else
				{
					instance.stopAllTest();
					instance.handleSyncCommand(SyncCommand::MANAGER_WAIT);
				}
				instance.acknowledgeCMD();
				return;
			}
			else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::PAUSE)) != 0)
			{
				instance.refreshState();
				instance.stopAllTest();
				instance.handleSyncCommand(SyncCommand::STOP_OBSERVER);
				instance.handleSyncCommand(SyncCommand::MANAGER_WAIT);
				instance.acknowledgeCMD();
				return;
			}
			else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::RESUME)) != 0)
			{
				instance.refreshState();
				instance.handleSyncCommand(SyncCommand::START_OBSERVER);
				instance.handleSyncCommand(SyncCommand::MANAGER_ACTIVE);
				instance.acknowledgeCMD();
				return;
			}
			vTaskDelay(pdMS_TO_TICKS(200));
		}

		while(EventBits_t updateBits =
				  xEventGroupWaitBits(EventHelper::userUpdateEventGroup, UPDATE_BITS_MASK, pdFALSE,
									  pdTRUE, portMAX_DELAY))
		{
			// Wait for any bit in the userCommandEventGroup
			int updateResult = xEventGroupGetBits(EventHelper::userCommandEventGroup);

			logger.log(LogLevel::SUCCESS, "Implementing updates");

			if((updateResult & static_cast<EventBits_t>(UserUpdateEvent::NEW_TEST)) != 0)
			{
				instance.refreshState();
				instance.enableCurrentTest();
				instance.acknowledgeCMD();
				return;
			}
			else if((updateResult & static_cast<EventBits_t>(UserUpdateEvent::DELETE_TEST)) != 0)
			{
				instance.refreshState();
				instance.disableCurrentTest();
				instance.acknowledgeCMD();
				return;
			}

			vTaskDelay(pdMS_TO_TICKS(200));
		}
	}

	vTaskDelete(NULL);
}

void TestSync::testSyncObserverTask(void* pvParameters)
{
	TestSync& instance = TestSync::getInstance();
	while(xEventGroupWaitBits(EventHelper::syncControlEvent,
							  static_cast<EventBits_t>(SyncCommand::START_OBSERVER), pdFALSE,
							  pdTRUE, portMAX_DELAY))
	{
		logger.log(LogLevel::INFO, "Observing test.. ");

		EventBits_t RETEST_BIT_MASK = static_cast<EventBits_t>(TestEvent::RETEST);

		while(xEventGroupWaitBits(EventHelper::testEventGroup, RETEST_BIT_MASK, pdFALSE, pdFALSE,
								  portMAX_DELAY))
		{
			EventBits_t testEventBits = xEventGroupGetBits(EventHelper::testEventGroup);
		}

		vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL);
}