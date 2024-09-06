#include "SwitchTest.h"
#include "BackupTest.h"
#include "TesterMemory.h"
#include "EventHelper.h"

extern Logger& logger;

extern void IRAM_ATTR keyISR1(void* pvParameters);
extern void IRAM_ATTR keyISR2(void* pvParameters);
extern void IRAM_ATTR keyISR3(void* pvParameters);

using namespace Node_Core;

extern TaskHandle_t TestManagerTaskHandle;
extern TaskHandle_t switchTestTaskHandle;
extern TaskHandle_t backupTestTaskHandle;
extern TaskHandle_t efficiencyTestTaskHandle;
extern TaskHandle_t inputvoltageTestTaskHandle;
extern TaskHandle_t waveformTestTaskHandle;
extern TaskHandle_t tunepwmTestTaskHandle;
extern TaskHandle_t ISR_MAINS_POWER_LOSS;
extern TaskHandle_t ISR_UPS_POWER_GAIN;
extern TaskHandle_t ISR_UPS_POWER_LOSS;

extern SemaphoreHandle_t mainLoss;
extern SemaphoreHandle_t upsLoss;
extern SemaphoreHandle_t upsGain;

extern QueueHandle_t TestManageQueue;
extern QueueHandle_t SwitchTestDataQueue;
extern QueueHandle_t BackupTestDataQueue;

// extern SemaphoreHandle_t state_mutex;

TestManager::TestManager() :
	_initialized(false), _newEventTrigger(false), _setupUpdated(false), _numTest(0)
{
}

TestManager& TestManager::getInstance()
{
	static TestManager instance;

	return instance;
}

void TestManager::init()
{
	if(_initialized)
	{
		return; // Already initialized, do nothing
	}
	UpdateSettings();
	setupPins();
	// createISRTasks();
	initializeTestInstances();
	createManagerTasks();
	// createTestTasks();

	// pauseAllTest();

	_initialized = true; // Mark as initialized
}

void TestManager::UpdateSettings()
{
	_cfgSpec = TesterSetup.specSetup();
	_cfgTest = TesterSetup.testSetup();
	_cfgTask = TesterSetup.taskSetup();
	_cfgTaskParam = TesterSetup.paramSetup();
	_cfgHardware = TesterSetup.hardwareSetup();
	logger.log(LogLevel::SUCCESS, "Testmanager data updated");
}

State TestManager::refreshState()
{
	TestSync& SyncTest = TestSync::getInstance();
	_currentState.store(SyncTest.refreshState());
	return _currentState.load();
}

void TestManager::addTests(RequiredTest testList[], int testNum)
{
	if(testNum > MAX_TEST)
	{
		logger.log(LogLevel::ERROR, "Maximum Test Limit exeeded", MAX_TEST);
		return;
	}
	for(int i = 0; i < testNum; ++i)
	{
		_testList[_numTest].testRequired = testList[i];
		_testList[_numTest].testStatus.managerStatus = TestManagerStatus::PENDING;
		_testList[_numTest].testStatus.operatorStatus = TestOperatorStatus::NOT_STARTED;
		logger.log(LogLevel::SUCCESS, "Added test", testTypeToString(testList[i].testType));
		logger.log(LogLevel::INFO, "Test No: ", testList[i].testId);
		logger.log(LogLevel::INFO, "Load Level: ", loadPercentageToString(testList[i].loadLevel));
		_numTest++;
	}
}

void TestManager::pauseAllTest()
{
	if(switchTestTaskHandle != NULL)
	{
		logger.log(LogLevel::WARNING, "Pausing Switch test task");
		vTaskSuspend(switchTestTaskHandle);
	}
}
void TestManager::passEvent(Event event)
{
	TestSync& SyncTest = TestSync::getInstance();
	SyncTest.reportEvent(event);
}

