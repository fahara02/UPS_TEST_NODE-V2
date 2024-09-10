#include "SwitchTest.h"

// extern EventGroupHandle_t eventGroupTest;

using namespace Node_Core;

extern QueueHandle_t TestManageQueue;
void SwitchTest::init()
{
	if(!_initialized_SW)
	{
		for(auto& test: _data_SW.switchTest)
		{
			test.testNo = 0;
			test.testTimestamp = 0;
			test.valid_data = false;
			test.switchtime = 0;
			test.starttime = 0;
			test.endtime = 0;
			test.load_percentage = LoadPercentage::LOAD_0P;
		};
		_initialized_SW = true;

		UpdateSettings();
		logger.log(LogLevel::SUCCESS, "SwitchTest initialised ");
	}
}

void SwitchTest::UpdateSettings()
{
	_cfgSpec_SW = TesterSetup.specSetup();
	_cfgTest_SW = TesterSetup.testSetup();
	_cfgTask_SW = TesterSetup.taskSetup();
	_cfgTaskParam_SW = TesterSetup.paramSetup();
	_cfgHardware_SW = TesterSetup.hardwareSetup();
}

SwitchTestData& SwitchTest::data()
{
	return _data_SW;
}
// Function for SwitchTest task
void SwitchTest::SwitchTestTask(void* pvParameters)
{
	SetupTaskParams taskParam;

	SwitchTest& switchTest = SwitchTest::getInstance();

	xQueueReceive(TestManageQueue, (void*)&taskParam, 0 == pdTRUE);

	while(xEventGroupWaitBits(EventHelper::testControlEvent,
							  static_cast<EventBits_t>(TestType::SwitchTest), pdFALSE, pdTRUE,
							  portMAX_DELAY))
	{
		logger.log(LogLevel::INFO, "resuming switchTest task");

		EventBits_t sw_eventbits = static_cast<EventBits_t>(TestType::SwitchTest);
		int result = xEventGroupGetBits(EventHelper::testControlEvent);

		if((result & sw_eventbits) != 0)

		{
			if(switchTest._currentTestResult == TestResult::TEST_SUCCESSFUL)
			{
				switchTest._sendTestData_SW = true;
			}

			if(!switchTest._sendTestData_SW)
			{
				xQueueReceive(TestManageQueue, (void*)&taskParam, 0 == pdTRUE);
				logger.log(LogLevel::TEST,
						   "Switchtask VA rating is: ", switchTest._cfgTest_SW.testVARating);
				logger.log(LogLevel::TEST,
						   "Switchtask duration is: ", switchTest._cfgTest_SW.testDuration_ms);

				switchTest._currentTestResult = switchTest.run(
					switchTest._cfgTest_SW.testVARating, switchTest._cfgTest_SW.testDuration_ms);
			}
			if(switchTest._sendTestData_SW)
			{
				int retrySend = 0;
				SwitchTestData testData = switchTest.data();
				if(xQueueSend(SwitchTestDataQueue, &testData, 100) == pdTRUE)
				{
					switchTest._currentTestResult = TestResult::TEST_PENDING;
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
					switchTest._sendTestData_SW = false;
				}

				;
			}
			logger.log(LogLevel::WARNING, "Hihg Water mark ", uxTaskGetStackHighWaterMark(NULL));
		}

		vTaskDelay(pdMS_TO_TICKS(200)); // Delay to prevent tight loop
	}
	vTaskDelete(NULL);
}

// Start the test
void SwitchTest::startTestCapture()
{
	if(_dataCaptureRunning_SW)
	{
		_data_SW.switchTest[_currentTest_SW].starttime = millis();
		_dataCaptureOk_SW = false;
		logger.log(LogLevel::TEST,
				   " time captured, starttime:", _data_SW.switchTest[_currentTest_SW].starttime);
	}
	else
	{
		logger.log(LogLevel::ERROR, "Debounce ");
	}
}

// Stop the test
void SwitchTest::stopTestCapture()
{
	if(_dataCaptureRunning_SW)
	{
		_data_SW.switchTest[_currentTest_SW].endtime = millis();
		logger.log(LogLevel::TEST,
				   " time captured, stoptime:", _data_SW.switchTest[_currentTest_SW].endtime);

		logger.log(LogLevel::TEST, " switch time: ",
				   _data_SW.switchTest[_currentTest_SW].endtime -
					   _data_SW.switchTest[_currentTest_SW].starttime);

		_dataCaptureRunning_SW = false;
		_dataCaptureOk_SW = true; // Set flag to process timing data

		if(_dataCaptureOk_SW)
		{
			logger.log(LogLevel::SUCCESS, " time capture ok");
		}
	}
}
bool SwitchTest::checkTimerRange(unsigned long switchtime)
{
	if(switchtime >= _cfgTest_SW.min_valid_switch_time_ms &&
	   switchtime <= _cfgTest_SW.max_valid_switch_time_ms)
	{
		return true;
	}
	return false;
}

