#include "SwitchTest.h"
#include "BackupTest.h"

extern SwitchTest* switchTest;
extern BackupTest* backupTest;

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

TestManager* TestManager::instance = nullptr;

TestManager::TestManager() : _initialized(false), _newEventTrigger(false), _setupUpdated(false)
{
	instance->UpdateSettings();
}

TestManager::~TestManager()
{
}

TestManager* TestManager::getInstance()
{
	if(instance == nullptr)
	{
		instance = new TestManager();
	}
	return instance;
}

void TestManager::deleteInstance()
{
	delete instance;
	instance = nullptr;
}

void TestManager::init()
{
	if(_initialized)
	{
		return; // Already initialized, do nothing
	}
	setupPins();
	createISRTasks();
	initializeTestInstances();
	createManagerTasks();
	createTestTasks();

	// pauseAllTest();

	_initialized = true; // Mark as initialized
}

void TestManager::UpdateSettings()
{
	if(TesterSetup)
	{
		_cfgSpec = TesterSetup->specSetup();
		_cfgTest = TesterSetup->testSetup();
		_cfgTask = TesterSetup->taskSetup();
		_cfgTaskParam = TesterSetup->paramSetup();
		_cfgHardware = TesterSetup->hardwareSetup();
	};
}

void TestManager::addTests(RequiredTest testList[], int numTest)
{
	if(numTest > MAX_TESTS)
	{
		logger.log(LogLevel::ERROR, "Maximum Test Limit exeeded", MAX_TESTS);
		return;
	}
	for(int i = 0; i < numTest; ++i)
	{
		logger.log(LogLevel::INFO, "Test No: ", testList[i].TestNo);
		logger.log(LogLevel::INFO, "Test Type:%s ", testTypeToString(testList[i].testtype));
		logger.log(LogLevel::INFO, "Load Level:%s ", loadPercentageToString(testList[i].loadlevel));

		if(testList[i].testtype == TestType::SwitchTest)
		{
			_testList[_numSwitchTest].testRequired = testList[i];
			_testList[_numSwitchTest].testStatus.managerStatus = TestManagerStatus::PENDING;
			_testList[_numSwitchTest].testStatus.operatorStatus = TestOperatorStatus::NOT_STARTED;
			logger.log(LogLevel::SUCCESS, "Added test", testTypeToString(testList[i].testtype));

			_addedSwitchTest = true;
			_numSwitchTest++;
		}
		else
		{
			logger.log(LogLevel::WARNING, "Other Test still not available");
		}
	}
}

void TestManager::pauseAllTest()
{
	if(switchTest && switchTestTaskHandle != NULL)
	{
		logger.log(LogLevel::WARNING, "Pausing Switch test task");
		vTaskSuspend(switchTestTaskHandle);
	}
}
void TestManager::triggerEvent(Event event)
{
	SyncTest.triggerEvent(event);
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
}

void TestManager::configureInterrupts()
{
	logger.log(LogLevel::INFO, "configuring Interrupts ");
	gpio_num_t mainpowerPin = static_cast<gpio_num_t>(SENSE_MAINS_POWER_PIN);
	gpio_num_t upspowerupPin = static_cast<gpio_num_t>(SENSE_UPS_POWER_PIN);
	gpio_num_t upsshutdownPin = static_cast<gpio_num_t>(SENSE_UPS_POWER_DOWN);
	gpio_install_isr_service(0);

	gpio_set_intr_type(mainpowerPin, GPIO_INTR_NEGEDGE);
	gpio_set_intr_type(upspowerupPin, GPIO_INTR_POSEDGE);
	gpio_set_intr_type(upsshutdownPin, GPIO_INTR_NEGEDGE);

	gpio_isr_handler_add(mainpowerPin, keyISR1, NULL);
	gpio_isr_handler_add(upspowerupPin, keyISR2, NULL);
	gpio_isr_handler_add(upsshutdownPin, keyISR3, NULL);
}
void TestManager::createManagerTasks()
{
	xTaskCreatePinnedToCore(TestManagerTask, "MainsTestManager", _cfgTask.mainTest_taskStack, NULL,
							2, &TestManagerTaskHandle, _cfgTask.mainTest_taskCore);
}
void TestManager::createISRTasks()
{
	xTaskCreatePinnedToCore(onMainsPowerLossTask, "MainslossISRTask", _cfgTask.mainsISR_taskStack,
							NULL, _cfgTask.mainsISR_taskIdlePriority, &ISR_MAINS_POWER_LOSS,
							_cfgTask.mainsISR_taskCore);

	xTaskCreatePinnedToCore(onUPSPowerGainTask, "UPSgainISRTask", _cfgTask.upsISR_taskStack, NULL,
							_cfgTask.upsISR_taskIdlePriority, &ISR_UPS_POWER_GAIN,
							_cfgTask.upsISR_taskCore);

	xTaskCreatePinnedToCore(onUPSPowerLossTask, "UPSLossISRTask", _cfgTask.upsISR_taskStack, NULL,
							_cfgTask.upsISR_taskIdlePriority, &ISR_UPS_POWER_LOSS,
							_cfgTask.upsISR_taskCore);

	logger.log(LogLevel::SUCCESS, "All ISR task Created");
}
void TestManager::createTestTasks()
{
	if(switchTest)
	{
		logger.log(LogLevel::INFO, "Creating Switchtest task ");

		xQueueSend(TestManageQueue, &_cfgTaskParam, 100);

		xTaskCreatePinnedToCore(switchTest->SwitchTestTask, "MainsTestManager", 12000, NULL,
								_cfgTask.mainTest_taskIdlePriority, &switchTestTaskHandle, 0);

		eTaskState state = eTaskGetState(switchTestTaskHandle);
		logger.log(LogLevel::INFO, "SwitchTest task state: %s", etaskStatetoString(state));
	}
	if(backupTest)
	{
		logger.log(LogLevel::INFO, "Creating Backuptest task ");

		xQueueSend(TestManageQueue, &_cfgTaskParam, 100);

		xTaskCreatePinnedToCore(backupTest->BackupTestTask, "MainsTestManager", 12000, NULL,
								_cfgTask.mainTest_taskIdlePriority, &backupTestTaskHandle, 0);

		eTaskState state = eTaskGetState(backupTestTaskHandle);
		logger.log(LogLevel::INFO, "BackupTest task state: %s", etaskStatetoString(state));
	}
}