void TestManager::setupPins()
{
	pinMode(SENSE_MAINS_POWER_PIN, INPUT_PULLDOWN);
	pinMode(SENSE_UPS_POWER_PIN, INPUT_PULLDOWN);
	pinMode(SENSE_UPS_POWER_DOWN, INPUT_PULLDOWN);

	pinMode(UPS_POWER_CUT_PIN, OUTPUT); // Set power cut pin as output
	pinMode(TEST_END_INT_PIN, OUTPUT);
	pinMode(LOAD_PWM_PIN, OUTPUT);
	pinMode(LOAD25P_ON_PIN, OUTPUT);
	pinMode(LOAD50P_ON_PIN, OUTPUT);
	pinMode(LOAD75P_ON_PIN, OUTPUT);
	pinMode(LOAD_FULL_ON_PIN, OUTPUT);
	pinMode(TEST_END_INT_PIN, OUTPUT);

	ledcSetup(_cfgHardware.pwmchannelNo, _cfgHardware.pwm_frequency,
			  _cfgHardware.pwmResolusion_bits);
	ledcWrite(_cfgHardware.pwmchannelNo, 0);
	ledcAttachPin(LOAD_PWM_PIN, 0);
	configureInterrupts();

	logger.log(LogLevel::SUCCESS, "Interrupts configured");
	pinMode(SENSE_MAINS_POWER_PIN, INPUT_PULLDOWN);
	pinMode(SENSE_UPS_POWER_PIN, INPUT_PULLDOWN);
	pinMode(SENSE_UPS_POWER_DOWN, INPUT_PULLDOWN);

	logger.log(LogLevel::SUCCESS, "Testmanager sets up all pins");
}

void TestManager::configureInterrupts()
{
	logger.log(LogLevel::INFO, "configuring Interrupts ");
	gpio_num_t mainpowerPin = static_cast<gpio_num_t>(SENSE_MAINS_POWER_PIN);
	gpio_num_t upspowerupPin = static_cast<gpio_num_t>(SENSE_UPS_POWER_PIN);
	gpio_num_t upsshutdownPin = static_cast<gpio_num_t>(SENSE_UPS_POWER_DOWN);
	gpio_install_isr_service(0);

	gpio_set_intr_type(mainpowerPin, GPIO_INTR_NEGEDGE);
	gpio_set_intr_type(upspowerupPin, GPIO_INTR_NEGEDGE);
	gpio_set_intr_type(upsshutdownPin, GPIO_INTR_NEGEDGE);

	gpio_isr_handler_add(mainpowerPin, keyISR1, NULL);
	gpio_isr_handler_add(upspowerupPin, keyISR2, NULL);
	gpio_isr_handler_add(upsshutdownPin, keyISR3, NULL);

	logger.log(LogLevel::SUCCESS, "Testmanager configured all interrupts");
}
void TestManager::createManagerTasks()
{
	logger.log(LogLevel::INFO, "Testmanager creating its own task");
	xTaskCreatePinnedToCore(TestManagerTask, "MainsTestManager", _cfgTask.mainTest_taskStack, NULL,
							2, &TestManagerTaskHandle, _cfgTask.mainTest_taskCore);
	logger.log(LogLevel::SUCCESS, "Testmanager task created");
}
void TestManager::createISRTasks()
{
	logger.log(LogLevel::INFO, "Testmanager creating its ISR task");
	xTaskCreatePinnedToCore(onMainsPowerLossTask, "MainslossISRTask", _cfgTask.mainsISR_taskStack,
							NULL, _cfgTask.mainsISR_taskIdlePriority, &ISR_MAINS_POWER_LOSS,
							_cfgTask.mainsISR_taskCore);

	logger.log(LogLevel::SUCCESS, "ISR1 task created");
	xTaskCreatePinnedToCore(onUPSPowerGainTask, "UPSgainISRTask", _cfgTask.upsISR_taskStack, NULL,
							_cfgTask.upsISR_taskIdlePriority, &ISR_UPS_POWER_GAIN,
							_cfgTask.upsISR_taskCore);
	logger.log(LogLevel::SUCCESS, "ISR2 task created");
	xTaskCreatePinnedToCore(onUPSPowerLossTask, "UPSLossISRTask", _cfgTask.upsISR_taskStack, NULL,
							_cfgTask.upsISR_taskIdlePriority, &ISR_UPS_POWER_LOSS,
							_cfgTask.upsISR_taskCore);
	logger.log(LogLevel::SUCCESS, "ISR3 task created");
	logger.log(LogLevel::SUCCESS, "All ISR task Created");
}
void TestManager::createTestTasks()
{
	logger.log(LogLevel::INFO, "Creating Switchtest task ");
	SwitchTest& switchTest = UPSTest<SwitchTest, SwitchTestData>::getInstance();
	BackupTest& backupTest = UPSTest<BackupTest, BackupTestData>::getInstance();
	xQueueSend(TestManageQueue, &_cfgTaskParam, 100);
	xTaskCreatePinnedToCore(switchTest.SwitchTestTask, "SwitchTestTask", 4096, NULL,
							_cfgTask.mainTest_taskIdlePriority, &switchTestTaskHandle, 0);
	logger.log(LogLevel::SUCCESS, "Switch Test task created");
	logger.log(LogLevel::INFO, "Creating Backuptest task ");
	xQueueSend(TestManageQueue, &_cfgTaskParam, 100);
	xTaskCreatePinnedToCore(backupTest.BackupTestTask, "BackUpTestTask", 4096, NULL,
							_cfgTask.mainTest_taskIdlePriority, &backupTestTaskHandle, 0);
	logger.log(LogLevel::SUCCESS, "Switch Test task created");
}

