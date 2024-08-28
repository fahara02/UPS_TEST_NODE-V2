#include "BackupTest.h"

extern TestManager* Manager;

extern EventGroupHandle_t eventGroupTest;

extern QueueHandle_t TestManageQueue;

using namespace Node_Core;
extern BackupTest* backupTest;

BackupTest::~BackupTest()
{
	if(backupTestTaskHandle != NULL)
	{
		vTaskDelete(backupTestTaskHandle);
		backupTestTaskHandle = NULL;
	}
}

void BackupTest::init()
{
	if(!_initialized_BT)
	{
		for(auto& test: _data_BT.backupTest)
		{
			test.testNo = 0;
			test.testTimestamp = 0;
			test.valid_data = false;
			test.backuptime = 0;
			test.starttime = 0;
			test.endtime = 0;
			test.load_percentage = LoadPercentage::LOAD_0P;
		};
		_initialized_BT = true;

		UpdateSettings();
	}
}

void BackupTest::UpdateSettings()
{
	if(TesterSetup)
	{
		backupTest->_cfgSpec_BT = TesterSetup->specSetup();
		backupTest->_cfgTest_BT = TesterSetup->testSetup();
		backupTest->_cfgTask_BT = TesterSetup->taskSetup();
		backupTest->_cfgTaskParam_BT = TesterSetup->paramSetup();
		backupTest->_cfgHardware_BT = TesterSetup->hardwareSetup();
	};
}

BackupTestData& BackupTest::data()
{
	return _data_BT;
}

// Function for BackupTest task
void BackupTest::BackupTestTask(void* pvParameters)
{
	SetupTaskParams taskParam;
	xQueueReceive(TestManageQueue, (void*)&taskParam, 0 == pdTRUE);

	while(xEventGroupWaitBits(eventGroupTest, static_cast<EventBits_t>(TestType::BackupTest),
							  pdFALSE, pdTRUE, portMAX_DELAY))
	{
		logger.log(LogLevel::INFO, "resuming backupTest task");

		EventBits_t bt_eventbits = static_cast<EventBits_t>(TestType::BackupTest);
		int result = xEventGroupGetBits(eventGroupTest);

		if((result & bt_eventbits) != 0)
		{
			if(backupTest->_currentTestResult == TestResult::TEST_SUCCESSFUL)
			{
				backupTest->_sendTestData_BT = true;
			}

			if(!backupTest->_sendTestData_BT)
			{
				xQueueReceive(TestManageQueue, (void*)&taskParam, 0 == pdTRUE);
				logger.log(LogLevel::TEST,
						   "Backuptask VA rating is: ", taskParam.task_TestVARating);
				logger.log(LogLevel::TEST,
						   "Backuptask duration is: ", taskParam.task_testDuration_ms);

				backupTest->_currentTestResult =
					backupTest->run(taskParam.task_TestVARating, taskParam.task_testDuration_ms);
			}
			if(backupTest->_sendTestData_BT)
			{
				int retrySend = 0;
				BackupTestData testData = backupTest->data();
				if(xQueueSend(BackupTestDataQueue, &testData, 100) == pdTRUE)
				{
					backupTest->_currentTestResult = TestResult::TEST_PENDING;
					logger.log(LogLevel::SUCCESS, "test data sending complete");
					retrySend = 0;
				}
				else if(retrySend <= 3)
				{
					logger.log(LogLevel::WARNING, "retrying send data...");
					retrySend = retrySend + 1;
				}
				else
				{
					logger.log(LogLevel::ERROR, "test data sending failed");
					backupTest->_sendTestData_BT = false;
				}
			}
			logger.log(LogLevel::WARNING, "High Water mark ", uxTaskGetStackHighWaterMark(NULL));
		}

		vTaskDelay(pdMS_TO_TICKS(200)); // Delay to prevent tight loop
	}
	vTaskDelete(NULL);
}

// Start the test
void BackupTest::startTestCapture()
{
	if(_dataCaptureRunning_BT)
	{
		_data_BT.backupTest[_currentTest_BT].starttime = millis();
		_dataCaptureOk_BT = false;
		logger.log(LogLevel::TEST,
				   " time captured, starttime:", _data_BT.backupTest[_currentTest_BT].starttime);
	}
	else
	{
		logger.log(LogLevel::ERROR, "Debounce ");
	}
}

// Stop the test
void BackupTest::stopTestCapture()
{
	if(_dataCaptureRunning_BT)
	{
		_data_BT.backupTest[_currentTest_BT].endtime = millis();
		logger.log(LogLevel::TEST,
				   " time captured, stoptime:", _data_BT.backupTest[_currentTest_BT].endtime);

		logger.log(LogLevel::TEST, " switch time: ",
				   _data_BT.backupTest[_currentTest_BT].endtime -
					   _data_BT.backupTest[_currentTest_BT].starttime);

		_dataCaptureRunning_BT = false;
		_dataCaptureOk_BT = true; // Set flag to process timing data

		if(_dataCaptureOk_BT)
		{
			logger.log(LogLevel::SUCCESS, " time capture ok");
		}
	}
}

