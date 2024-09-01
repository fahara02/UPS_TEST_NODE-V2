#include "TestSync.h"

TestSync& TestSync::getInstance()
{
	static TestSync instance;
	return instance;
}

TestSync::TestSync() :
	_cmdAcknowledged(false), _enableCurrentTest(false), _currentState{State::DEVICE_ON},
	eventGroupTest(NULL), eventGroupUser(NULL), eventGroupSync(NULL) // Initialize atomic variable
{
	for(int i = 0; i < MAX_TEST; ++i)
	{
		_testList[i] = RequiredTest();
	}
}

void TestSync::init()
{
	refreshState();
	if(eventGroupTest == NULL)
	{
		eventGroupTest = xEventGroupCreate();
	}

	if(eventGroupUser == NULL)
	{
		eventGroupUser = xEventGroupCreate();
	}

	if(eventGroupSync == NULL)
	{
		eventGroupSync = xEventGroupCreate();
	}
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
void TestSync::diableCurrentTest()
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

void TestSync::resetAllBits()
{
	xEventGroupClearBits(eventGroupTest, ALL_TEST_BITS);
	xEventGroupClearBits(eventGroupUser, ALL_CMD_BITS);
}

void TestSync::startTest(TestType test)
{
	TestSync& instance = TestSync::getInstance();
	if(instance.isTestEnabled())
	{
		EventBits_t test_eventbits = static_cast<EventBits_t>(test);
		xEventGroupSetBits(instance.eventGroupTest, test_eventbits);
		logger.log(LogLevel::WARNING, "test %s will be started", testTypeToString(test));
		instance.diableCurrentTest();
	}
}

void TestSync::stopTest(TestType test)
{
	TestSync& instance = TestSync::getInstance();
	EventBits_t test_eventbits = static_cast<EventBits_t>(test);
	xEventGroupClearBits(instance.eventGroupTest, test_eventbits);
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
			instance.reportEvent(Event::PENDING_TEST_FOUND);
			instance.refreshState();
			break;
		case UserCommand::DELETE_TEST:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::NEW_TEST));

			break;
		case UserCommand::PAUSE:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::RESUME));
			instance.reportEvent(Event::USER_RESUME);
			instance.refreshState();
			break;
		case UserCommand::RESUME:
			xEventGroupClearBits(eventGroupUser, static_cast<EventBits_t>(UserCommand::PAUSE));
			instance.reportEvent(Event::USER_PAUSED);
			instance.refreshState();
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

			xEventGroupClearBits(eventGroupSync, static_cast<EventBits_t>(SyncCommand::SKIP_TEST));
			break;
		case SyncCommand::SKIP_TEST:

			xEventGroupClearBits(eventGroupSync, static_cast<EventBits_t>(SyncCommand::RE_TEST));
			break;
		case SyncCommand::START_OBSERVER:

			xEventGroupClearBits(eventGroupSync,
								 static_cast<EventBits_t>(SyncCommand::STOP_OBSERVER));
			break;
		case SyncCommand::STOP_OBSERVER:

			xEventGroupClearBits(eventGroupSync,
								 static_cast<EventBits_t>(SyncCommand::START_OBSERVER));
			break;
		default:

			break;
	}

	// Set the current sync command
	xEventGroupSetBits(eventGroupSync, commandBits);
	logger.log(LogLevel::INFO, "Sync command %d triggered", static_cast<int>(command));
}

void TestSync::userCommandObserverTask(void* pvParameters)
{
	TestSync& instance = TestSync::getInstance();
	while(xEventGroupWaitBits(instance.eventGroupUser, static_cast<EventBits_t>(ALL_CMD_BITS),
							  pdFALSE, pdTRUE, portMAX_DELAY))
	{
		logger.log(LogLevel::SUCCESS, "New User Command Received");

		if(instance.iscmdAcknowledged())
		{
			vTaskDelete(NULL);
			return;
		}

		int allCMD = xEventGroupGetBits(instance.eventGroupUser);
		EventBits_t cmdAuto = static_cast<EventBits_t>(UserCommand::AUTO);
		EventBits_t cmdManual = static_cast<EventBits_t>(UserCommand::MANUAL);
		EventBits_t cmdStart = static_cast<EventBits_t>(UserCommand::START);
		EventBits_t cmdStop = static_cast<EventBits_t>(UserCommand::STOP);
		EventBits_t cmdPause = static_cast<EventBits_t>(UserCommand::PAUSE);
		EventBits_t cmdResume = static_cast<EventBits_t>(UserCommand::RESUME);
		EventBits_t cmdNewTest = static_cast<EventBits_t>(UserCommand::NEW_TEST);
		EventBits_t cmdDeleteTest = static_cast<EventBits_t>(UserCommand::DELETE_TEST);

		if((allCMD & cmdAuto) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::START_OBSERVER);
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
			instance.handleSyncCommand(SyncCommand::MANAGER_ACTIVE);
			if((allCMD & cmdManual) != 0)
			{
				instance.startTest(instance._testList[0].testType);
			}
			instance.acknowledgeCMD();
		}
		else if((allCMD & cmdStop) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::STOP_OBSERVER);
			if((allCMD & cmdManual) != 0)
			{
				instance.stopTest(instance._testList[0].testType);
			}
			instance.handleSyncCommand(SyncCommand::MANAGER_WAIT);
			instance.acknowledgeCMD();
		}
		else if((allCMD & cmdPause) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::STOP_OBSERVER);
			instance.handleSyncCommand(SyncCommand::MANAGER_WAIT);
			instance.acknowledgeCMD();
		}
		else if((allCMD & cmdResume) != 0)
		{
			instance.refreshState();
			instance.handleSyncCommand(SyncCommand::START_OBSERVER);
			instance.handleSyncCommand(SyncCommand::MANAGER_ACTIVE);
			instance.acknowledgeCMD();
		}
		else if((allCMD & cmdNewTest) != 0)
		{
			instance.refreshState();
			instance.enableCurrentTest();
			instance.acknowledgeCMD();
		}
		vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL);
}

void TestSync::testSyncTask(void* pvParameters)
{
	TestSync& instance = TestSync::getInstance();
	while(xEventGroupWaitBits(instance.eventGroupSync,
							  static_cast<EventBits_t>(SyncCommand::START_OBSERVER), pdFALSE,
							  pdTRUE, portMAX_DELAY))
	{
		logger.log(LogLevel::SUCCESS, "test Observer awaken ");
		vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL);
}