void TestManager::TestManagerTask(void* pvParameters)
{
	logger.log(LogLevel::INFO, "Resuming Test Manager task");

	while(true)
	{
		State managerState = SyncTest.getState();

		if(managerState == State::DEVICE_READY)
		{
			logger.log(LogLevel::INFO, "Device is ready. Checking pending tests...");
		}

		for(int i = 0; i < instance->_numSwitchTest; ++i)
		{
			if(instance->isTestPendingAndNotStarted(instance->_testList[i]))
			{
				managerState = SyncTest.getState();

				TestType testType = instance->_testList[i].testRequired.testtype;
				bool success = false;

				if(testType == TestType::SwitchTest)
				{
					SwitchTestData dataBuff1;
					success = instance->handleTestState(SwitchTest::getInstance(), managerState, i,
														&dataBuff1);
				}
				else if(testType == TestType::BackupTest)
				{
					BackupTestData dataBuff2;
					success = instance->handleTestState(BackupTest::getInstance(), managerState, i,
														&dataBuff2);
				}

				if(success && managerState == State::CURRENT_TEST_OK)
				{
					logger.log(LogLevel::SUCCESS, "Test Data recived");
					// logger.log(LogLevel::SUCCESS,
					// 		   "Test Report switch time:", dataBuff1.switchTest->switchtime);
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
			// vTaskPrioritySet(&ISR_MAINS_POWER_LOSS, 3);
			if(switchTest->isTestRunning())
			{
				switchTest->_dataCaptureRunning_SW = true;
				logger.log(LogLevel::INTR, "mains Powerloss triggered...");
				logger.log(LogLevel::TEST, " Switch Test DataCapture...");
				switchTest->startTestCapture();
			}
			if(backupTest->isTestRunning())
			{
				backupTest->_dataCaptureRunning_BT = true;
				logger.log(LogLevel::INTR, "mains Powerloss triggered...");
				logger.log(LogLevel::TEST, " Backup Test DataCapture...");
				backupTest->startTestCapture();
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
			if(switchTest->isTestRunning())
			{
				logger.log(LogLevel::INTR, "UPS Power gain triggered...");
				switchTest->stopTestCapture();
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
			if(backupTest->isTestRunning())

			{
				logger.log(LogLevel::INTR, "UPS lost power triggered...");
				backupTest->stopTestCapture();
			}
			logger.log(LogLevel::INFO, "UPS High Water Mark:", uxTaskGetStackHighWaterMark(NULL));

			vTaskDelay(pdMS_TO_TICKS(100)); // Task delay
		}
	}
	vTaskDelete(NULL);
}

void TestManager::initializeTestInstances()
{
	if(switchTest)
	{
		switchTest->init();
	}
	if(backupTest)
	{
		backupTest->init();
	}
}

bool TestManager::isTestPendingAndNotStarted(const UPSTestRun& test)
{
	return test.testStatus.managerStatus == TestManagerStatus::PENDING &&
		   test.testStatus.operatorStatus == TestOperatorStatus::NOT_STARTED;
}

void TestManager::configureTest(LoadPercentage load)
{
	SetupTaskParams task_Param;
	task_Param = TesterSetup->paramSetup();
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
	LoadPercentage load = test.testRequired.loadlevel;
	TestType type = test.testRequired.testtype;

	logger.log(LogLevel::INFO, "Pending Test Load level: %s", loadPercentageToString(load));
	logger.log(LogLevel::INFO, "Pending Test name: %s", testTypeToString(type));
	logger.log(LogLevel::INFO, "SwitchTest list is updated now.");
}
