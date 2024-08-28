#ifndef TEST_MANAGER_H
#define TEST_MANAGER_H
#include "Arduino.h"
#include "HardwareConfig.h"
#include "Settings.h"
#include "StateMachine.h"
#include "TestData.h"
#include "Logger.h"
#include "TestSync.h"
#include "UPSError.h"
#include "UPSTesterSetup.h"
#include "UPSTest.h"

extern void IRAM_ATTR keyISR1(void* pvParameters);
extern void IRAM_ATTR keyISR2(void* pvParameters);
extern void IRAM_ATTR keyISR3(void* pvParameters);

using namespace Node_Core;
extern Logger& logger;
extern TestSync& SyncTest;

const uint16_t MAX_TESTS = 6;

enum class TestManagerStatus : EventBits_t
{
	NOT_IN_QUEUE = 1 << 0,
	PENDING = 1 << 1,
	RETEST = 1 << 2,
	DONE = 1 << 3
};

//
enum class TestOperatorStatus : EventBits_t
{
	NOT_STARTED = 1 << 4,
	RUNNING = 1 << 5,
	SUCCESS = 1 << 6,
	FAILED = 1 << 7
};

struct TestStatus
{
	TestManagerStatus managerStatus;
	TestOperatorStatus operatorStatus;
	TestStatus() :
		managerStatus(TestManagerStatus::PENDING), operatorStatus(TestOperatorStatus::NOT_STARTED)
	{
	}
};
// for outside use of  the class
struct RequiredTest
{
	int TestNo; // Unique test number
	TestType testtype;
	LoadPercentage loadlevel;
	bool addTest = true;
	RequiredTest() :
		TestNo(0), testtype(TestType::SwitchTest), loadlevel(LoadPercentage::LOAD_25P),
		addTest(true)
	{
	}
	RequiredTest(int testNo, TestType type, LoadPercentage level, bool add) :
		TestNo(testNo), testtype(type), loadlevel(level), addTest(add)
	{
	}
};

struct UPSTestRun
{
	RequiredTest testRequired;
	TestStatus testStatus;
	UPSTestRun() :
		testRequired(), // Calls the default constructor of RequiredTest
		testStatus() // Calls the default constructor of TestStatus
	{
	}
};

class TestManager
{
  public:
	static TestManager* getInstance();
	static void deleteInstance();

	void init();
	void addTests(RequiredTest testList[], int numTest);

	void runTests();
	void manageTests();
	void terminateTest();
	void triggerEvent(Event event);

	static void TestManagerTask(void* pvParameters);
	static void onMainsPowerLossTask(void* pvParameters);
	static void onUPSPowerGainTask(void* pvParameters);
	static void onUPSPowerLossTask(void* pvParameters);
	void initializeTestInstances();
	void UpdateSettings();

  private:
	TestManager();
	~TestManager();
	static TestManager* instance;

	UPSTestRun _testList[MAX_TESTS];

	bool _initialized = false;
	bool _newEventTrigger = false;
	bool _setupUpdated = false;
	bool _addedSwitchTest = false;
	uint8_t _numSwitchTest = 0;
	uint8_t _numBackupTest = 0;

	State _stateManager;
	SetupSpec _cfgSpec;
	SetupTest _cfgTest;
	SetupTask _cfgTask;
	SetupTaskParams _cfgTaskParam;
	SetupHardware _cfgHardware;

	void setupPins();
	void configureInterrupts();
	void createISRTasks();
	void createTestTasks();
	void createManagerTasks();

	void pauseAllTest();

	// helper functions switchTest
	bool isTestPendingAndNotStarted(const UPSTestRun& test);
	void logPendingTest(const UPSTestRun& test);
	void configureTest(LoadPercentage load);

	template<typename T,typename U>
	void handleTestState(UPSTest<T,U>* testInstance, State managerState, int testIndex,
						 int TestPriority);

	TestManager(const TestManager&) = delete;
	TestManager& operator=(const TestManager&) = delete;
};

template<typename T , typename U>
void TestManager::handleTestState(UPSTest<T,U>* testInstance, State managerState, int testIndex,
								  int TestPriority)
{
	QueueHandle_t dataQueue = testInstance->getQueue();
	int i = testIndex;

	if(managerState == State::TEST_START)
	{
		logger.log(LogLevel::INFO, "Manager Task under test start phase");
		LoadPercentage load = instance->_testList[i].testRequired.loadlevel;
		instance->configureTest(load);
		testInstance->logTaskState(LogLevel::INFO);
		// TaskHandle_t taskHandle = testInstance->getTaskHandle();
		testInstance->setTaskPriority(TestPriority);

		logger.log(LogLevel::INFO, "Starting %s...", testInstance->testTypeName());
		SyncTest.startTest(testInstance->getTestType());
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	else if(managerState == State::TEST_IN_PROGRESS)
	{
		if(testInstance->isdataCaptureOk())
		{
			logger.log(LogLevel::SUCCESS, "Successful data capture.");
		}
		if(testInstance->isTestEnded())
		{
			logger.log(LogLevel::INFO, "Test Cycle ended.");
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	else if(managerState == State::CURRENT_TEST_CHECK)
	{
		logger.log(LogLevel::INFO,
							   "In check State  checking either test ended or data captured");
		if(testInstance->isdataCaptureOk())
		{
			logger.log(LogLevel::SUCCESS, "Successful data capture.");
		}
		testInstance->setTaskPriority(TestPriority);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	else if(managerState == State::CURRENT_TEST_OK)
	{
		logger.log(LogLevel::SUCCESS, "Received Test data");
		U dataBuff = testInstance->data();
		if(xQueueReceive(dataQueue, &dataBuff, 1000) == pdTRUE)
		{
			logger.log(LogLevel::SUCCESS, "Received Test data ");
			logger.log(LogLevel::INFO, "Stopping SwitchTest...");
			SyncTest.stopTest(testInstance->getTestType());
			instance->_testList[i].testStatus.managerStatus = TestManagerStatus::DONE;
			instance->_testList[i].testStatus.operatorStatus =
							TestOperatorStatus::SUCCESS;
			
			testInstance->markTestAsDone();
			logger.log(LogLevel::WARNING, "Triggering SAVE event from manager");
			instance->triggerEvent(Event::SAVE);
			vTaskDelay(pdMS_TO_TICKS(100));
		}
		else
		{
			logger.log(LogLevel::ERROR, "Receive Test data timeout");
			SyncTest.stopTest(testInstance->getTestType());
		}
	}

				else if(managerState == State::READY_NEXT_TEST)
				{
					logger.log(LogLevel::INFO, "Checking for next pending test...");

					bool pendingTestFound = false;

					// Check if there are any more pending tests
					for(int j = 0; j < instance->_numSwitchTest; ++j)
					{
						if(instance->isTestPendingAndNotStarted(instance->_testList[j]))
						{
							pendingTestFound = true;
							break;
						}
					}

					if(pendingTestFound)
					{
						logger.log(LogLevel::INFO,
								   "Pending test found. Preparing to start next test...");

						instance->triggerEvent(Event::PENDING_TEST_FOUND);
						vTaskDelay(pdMS_TO_TICKS(200));
					}
					else
					{
						logger.log(LogLevel::INFO, "No more pending tests.");
					}
				}
	else
	{
		logger.log(LogLevel::WARNING, "Unhandled state encountered.");
	}

	vTaskDelay(pdMS_TO_TICKS(100)); // Delay to avoid rapid state changes
}

#endif