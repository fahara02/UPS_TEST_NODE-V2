#include "TestSync.h"

TestSync& TestSync::getInstance()
{
	static TestSync instance;
	return instance;
}

TestSync::TestSync() : _currentState{State::DEVICE_ON}
{
}

void TestSync::init()
{
	refreshState();
	eventGroupTest = xEventGroupCreate();
	eventGroupUser = xEventGroupCreate();
	eventGroupSync = xEventGroupCreate();
	resetAllBits();
	logger.log(LogLevel::INFO, "testSync initialization");
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

void TestSync::resetAllBits()
{
	xEventGroupClearBits(eventGroupTest, ALL_TEST_BITS);
	xEventGroupClearBits(eventGroupUser, ALL_CMD_BITS);
}

void TestSync::startTest(TestType test)
{
	EventBits_t test_eventbits = static_cast<EventBits_t>(test);
	xEventGroupSetBits(eventGroupTest, test_eventbits);
	logger.log(LogLevel::WARNING, "test %s will be started", testTypeToString(test));
}

void TestSync::stopTest(TestType test)
{
	EventBits_t test_eventbits = static_cast<EventBits_t>(test);
	xEventGroupClearBits(eventGroupTest, test_eventbits);
	logger.log(LogLevel::WARNING, "test %s will be stopped", testTypeToString(test));
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

				handleUserCommand(UserCommand::NEW_TEST);
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
		handleUserCommand(UserCommand::DELETE_TEST);
	}
}

void TestSync::handleUserCommand(UserCommand command)
{
	EventBits_t commandBits = static_cast<EventBits_t>(command);
	TestSync& instance = TestSync::getInstance();

	switch(command)
	{
		case UserCommand::NEW_TEST:
			xEventGroupClearBits(eventGroupUser,
								 static_cast<EventBits_t>(UserCommand::DELETE_TEST));
			break;
		case UserCommand::DELETE_TEST:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::NEW_TEST));
			break;
		case UserCommand::PAUSE:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::RESUME));
			break;
		case UserCommand::RESUME:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::PAUSE));
			break;
		case UserCommand::AUTO:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::MANUAL));
			instance.reportEvent(Event::AUTO_TEST_CMD);
			instance.refreshState();
			break;
		case UserCommand::MANUAL:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::AUTO));
			instance.reportEvent(Event::MANUAL_OVERRIDE);
			instance.refreshState();
			break;
		case UserCommand::START:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::STOP));
			break;
		case UserCommand::STOP:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::START));
			break;
		default:
			break;
	}

	xEventGroupSetBits(eventGroupUser, commandBits);
	logger.log(LogLevel::INFO, "User command %d triggered", static_cast<int>(command));
}

void TestSync::handleSyncCommand(SyncCommand command)
{
	EventBits_t commandBits = static_cast<EventBits_t>(command);

	switch(command)
	{
		case SyncCommand::MANAGER_WAIT:
			xEventGroupClearBits(eventGroupSync,
								 static_cast<EventBits_t>(SyncCommand::MANAGER_ACTIVE));
			break;
		case SyncCommand::MANAGER_ACTIVE:
			// Clear WAIT when ACTIVATE is issued
			xEventGroupClearBits(eventGroupSync,
								 static_cast<EventBits_t>(SyncCommand::MANAGER_WAIT));
			break;
		case SyncCommand::RE_TEST:
			// Clear SKIP_TEST if RE_TEST is issued
			xEventGroupClearBits(eventGroupSync, static_cast<EventBits_t>(SyncCommand::SKIP_TEST));
			break;
		case SyncCommand::SKIP_TEST:
			// Clear RE_TEST if SKIP_TEST is issued
			xEventGroupClearBits(eventGroupSync, static_cast<EventBits_t>(SyncCommand::RE_TEST));
			break;
		case SyncCommand::START_OBSERVER:
			// Clear STOP_TEST when starting the test
			xEventGroupClearBits(eventGroupSync,
								 static_cast<EventBits_t>(SyncCommand::STOP_OBSERVER));
			break;
		case SyncCommand::STOP_OBSERVER:

			xEventGroupClearBits(eventGroupSync,
								 static_cast<EventBits_t>(SyncCommand::START_OBSERVER));
			break;
		default:
			// No conflicting commands to clear for SAVE or IGNORE
			break;
	}

	// Set the current sync command
	xEventGroupSetBits(eventGroupSync, commandBits);
	logger.log(LogLevel::INFO, "Sync command %d triggered", static_cast<int>(command));
}

void TestSync::userCommandObserverTask(void* pvParameters)
{
	while(xEventGroupWaitBits(eventGroupUser, static_cast<EventBits_t>(ALL_CMD_BITS), pdFALSE,
							  pdTRUE, portMAX_DELAY))
	{
		logger.log(LogLevel::SUCCESS, "New User Command Received");
		TestSync& instance = TestSync::getInstance();
		if(instance.iscmdAcknowledged())
		{
			vTaskDelete(NULL);
			return;
		}

		int allCMD = xEventGroupGetBits(eventGroupUser);
		EventBits_t cmdAuto = static_cast<EventBits_t>(UserCommand::AUTO);
		EventBits_t cmdManual = static_cast<EventBits_t>(UserCommand::MANUAL);
		EventBits_t cmdStart = static_cast<EventBits_t>(UserCommand::START);
		EventBits_t cmdStop = static_cast<EventBits_t>(UserCommand::STOP);
		EventBits_t cmdPause = static_cast<EventBits_t>(UserCommand::PAUSE);
		EventBits_t cmdResume = static_cast<EventBits_t>(UserCommand::RESUME);

		if((allCMD & cmdAuto) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::START_OBSERVER);
			instance.handleSyncCommand(SyncCommand::MANAGER_ACTIVE);
			instance.acknowledgeCMD();
		}
		else if((allCMD & cmdManual) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::START_OBSERVER);
			instance.acknowledgeCMD();
		}
		else if((allCMD & cmdStart) != 0)
		{
			instance.refreshState();
			// have to link to manager
			instance.acknowledgeCMD();
		}
		else if((allCMD & cmdStop) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::STOP_OBSERVER);
			instance.handleSyncCommand(SyncCommand::MANAGER_WAIT);
			instance.acknowledgeCMD();
		}
		vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL);
}

void TestSync::testSyncTask(void* pvParameters)
{
	while(xEventGroupWaitBits(eventGroupSync, static_cast<EventBits_t>(SyncCommand::START_OBSERVER),
							  pdFALSE, pdTRUE, portMAX_DELAY))
	{
		logger.log(LogLevel::SUCCESS, "test Observer awaken ");
		vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL);
}