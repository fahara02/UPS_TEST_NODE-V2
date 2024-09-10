#include "TestSync.h"
#include "TestManager.h"
#include "DataHandler.h"
#include "HPTSettings.h"

TestSync& TestSync::getInstance()
{
	static TestSync instance;
	return instance;
}

TestSync::TestSync() :
	_cmdAcknowledged(false), _enableCurrentTest(false), parsingOngoing(false),
	_currentState{State::DEVICE_ON}

{
	for(int i = 0; i < MAX_TEST; ++i)
	{
		_testList[i].testId = 0;
		_testList[i].testType = TestType::TunePWMTest;
		_testList[i].loadLevel = LoadPercentage::LOAD_0P;
		_testList[i].isActive = false;
		_testID[i] = i * 2 + 31;
	}
}

void TestSync::init()
{
	Node_Core::EventHelper::initializeEventGroups();

	createSynctask();

	logger.log(LogLevel::INFO, "testSync initialization");
}

void TestSync::createSynctask()
{
	xTaskCreatePinnedToCore(userCommandTask, "userCommand", userCommand_Stack, NULL,
							userCommand_Priority, &commandObserverTaskHandle, userCommand_CORE);
	xTaskCreatePinnedToCore(userUpdateTask, "userUpdate", userUpdate_Stack, NULL,
							userUpdate_Priority, &updateObserverTaskHandle, userUpdate_CORE);
	xTaskCreatePinnedToCore(testSyncTask, "testSync", testSync_Stack, NULL, testSync_Priority,
							&testObserverTaskHandle, testSync_CORE);

	logger.log(LogLevel::INFO, "testSync task created initialization");
}

void TestSync::reportEvent(Event event)
{
	StateMachine::getInstance().handleEvent(event);
}