bool BackupTest::checkBackupRange(unsigned long backuptime)
{
	if(backuptime >= _cfgTest_BT.min_valid_backup_time_ms &&
	   backuptime <= _cfgTest_BT.max_valid_backup_time_ms)
	{
		return true;
	}
	return false;
}

bool BackupTest::processTestImpl()
{
	unsigned long endtime = _data_BT.backupTest[_currentTest_BT].endtime;
	unsigned long starttime = _data_BT.backupTest[_currentTest_BT].starttime;
	unsigned long backuptime = endtime - starttime;

	if(starttime > 0 && endtime > 0)
	{
		if(checkBackupRange(backuptime))
		{
			_data_BT.backupTest[_currentTest_BT].valid_data = true;
			_data_BT.backupTest[_currentTest_BT].testNo = _currentTest_BT + 1;
			_data_BT.backupTest[_currentTest_BT].testTimestamp = millis();
			_data_BT.backupTest[_currentTest_BT].backuptime = backuptime;
			logger.log(LogLevel::SUCCESS, "Processing successful");
			return true;
		}
	}
	logger.log(LogLevel::ERROR, "Processing failed, invalid data");
	return false;
}

TestResult BackupTest::run(uint16_t testVARating, unsigned long testduration)
{
	setLoad(testVARating); // Set the load
	unsigned long testStartTime = millis(); // Record the start time
	_testDuration_BT = testduration; // Set the test duration
	_dataCaptureOk_BT = false; // Ensure data capture is reset
	_testinProgress_BT = false;

	logger.log(LogLevel::INFO, "Starting Backup Test");
	if(!_triggerTestOngoingEvent_BT)
	{
		_triggerTestOngoingEvent_BT = true;
		_triggerValidDataEvent_BT = false;
		_triggerTestEndEvent_BT = false;

		logger.log(LogLevel::WARNING, "Triggering Test ongoing event from backup test");
		Manager->triggerEvent(Event::TEST_ONGOING);
		vTaskDelay(pdTICKS_TO_MS(100));
	}

	// Main loop running until the total test duration expires
	while(millis() - testStartTime < _testDuration_BT)
	{
		logger.log(LogLevel::TEST, "BackupTest ongoing...");

		unsigned long elapsedTime = millis() - testStartTime;
		unsigned long remainingTime = _testDuration_BT - elapsedTime;
		logger.log(LogLevel::INFO, "remaining time ms:", remainingTime);

		if(!_testinProgress_BT)
		{
			_testinProgress_BT = true;
			logger.log(LogLevel::TEST, "Simulating Power cut");
			simulatePowerCut();
			vTaskDelay(pdMS_TO_TICKS(50));
		}

		vTaskDelay(pdMS_TO_TICKS(100)); // Delay to avoid busy-waiting
	}

	// After the main test duration has expired
	if(_testinProgress_BT)
	{
		simulatePowerRestore();
		logger.log(LogLevel::TEST, "Test cycle ended. Power restored.");

		_testinProgress_BT = false;
		_triggerTestOngoingEvent_BT = false;
		_triggerTestEndEvent_BT = true;

		logger.log(LogLevel::WARNING, "Cycle ended for single backup test");
		Manager->triggerEvent(Event::TEST_TIME_END);
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	vTaskDelay(pdMS_TO_TICKS(200)); // Small delay before processing

	if(_dataCaptureOk_BT)
	{
		logger.log(LogLevel::TEST, "Processing time captured");
		if(!_triggerDataCaptureEvent_BT)
		{
			_triggerDataCaptureEvent_BT = true;
			logger.log(LogLevel::INFO, "Triggering DATA Captured event from backup test");
			Manager->triggerEvent(Event::DATA_CAPTURED);
			vTaskDelay(pdMS_TO_TICKS(100));
		}

		if(processTestImpl())
		{
			if(!_triggerValidDataEvent_BT)
			{
				_triggerDataCaptureEvent_BT = false;
				_triggerValidDataEvent_BT = true;

				logger.log(LogLevel::SUCCESS, "Triggering valid Data event from backup test");
				logger.log(LogLevel::TEST,
						   "BackUp Time: ", _data_BT.backupTest[_currentTest_BT].backuptime);
				sendEndSignal();
				Manager->triggerEvent(Event::VALID_DATA);
				vTaskDelay(pdMS_TO_TICKS(100));
			}

			logger.log(LogLevel::TEST, "Current BackupTest finished!");
			vTaskDelay(pdMS_TO_TICKS(100));
			return TEST_SUCCESSFUL;
		}
		else
		{
			logger.log(LogLevel::ERROR, "Invalid timing data");
			Manager->triggerEvent(Event::TEST_FAILED);
			vTaskDelay(pdMS_TO_TICKS(100));
			return TEST_FAILED;
		}
	}
	else
	{
		logger.log(LogLevel::ERROR, "Data capture failed");
		Manager->triggerEvent(Event::TEST_FAILED);
		vTaskDelay(pdMS_TO_TICKS(100));
		return TEST_FAILED;
	}

	logger.log(LogLevel::ERROR, "Test failed");
	Manager->triggerEvent(Event::TEST_FAILED);
	vTaskDelay(pdMS_TO_TICKS(100));
	return TEST_FAILED;
}