void TestManager::TestManagerTask(void* pvParameters)
{
	logger.log(LogLevel::INFO, "Resuming Test Manager task");

	while(xEventGroupWaitBits(EventHelper::syncControlEvent,
							  static_cast<EventBits_t>(SyncCommand::MANAGER_ACTIVE), pdFALSE,
							  pdTRUE, portMAX_DELAY))
	{
		TestManager& instance = TestManager::getInstance();
		State managerState = instance.refreshState();
		// xEventGroupWaitBits(EventHelper::syncControlEvent,
		// 					static_cast<EventBits_t>(SyncCommand::MANAGER_ACTIVE), pdFALSE, pdFALSE,
		// 					portMAX_DELAY);

		if(managerState == State::DEVICE_READY)
		{
			managerState = instance.refreshState();
			logger.log(LogLevel::INFO, "Manager state is:%s", stateToString(managerState));
			// Serial.print(stateToString(managerState));
			vTaskDelay(pdMS_TO_TICKS(100));
		}

		for(int i = 0; i < instance._numTest; ++i)
		{
			if(instance.isTestPendingAndNotStarted(instance._testList[i]))
			{
				managerState = instance.refreshState();

				TestType testType = instance._testList[i].testRequired.testType;
				bool success = false;

				if(testType == TestType::SwitchTest)
				{
					managerState = instance.refreshState();
					SwitchTestData dataBuff1;
					success = instance.handleTestState(SwitchTest::getInstance(), managerState, i,
													   &dataBuff1);

					if(success && managerState == State::CURRENT_TEST_OK)
					{
						logger.log(LogLevel::SUCCESS, "Test Data recived");

						logger.log(LogLevel::SUCCESS,
								   "Test Report switch time:", dataBuff1.switchTest->switchtime);
					}
					else if(!success && managerState == State::CURRENT_TEST_OK)
					{
						logger.log(LogLevel::ERROR, "Test Data not recived");
					}
					else
					{
						logger.log(LogLevel::INFO, "Manager observing test..");
					}
				}
				else if(testType == TestType::BackupTest)
				{
					managerState = instance.refreshState();
					BackupTestData dataBuff2;
					success = instance.handleTestState(BackupTest::getInstance(), managerState, i,
													   &dataBuff2);

					if(success && managerState == State::CURRENT_TEST_OK)
					{
						logger.log(LogLevel::SUCCESS, "Test Data recived");

						logger.log(LogLevel::SUCCESS,
								   "Test Report backup time:", dataBuff2.backupTest->backuptime);
					}
					else if(!success && managerState == State::CURRENT_TEST_OK)
					{
						logger.log(LogLevel::ERROR, "Test Data not recived");
					}
					else
					{
						logger.log(LogLevel::INFO, "Manager observing test..");
					}
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(100)); // General delay for task
	}

	vTaskDelete(NULL); // Delete the task when finished
}

void TestManager::onMainsPowerLossTask(void* pvParameters)
{
	while(true)
	{
		if(xSemaphoreTake(mainLoss, portMAX_DELAY))
		{
			SwitchTest& switchTest = UPSTest<SwitchTest, SwitchTestData>::getInstance();
			BackupTest& backupTest = UPSTest<BackupTest, BackupTestData>::getInstance();
			// vTaskPrioritySet(&ISR_MAINS_POWER_LOSS, 3);
			if(switchTest.isTestRunning())
			{
				switchTest._dataCaptureRunning_SW = true;
				logger.log(LogLevel::INTR, "mains Powerloss triggered...");
				logger.log(LogLevel::TEST, " Switch Test DataCapture starts...");
				switchTest.startTestCapture();
			}
			if(backupTest.isTestRunning())
			{
				backupTest._dataCaptureRunning_BT = true;
				logger.log(LogLevel::INTR, "mains Powerloss triggered...");
				logger.log(LogLevel::TEST, " Backup Test DataCapture starts...");
				backupTest.startTestCapture();
			}

			logger.log(LogLevel::INFO,
					   "mains task High Water Mark:", uxTaskGetStackHighWaterMark(NULL));
			vTaskDelay(pdMS_TO_TICKS(100)); // Task delay
		}
	}
	vTaskDelete(NULL);
}

// // ISR for UPS power gain
void TestManager::onUPSPowerGainTask(void* pvParameters)
{
	while(true)
	{
		if(xSemaphoreTake(upsGain, portMAX_DELAY))
		{
			SwitchTest& switchTest = UPSTest<SwitchTest, SwitchTestData>::getInstance();

			if(switchTest.isTestRunning())
			{
				logger.log(LogLevel::INTR, "UPS Power gain triggered...");
				switchTest.stopTestCapture();
			}
			logger.log(LogLevel::INFO, "UPS High Water Mark:", uxTaskGetStackHighWaterMark(NULL));

			vTaskDelay(pdMS_TO_TICKS(100)); // Task delay
		}
	}
	vTaskDelete(NULL);
}

void TestManager::onUPSPowerLossTask(void* pvParameters)
{
	while(true)
	{
		if(xSemaphoreTake(upsLoss, portMAX_DELAY))
		{
			BackupTest& backupTest = UPSTest<BackupTest, BackupTestData>::getInstance();
			if(backupTest.isTestRunning())

			{
				logger.log(LogLevel::INTR, "UPS lost power triggered...");
				backupTest.stopTestCapture();
			}
			logger.log(LogLevel::INFO, "UPS High Water Mark:", uxTaskGetStackHighWaterMark(NULL));

			vTaskDelay(pdMS_TO_TICKS(100)); // Task delay
		}
	}
	vTaskDelete(NULL);
}

void TestManager::initializeTestInstances()
{
	SwitchTest& switchTest = UPSTest<SwitchTest, SwitchTestData>::getInstance();
	BackupTest& backupTest = UPSTest<BackupTest, BackupTestData>::getInstance();
	switchTest.init();

	backupTest.init();
}

bool TestManager::isTestPendingAndNotStarted(const UPSTestRun& test)
{
	return test.testStatus.managerStatus == TestManagerStatus::PENDING &&
		   test.testStatus.operatorStatus == TestOperatorStatus::NOT_STARTED;
}

void TestManager::configureTest(LoadPercentage load)
{
	SetupTaskParams task_Param;
	task_Param = TesterSetup.paramSetup();
	switch(load)
	{
		case LoadPercentage::LOAD_0P:

			task_Param.task_TestVARating = 0;
			logger.log(LogLevel::INFO, "Setup switch test for Noload");
			break;
		case LoadPercentage::LOAD_25P:

			task_Param.task_TestVARating = _cfgTest.testVARating * 25 / 100;
			logger.log(LogLevel::INFO, "Setup switch test at 25 percent load");
			break;
		case LoadPercentage::LOAD_50P:

			task_Param.task_TestVARating = _cfgTest.testVARating * 50 / 100;
			logger.log(LogLevel::INFO, "Setup switch test at 50 percent load");
			break;
		case LoadPercentage::LOAD_75P:

			task_Param.task_TestVARating = _cfgTest.testVARating * 75 / 100;
			logger.log(LogLevel::INFO, "Setup switch test at 75 percent load");
			break;
		case LoadPercentage::LOAD_100P:

			task_Param.task_TestVARating = _cfgTest.testVARating;
			logger.log(LogLevel::INFO, "Setup switch test at 100 percent load");
			break;
		default:
			logger.log(LogLevel::ERROR, "Unknown load percentage");
			break;
	}

	// Send the task parameters to the queue
	xQueueSend(TestManageQueue, &task_Param, 100);
}

void TestManager::logPendingTest(const UPSTestRun& test)
{
	LoadPercentage load = test.testRequired.loadLevel;
	TestType type = test.testRequired.testType;

	logger.log(LogLevel::INFO, "Pending Test Load level: %s", loadPercentageToString(load));
	logger.log(LogLevel::INFO, "Pending Test name: %s", testTypeToString(type));
	logger.log(LogLevel::INFO, "SwitchTest list is updated now.");
}
template<typename T, typename U>
bool TestManager::handleTestState(UPSTest<T, U>& testInstance, State managerState, int testIndex,
								  U* dataBuff)
{
	TestManager& instance = TestManager::getInstance();
	QueueHandle_t dataQueue = testInstance.getQueue();
	int i = testIndex;
	int TestPriority = 1;
	if(managerState == State::DEVICE_READY)
	{
		instance.logPendingTest(instance._testList[i]);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	else if(managerState == State::READY_TO_PROCEED)
	{
		instance.logPendingTest(instance._testList[i]);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	else if(managerState == State::TEST_START)
	{
		TestPriority = 3;
		logger.log(LogLevel::INFO, "Manager Task under test start phase");
		LoadPercentage load = instance._testList[i].testRequired.loadLevel;
		instance.configureTest(load);
		testInstance.logTaskState(LogLevel::INFO);

		testInstance.setTaskPriority(TestPriority);

		logger.log(LogLevel::INFO, "Starting %s...", testInstance.testTypeName());
		// SyncTest.startTest(testInstance.getTestType());
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	else if(managerState == State::TEST_RUNNING)
	{
		if(testInstance.isdataCaptureOk())
		{
			logger.log(LogLevel::SUCCESS, "Successful data capture.");
		}
		if(testInstance.isTestEnded())
		{
			logger.log(LogLevel::INFO, "Test Cycle ended.");
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	else if(managerState == State::CURRENT_TEST_CHECK)
	{
		TestPriority = 2;
		logger.log(LogLevel::INFO, "In check State  checking either test ended or data captured");
		if(testInstance.isdataCaptureOk())
		{
			logger.log(LogLevel::SUCCESS, "Successful data capture.");
		}
		testInstance.setTaskPriority(TestPriority);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	else if(managerState == State::CURRENT_TEST_OK)
	{
		TestPriority = 1;
		logger.log(LogLevel::SUCCESS, "Received Test data");

		if(dataBuff && xQueueReceive(dataQueue, dataBuff, 1000) == pdTRUE)
		{
			logger.log(LogLevel::INFO, "Stopping SwitchTest...");
			// SyncTest.stopTest(testInstance.getTestType());
			instance._testList[i].testStatus.managerStatus = TestManagerStatus::DONE;
			instance._testList[i].testStatus.operatorStatus = TestOperatorStatus::SUCCESS;

			testInstance.markTestAsDone();
			testInstance.setTaskPriority(TestPriority);
			logger.log(LogLevel::WARNING, "Triggering SAVE event from manager");
			instance.passEvent(Event::SAVE);
			vTaskDelay(pdMS_TO_TICKS(100));
			return true;
		}
		else
		{
			logger.log(LogLevel::ERROR, "Receive Test data timeout");
			//	SyncTest.stopTest(testInstance.getTestType());
		}
	}

	else if(managerState == State::READY_NEXT_TEST)
	{
		logger.log(LogLevel::INFO, "Checking for next pending test...");

		bool pendingTestFound = false;

		// Check if there are any more pending tests
		for(int j = 0; j < instance._numTest; ++j)
		{
			if(instance.isTestPendingAndNotStarted(instance._testList[j]))
			{
				pendingTestFound = true;
				break;
			}
		}

		if(pendingTestFound)
		{
			logger.log(LogLevel::INFO, "Pending test found. Preparing to start next test...");

			instance.passEvent(Event::PENDING_TEST_FOUND);
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
	return false;
}