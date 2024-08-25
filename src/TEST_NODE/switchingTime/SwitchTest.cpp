#include "SwitchTest.h"

using namespace Node_Core;
// Initialize static members
SwitchTest* SwitchTest::instance = nullptr;

SwitchTest::~SwitchTest() {
  if (instance) {
    if (switchTestTaskHandle != NULL) {
      vTaskDelete(switchTestTaskHandle);
      switchTestTaskHandle = NULL;
    }
  }
  instance = nullptr;
}

SwitchTest* SwitchTest::getInstance() {
  if (instance == nullptr) {
    instance = new SwitchTest();
    // instance->testFunctions[0] = &SwitchTest::MainTestTask;
  }
  return instance;
}

void SwitchTest::deleteInstance() {
  if (instance != nullptr) {
    delete instance;
  }
}

void SwitchTest::initTestdataImpl() {

  for (auto& test : _data.switchTest) {
    test.testNo = 0;
    test.testTimestamp = 0;
    test.valid_data = false;
    test.switchtime = 0;
    test.starttime = 0;
    test.endtime = 0;
    test.load_percentage = LoadPercentage::LOAD_0P;
  };
}

// Function for SwitchTest task
void SwitchTest::MainTestTask(void* pvParameters) {
  if (instance) {
    while (true) {
      logger.log(LogLevel::INFO,
                 "resuming switchTest task, checking run test Flag");
      SetupTaskParams* taskParam = (SetupTaskParams*)pvParameters;

      if (true) {

        // TaskParams* params = (TaskParams*)pvParameters;
        logger.log(LogLevel::TEST,
                   "Switchtask VA rating is: ", taskParam->task_TestVARating);
        logger.log(LogLevel::TEST,
                   "Switchtask duration is: ", taskParam->task_testDuration_ms);

        instance->run(taskParam->task_TestVARating,
                      taskParam->task_testDuration_ms);
        logger.log(LogLevel::WARNING, "Hihg Water mark ",
                   uxTaskGetStackHighWaterMark(NULL));

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      vTaskDelay(pdMS_TO_TICKS(200));  // Delay to prevent tight loop
    }
  }
  vTaskDelete(NULL);
}

// Start the test
void SwitchTest::startTestCapture() {
  if (_dataCaptureRunning) {
    _data.switchTest[_currentTest].starttime = millis();
    _dataCaptureOk = false;
    logger.log(LogLevel::TEST, " time captured, starttime:",
               _data.switchTest[_currentTest].starttime);
  } else {
    logger.log(LogLevel::ERROR, "Debounce ");
  }
}

// Stop the test
void SwitchTest::stopTestCapture() {
  if (_dataCaptureRunning) {
    _data.switchTest[_currentTest].endtime = millis();
    logger.log(LogLevel::TEST, " time captured, stoptime:",
               _data.switchTest[_currentTest].endtime);

    logger.log(LogLevel::TEST, " switch time: ",
               _data.switchTest[_currentTest].endtime
                   - _data.switchTest[_currentTest].starttime);

    _dataCaptureRunning = false;
    _dataCaptureOk = true;  // Set flag to process timing data

    if (_dataCaptureOk) {
      logger.log(LogLevel::SUCCESS, " time capture ok");
    }
  }
}
bool SwitchTest::checkTimerRange(unsigned long switchtime) {
  if (switchtime >= _cfgTest.min_valid_switch_time_ms
      && switchtime <= _cfgTest.max_valid_switch_time_ms) {
    return true;
  }
  return false;
}

bool SwitchTest::processTestImpl() {
  unsigned long endtime = _data.switchTest[_currentTest].endtime;
  unsigned long starttime = _data.switchTest[_currentTest].starttime;
  unsigned long switchTime = endtime - starttime;

  if (starttime > 0 && endtime > 0) {
    if (checkTimerRange(switchTime)) {
      _data.switchTest[_currentTest].valid_data = true;
      _data.switchTest[_currentTest].testNo = _currentTest + 1;
      _data.switchTest[_currentTest].testTimestamp = millis();
      _data.switchTest[_currentTest].switchtime = switchTime;
      return true;
    }
  }
  return false;
}

TestResult SwitchTest::run(uint16_t testVARating, unsigned long testduration) {
  setLoad(testVARating);                   // Set the load
  unsigned long testStartTime = millis();  // Record the start time
  _testDuration = testduration;            // Set the test duration
  _dataCaptureOk = false;                  // Ensure data capture is reset
  _testinProgress = false;

  logger.log(LogLevel::INFO, "Starting Switching Test");

  // Main loop running until the total test duration expires
  while (millis() - testStartTime < _testDuration) {
    logger.log(LogLevel::TEST, "Test ongoing...");

    if (!_testinProgress) {
      simulatePowerCut();
      _testinProgress = true;
      logger.log(LogLevel::INFO, "Power cut simulated");
    }

    vTaskDelay(pdMS_TO_TICKS(100));  // Delay to avoid busy-waiting
  }

  // After the main test duration has expired
  if (_testinProgress) {
    simulatePowerRestore();
    _testinProgress = false;
    logger.log(LogLevel::TEST, "Test cycle ended. Power restored.");
  }

  vTaskDelay(pdMS_TO_TICKS(100));  // Small delay before processing

  if (_dataCaptureOk) {
    logger.log(LogLevel::TEST, "Processing time captured");

    if (processTestImpl()) {
      sendEndSignal();
      logger.log(LogLevel::TEST,
                 "Switching Time: ", _data.switchTest[_currentTest].switchtime);

      logger.log(LogLevel::SUCCESS, "Test completed successfully");
      return TEST_SUCCESSFUL;
    } else {
      logger.log(LogLevel::ERROR, "Invalid timing data");
      return TEST_FAILED;
    }
  } else {
    logger.log(LogLevel::ERROR, "Data capture failed");
    return TEST_FAILED;
  }

  logger.log(LogLevel::ERROR, "Test failed");
  return TEST_FAILED;
}