State getState()
{
	return StateMachine::getInstance().getCurrentState();
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
	if(json.is<JsonObject>())
	{
		JsonObject jsonObj = json.as<JsonObject>();

		if(jsonObj.containsKey("testName") && jsonObj.containsKey("loadLevel"))
		{
			logger.log(LogLevel::SUCCESS, "Queueing JSON for processing");
			jsonQueue.push(jsonObj); // Enqueue the JSON object

			if(!parsingOngoing) // Only start parsing if no other parsing is ongoing
			{
				processNextJson();
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

void TestSync::processNextJson()
{
	if(!jsonQueue.empty())
	{
		parsingOngoing = true;

		JsonObject jsonObj = jsonQueue.front(); // Get the next JSON object
		jsonQueue.pop(); // Remove it from the queue

		parseTestJson(jsonObj); // Parse the JSON

		parsingOngoing = false;

		if(!jsonQueue.empty()) // If there are more JSON objects in the queue, process the next one
		{
			processNextJson();
		}
	}
}

void TestSync::parseTestJson(JsonObject jsonObj)
{
	logger.log(LogLevel::TEST, "parseTest JSON INPUT");
	serializeJsonPretty(jsonObj, Serial);

	String testName = jsonObj["testName"];
	String loadLevel = jsonObj["loadLevel"];
	TestType testType = getTestTypeFromString(testName);
	LoadPercentage loadPercentage = getLoadLevelFromString(loadLevel);

	if(testType == static_cast<TestType>(0))
	{
		logger.log(LogLevel::ERROR, "Unknown testName: %s", testName.c_str());
		return;
	}

	if(loadPercentage == static_cast<LoadPercentage>(-1))
	{
		logger.log(LogLevel::ERROR, "Invalid loadLevel: %s", loadLevel.c_str());
		return;
	}

	// Check if the test with the same testType and loadPercentage already exists
	for(int i = 0; i < MAX_TEST; i++)
	{
		if(_testList[i].testType == testType && _testList[i].loadLevel == loadPercentage &&
		   _testList[i].isActive)
		{
			logger.log(LogLevel::WARNING,
					   "Test with the same type and load level already exists. No new test added.");
			logger.log(LogLevel::INFO, "Existing TestName: %s",
					   testTypeToString(_testList[i].testType));
			logger.log(LogLevel::INFO, "Existing LoadLevel percent: %s",
					   loadPercentageToString(_testList[i].loadLevel));
			return;
		}
	}

	// If no duplicate was found, add the new test
	for(int i = 0; i < MAX_TEST; i++)
	{
		if(!_testList[i].isActive)
		{
			_testList[i].testId = _testID[i];
			_testList[i].testType = testType;
			_testList[i].loadLevel = loadPercentage;
			_testList[i].isActive = true;
			_testCount = _testCount + 1;

			logger.log(LogLevel::SUCCESS, "Received new test requirement in Test Sync");
			logger.log(LogLevel::INFO, "TestName: %s", testTypeToString(_testList[i].testType));
			logger.log(LogLevel::INFO, "LoadLevel percent: %s",
					   loadPercentageToString(_testList[i].loadLevel));

			handleUserUpdate(UserUpdateEvent::NEW_TEST);
			return;
		}
	}

	logger.log(LogLevel::WARNING, "Test list is full, unable to add new test.");
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
			logger.log(LogLevel::INFO, "TestName:%s", testTypeToString(_testList[i].testType));
			logger.log(LogLevel::INFO, "LoadLevel percent:%s",
					   loadPercentageToString(_testList[i].loadLevel));
			_testList[i] = {};
			testDeleted = true;
		}
	}

	if(testDeleted)
	{
		handleUserUpdate(UserUpdateEvent::DELETE_TEST);
	}
}

void TestSync::transferTest()
{
	// Get the TestManager instance
	TestManager& testManager = TestManager::getInstance();

	// Count the number of active tests in _testList
	int activeTestCount = 0;
	for(int i = 0; i < MAX_TEST; ++i)
	{
		if(_testList[i].isActive)
		{
			activeTestCount++;
		}
	}

	// If there are no active tests, return early
	if(activeTestCount == 0)
	{
		logger.log(LogLevel::INFO, "No active tests to transfer.");
		return;
	}

	// Create a temporary array to hold the active tests
	RequiredTest activeTests[activeTestCount];
	int index = 0;
	for(int i = 0; i < MAX_TEST; ++i)
	{
		if(_testList[i].isActive)
		{
			activeTests[index++] = _testList[i];
		}
	}

	// Transfer the active tests to the TestManager
	testManager.addTests(activeTests, activeTestCount);

	logger.log(LogLevel::SUCCESS, "Transferred all active tests to TestManager.");
}

void TestSync::handleUserUpdate(UserUpdateEvent update)
{
	TestSync& SyncTest = TestSync::getInstance();
	switch(update)
	{
		case UserUpdateEvent::NEW_TEST:
			EventHelper::setBits(UserUpdateEvent::NEW_TEST);
			EventHelper::clearBits(UserUpdateEvent::DELETE_TEST);

			break;
		case UserUpdateEvent::DELETE_TEST:
			EventHelper::setBits(UserUpdateEvent::DELETE_TEST);
			EventHelper::clearBits(UserUpdateEvent::NEW_TEST);

			break;
		case UserUpdateEvent::DATA_ENTRY:
			EventHelper::setBits(UserUpdateEvent::DATA_ENTRY);
			break;
		case UserUpdateEvent::USER_TUNE:
			EventHelper::setBits(UserUpdateEvent::USER_TUNE);
			break;
		default:
			break;
	}
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

void TestSync::userCommandTask(void* pvParameters)
{
	TestSync& instance = TestSync::getInstance();

	const EventBits_t CMD_BITS_MASK = static_cast<EventBits_t>(UserCommandEvent::START) |
									  static_cast<EventBits_t>(UserCommandEvent::STOP) |
									  static_cast<EventBits_t>(UserCommandEvent::AUTO) |
									  static_cast<EventBits_t>(UserCommandEvent::MANUAL) |
									  static_cast<EventBits_t>(UserCommandEvent::PAUSE) |
									  static_cast<EventBits_t>(UserCommandEvent::RESUME);

	State syncState = StateMachine::getInstance().getCurrentState();
	logger.log(LogLevel::INFO, "Sync Class state is:%s", stateToString(syncState));

	while(xEventGroupWaitBits(EventHelper::userCommandEventGroup, CMD_BITS_MASK, pdFALSE, pdFALSE,
							  portMAX_DELAY))
	{
		int cmdResult = xEventGroupGetBits(EventHelper::userCommandEventGroup);
		logger.log(LogLevel::SUCCESS, "New User Command Received");

		if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::AUTO)) != 0)
		{
			StateMachine::getInstance().handleMode(TestMode::AUTO);
			instance.handleSyncCommand(SyncCommand::START_OBSERVER);
			instance.acknowledgeCMD();
			logger.log(LogLevel::WARNING, "clearing AUTO command bits");
			EventHelper::clearBits(UserCommandEvent::AUTO);
		}
		else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::MANUAL)) != 0)
		{
			StateMachine::getInstance().handleMode(TestMode::MANUAL);
			instance.handleSyncCommand(SyncCommand::STOP_OBSERVER);
			instance.acknowledgeCMD();
			logger.log(LogLevel::WARNING, "clearing MANUAL command bits");
			EventHelper::clearBits(UserCommandEvent::MANUAL);
		}
		else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::START)) != 0)

		{
			logger.log(LogLevel::INFO, "Reporting START Event--->");
			instance.reportEvent(Event::START);
			logger.log(LogLevel::INFO, "Activating Manager--->");
			instance.handleSyncCommand(SyncCommand::MANAGER_ACTIVE);
			if(StateMachine::getInstance().isManualMode())
			{
				logger.log(LogLevel::INFO, "Starting first manual test--->");
				instance.startTest(instance._testList[0].testType);
			}
			instance.acknowledgeCMD();
			logger.log(LogLevel::INFO, "Clearing Start bit after acknowledgement--->");
			EventHelper::clearBits(UserCommandEvent::START);
		}
		else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::STOP)) != 0)
		{
			logger.log(LogLevel::INFO, "Stopping current test in AUTO Mode--->");
			instance.UserStopTest();
			logger.log(LogLevel::INFO, "Stopping Observer--->");
			instance.handleSyncCommand(SyncCommand::STOP_OBSERVER);
			logger.log(LogLevel::INFO, "Reporting STOP Event--->");
			instance.reportEvent(Event::STOP);

			if(StateMachine::getInstance().isManualMode())
			{
				logger.log(LogLevel::INFO, "Stopping First test in manual Mode--->");
				instance.stopTest(instance._testList[0].testType);
			}
			else
			{
				logger.log(LogLevel::INFO, "Making Manager Wait--->");
				instance.handleSyncCommand(SyncCommand::MANAGER_WAIT);
			}
			instance.acknowledgeCMD();
			logger.log(LogLevel::INFO, "Clearing Stop bit after acknowledgement--->");
			EventHelper::clearBits(UserCommandEvent::STOP);
		}
		else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::PAUSE)) != 0)
		{
			instance.UserStopTest();
			instance.handleSyncCommand(SyncCommand::STOP_OBSERVER);
			instance.handleSyncCommand(SyncCommand::MANAGER_WAIT);
			instance.acknowledgeCMD();
		}
		else if((cmdResult & static_cast<EventBits_t>(UserCommandEvent::RESUME)) != 0)
		{
			instance.handleSyncCommand(SyncCommand::START_OBSERVER);
			instance.handleSyncCommand(SyncCommand::MANAGER_ACTIVE);
			instance.acknowledgeCMD();
		}
		vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL);
}