bool SwitchTest::processTestImpl()
{
	unsigned long endtime = _data_SW.switchTest[_currentTest_SW].endtime;
	unsigned long starttime = _data_SW.switchTest[_currentTest_SW].starttime;
	unsigned long switchTime = endtime - starttime;

	if(starttime > 0 && endtime > 0)
	{
		if(checkTimerRange(switchTime))
		{
			_data_SW.switchTest[_currentTest_SW].valid_data = true;
			_data_SW.switchTest[_currentTest_SW].testNo = _currentTest_SW + 1;
			_data_SW.switchTest[_currentTest_SW].testTimestamp = millis();
			_data_SW.switchTest[_currentTest_SW].switchtime = switchTime;
			logger.log(LogLevel::SUCCESS, "Processing successful");
			return true;
		}
	}
	logger.log(LogLevel::ERROR, "Processing failed ,invalid data");
	return false;
}

TestResult SwitchTest::run(uint16_t testVARating, unsigned long testduration)
{
	TestSync& SyncTest = TestSync::getInstance();
	setLoad(testVARating); // Set the load
	unsigned long testStartTime = millis(); // Record the start time
	_testDuration_SW = testduration; // Set the test duration
	_dataCaptureOk_SW = false; // Ensure data capture is reset
	_testinProgress_SW = false;

	logger.log(LogLevel::INFO, "Starting Switching Test");
	if(!_triggerTestOngoingEvent_SW)
	{
		_triggerTestOngoingEvent_SW = true;
		_triggerValidDataEvent_SW = false;
		_triggerTestEndEvent_SW = false;

		logger.log(LogLevel::WARNING, "Triggering Test ongoing event from switch test");
		SyncTest.reportEvent(Event::TEST_RUN_OK);
		vTaskDelay(pdTICKS_TO_MS(100));
	}

	// Main loop running until the total test duration expires
	while(millis() - testStartTime < _testDuration_SW)
	{
		logger.log(LogLevel::TEST, "SwitchTest ongoing...");

		unsigned long elapsedTime = millis() - testStartTime;
		unsigned long remainingTime = _testDuration_SW - elapsedTime;
		logger.log(LogLevel::INFO, "remaining time ms:", remainingTime);

		if(!_testinProgress_SW)

		{
			_testinProgress_SW = true;
			logger.log(LogLevel::TEST, "Simulating Power cut");
			simulatePowerCut();
			vTaskDelay(pdMS_TO_TICKS(50));
		}

		vTaskDelay(pdMS_TO_TICKS(100)); // Delay to avoid busy-waiting
	}

	// After the main test duration has expired
	if(_testinProgress_SW)
	{
		simulatePowerRestore();
		logger.log(LogLevel::TEST, "Test cycle ended. Power restored.");

		_testinProgress_SW = false;
		_triggerTestOngoingEvent_SW = false;
		_triggerTestEndEvent_SW = true;

		logger.log(LogLevel::WARNING, "Cycle ended for single switch test");
		SyncTest.reportEvent(Event::TEST_TIME_END);
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	vTaskDelay(pdMS_TO_TICKS(200)); // Small delay before processing

	if(_dataCaptureOk_SW)
	{
		logger.log(LogLevel::TEST, "Processing time captured");
		if(!_triggerDataCaptureEvent_SW)
		{
			_triggerDataCaptureEvent_SW = true;
			logger.log(LogLevel::INFO, "Triggering DATA Captured event from switch test");
			SyncTest.reportEvent(Event::DATA_CAPTURED);
			vTaskDelay(pdMS_TO_TICKS(100));
		}

		if(processTestImpl())
		{
			if(!_triggerValidDataEvent_SW)
			{
				_triggerDataCaptureEvent_SW = false;
				_triggerValidDataEvent_SW = true;

				logger.log(LogLevel::SUCCESS, "Triggering valid Data  event from switch test");
				logger.log(LogLevel::TEST,
						   "Switching Time: ", _data_SW.switchTest[_currentTest_SW].switchtime);
				sendEndSignal();
				SyncTest.reportEvent(Event::VALID_DATA);
				vTaskDelay(pdMS_TO_TICKS(100));
			}

			logger.log(LogLevel::TEST, "Current SwitchTest finished!");
			vTaskDelay(pdMS_TO_TICKS(100));
			return TEST_SUCCESSFUL;
		}
		else
		{
			logger.log(LogLevel::ERROR, "Invalid timing data");
			SyncTest.reportEvent(Event::TEST_FAILED);
			vTaskDelay(pdMS_TO_TICKS(100));
			return TEST_FAILED;
		}
	}
	else
	{
		logger.log(LogLevel::ERROR, "Data capture failed");
		SyncTest.reportEvent(Event::TEST_FAILED);
		vTaskDelay(pdMS_TO_TICKS(100));
		return TEST_FAILED;
	}

	logger.log(LogLevel::ERROR, "Test failed");
	SyncTest.reportEvent(Event::TEST_FAILED);
	vTaskDelay(pdMS_TO_TICKS(100));
	return TEST_FAILED;
}
