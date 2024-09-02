#include "TestSync.h"

TestSync& TestSync::getInstance()
{
	static TestSync instance;
	return instance;
}

TestSync::TestSync() :
	_cmdAcknowledged(false), _enableCurrentTest(false), _parsingOngoing(false),
	_currentState{State::DEVICE_ON}

{
	for(int i = 0; i < MAX_TEST; ++i)
	{
		_testList[i] = RequiredTest();
		_testID[i] = i * 2 + 31;
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
	TestSync& SyncTest = TestSync::getInstance();
	xTaskCreatePinnedToCore(userCommandObserverTask, "commandObserver", 8192, NULL, 1,
							&commandObserverTaskHandle, 0);
	xTaskCreatePinnedToCore(userUpdateObserverTask, "updateObserver", 8192, NULL, 1,
							&updateObserverTaskHandle, 0);

	logger.log(LogLevel::INFO, "testSync task created initialization");
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
State getState()
{
	return stateMachine.getCurrentState();
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
			// DeserializationError error = deserializeJson(_testDoc, json);
			if(!_parsingOngoing)
			{
				logger.log(LogLevel::SUCCESS, "Send to Json test parser");
				parseTestJson(jsonObj);
			}
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

void TestSync::parseTestJson(JsonObject jsonObj)
{
	logger.log(LogLevel::TEST, "parseTest JSON INPUT");
	serializeJsonPretty(jsonObj, Serial);

	bool isExistingTest = false;

	String testName = jsonObj["testName"];
	String loadLevel = jsonObj["loadLevel"];
	TestType testType = getTestTypeFromString(testName);
	LoadPercentage loadPercentage = getLoadLevelFromString(loadLevel);

	logger.log(LogLevel::TEST, "parseTest JSON CHECK,Printing again");
	Serial.println(testTypeToString(testType));
	Serial.println(loadPercentageToString(loadPercentage));

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

	if(!_parsingOngoing)

	{
		logger.log(LogLevel::TEST, "into parsing zone");
		_parsingOngoing = true;

		for(int i = 0; i < MAX_TEST; i++)
		{
			logger.log(LogLevel::TEST, "parsing no %d", i);
			if(_testList[i].testId == _testID[i] && _testList[i].testType == testType)
			{
				isExistingTest = true;
				_testList[i].loadLevel = loadPercentage;
				_testList[i].isActive = true;

				break;
			}
		}
		logger.log(LogLevel::TEST, "checking boolean flag");

		if(!isExistingTest)
		{
			logger.log(LogLevel::TEST, "parseTest NEW TEST CHECKED");
			for(int i = 0; i < MAX_TEST; i++)
			{
				if(!_testList[i].isActive)
				{
					_testList[i].testId = _testID[i];
					_testList[i].testType = testType;
					_testList[i].loadLevel = loadPercentage;
					_testList[i].isActive = true;
					logger.log(LogLevel::SUCCESS, "Received new test requirement");
					logger.log(LogLevel::INFO, "TestName:%s",
							   testTypeToString(_testList[i].testType));
					logger.log(LogLevel::INFO, "LoadLevel percent:%s",
							   loadPercentageToString(_testList[i].loadLevel));

					handleUserUpdate(UserUpdateEvent::NEW_TEST);
					return;
				}
			}
			logger.log(LogLevel::WARNING, "Test list is full, unable to add new test.");
		}
		_parsingOngoing = false;
	}

	logger.log(LogLevel::TEST, "parseTest JSON EXIT");
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
	TestSync& SyncTest = TestSync::getInstance();
	switch(update)
	{
		case UserUpdateEvent::NEW_TEST:

			EventHelper::setBits(UserUpdateEvent::NEW_TEST);
			EventHelper::clearBits(UserUpdateEvent::DELETE_TEST);

			logger.log(LogLevel::SUCCESS, "Handled New Test Event in handleUserUpdat() ");

			refreshState();
			break;
		case UserUpdateEvent::DELETE_TEST:

			refreshState();
			break;
		case UserUpdateEvent::DATA_ENTRY:
			EventHelper::setBits(UserUpdateEvent::DATA_ENTRY);
			refreshState();
			break;
		case UserUpdateEvent::USER_TUNE:
			EventHelper::setBits(UserUpdateEvent::USER_TUNE);
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
	State syncState = instance.refreshState();
	logger.log(LogLevel::INFO, "Sync Class state is:%s", stateToString(syncState));

	while(xEventGroupWaitBits(EventHelper::userCommandEventGroup, CMD_BITS_MASK, pdFALSE, pdFALSE,
							  portMAX_DELAY))
	{
		int cmdResult = xEventGroupGetBits(EventHelper::userCommandEventGroup);
		logger.log(LogLevel::SUCCESS, "New User Command Received");

		if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::AUTO)) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::START_OBSERVER);
			instance.acknowledgeCMD();
		}
		else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::MANUAL)) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::START_OBSERVER);
			instance.acknowledgeCMD();
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
		}
		else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::PAUSE)) != 0)
		{
			instance.refreshState();
			instance.stopAllTest();
			instance.handleSyncCommand(SyncCommand::STOP_OBSERVER);
			instance.handleSyncCommand(SyncCommand::MANAGER_WAIT);
			instance.acknowledgeCMD();
		}
		else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::RESUME)) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::START_OBSERVER);
			instance.handleSyncCommand(SyncCommand::MANAGER_ACTIVE);
			instance.acknowledgeCMD();
		}
		vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL);
}

void TestSync::userUpdateObserverTask(void* pvParameters)
{
	TestSync& instance = TestSync::getInstance();
	const EventBits_t UPDATE_BITS_MASK = static_cast<EventBits_t>(UserUpdateEvent::USER_TUNE) |
										 static_cast<EventBits_t>(UserUpdateEvent::DATA_ENTRY) |
										 static_cast<EventBits_t>(UserUpdateEvent::NEW_TEST) |
										 static_cast<EventBits_t>(UserUpdateEvent::DELETE_TEST);

	while(xEventGroupWaitBits(EventHelper::userUpdateEventGroup, UPDATE_BITS_MASK, pdFALSE, pdFALSE,
							  portMAX_DELAY))
	{
		int updateBits = xEventGroupGetBits(EventHelper::userUpdateEventGroup);

		logger.log(LogLevel::SUCCESS, "Implementing updates from Test Sync");
		State syncState = instance.refreshState();
		logger.log(LogLevel::INFO, "Sync Class state is:%s", stateToString(syncState));

		if((updateBits & static_cast<EventBits_t>(UserUpdateEvent::NEW_TEST)) != 0)
		{
			logger.log(LogLevel::SUCCESS, "handling for bit set NEW TEST");
			instance.refreshState();
			instance.enableCurrentTest();
			logger.log(LogLevel::TEST, "Invoking State Change...");
			instance.reportEvent(Event::NEW_TEST);
			vTaskDelay(pdMS_TO_TICKS(200));
			logger.log(LogLevel::TEST, "Is state changed?");
			instance.acknowledgeCMD();
			EventHelper::clearBits(UserUpdateEvent::NEW_TEST);
		}
		else if((updateBits & static_cast<EventBits_t>(UserUpdateEvent::DELETE_TEST)) != 0)
		{
			instance.refreshState();
			instance.disableCurrentTest();
			instance.acknowledgeCMD();
			EventHelper::clearBits(UserUpdateEvent::DELETE_TEST);
		}

		vTaskDelay(pdMS_TO_TICKS(200));
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