void TestSync::userUpdateTask(void* pvParameters)
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

		if((updateBits & static_cast<EventBits_t>(UserUpdateEvent::NEW_TEST)) != 0)
		{
			instance.enableCurrentTest();

			TestManager& manager = TestManager::getInstance();
			logger.log(LogLevel::WARNING, "Adding Test to manager");
			instance.transferTest();
			instance.reportEvent(Event::NEW_TEST);
			vTaskDelay(pdMS_TO_TICKS(200));
			xTaskNotify(DataHandler::getInstance().PeriodicDataHandle,
						static_cast<uint32_t>(UserUpdateEvent::NEW_TEST), eSetBits);

			logger.log(LogLevel::TEST, "Is state changed?");
			instance.acknowledgeCMD();
			EventHelper::clearBits(UserUpdateEvent::NEW_TEST);
		}
		else if((updateBits & static_cast<EventBits_t>(UserUpdateEvent::DELETE_TEST)) != 0)
		{
			instance.disableCurrentTest();

			logger.log(LogLevel::TEST, "Invoking State Change...");
			instance.reportEvent(Event::DELETE_TEST);
			// xTaskNotify(DataHandler::getInstance().dataTaskHandler,
			// 			static_cast<uint32_t>(UserUpdateEvent::DELETE_TEST), eNoAction);

			instance.acknowledgeCMD();
			EventHelper::clearBits(UserUpdateEvent::DELETE_TEST);
		}

		vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL);
}

void TestSync::testSyncTask(void* pvParameters)
{
	TestSync& instance = TestSync::getInstance();
	while(xEventGroupWaitBits(EventHelper::syncControlEvent,
							  static_cast<EventBits_t>(SyncCommand::START_OBSERVER), pdFALSE,
							  pdFALSE, portMAX_DELAY))
	{
		logger.log(LogLevel::INFO, "Observing test.. ");

		vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL);
}