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
    instance->testFunctions[0] = &SwitchTest::MainTestTask;
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
      // TaskParams* params = (TaskParams*)pvParameters;
      logger.log(LogLevel::TEST,
                 "Switchtask VA rating is: ", _cfgTest.testVARating);
      logger.log(LogLevel::TEST,
                 "Switchtask duration is: ", _cfgTest.testDuration_ms);

      instance->run(_cfgTest.testVARating, _cfgTest.testDuration_ms);
      logger.log(LogLevel::WARNING, "Hihg Water mark ",
                 uxTaskGetStackHighWaterMark(NULL));

      vTaskDelay(pdMS_TO_TICKS(100));  // Delay to prevent tight loop
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

bool SwitchTest::checkRange() {
  unsigned long switchtime = _data.switchTest[_currentTest].switchtime;
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
    if (checkRange()) {
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
  uint8_t retries = 0;
  setLoad(testVARating);                   // Set the load
  unsigned long testStartTime = millis();  // Overall start time
  bool valid_data = false;                 // Initially assume data is not valid
  bool testInProgress = false;
  unsigned long currentTestStartTime = 0;
  uint8_t maximum_retest_number = _cfgTest.MaxRetest;
  unsigned long testDurationWithRetest = 0;
  if (testduration == _testDuration) {

    testDurationWithRetest = maximum_retest_number * _testDuration;
  } else {
    _testDuration = testduration;
    testDurationWithRetest = maximum_retest_number * _testDuration;
  }

  while (millis() - testStartTime < testDurationWithRetest) {
    if (!_testinProgress) {
      logger.log(LogLevel::INFO, "Starting test Attempt:", retries + 1);
      simulatePowerCut();
      currentTestStartTime = millis();
      _testinProgress = true;
    }

    unsigned long elapsedTime = millis() - currentTestStartTime;
    long remainingTime = _testDuration - elapsedTime;
    logger.log(LogLevel::TEST, " test duration", _testDuration);
    logger.log(LogLevel::TEST, " remaining single test time:", remainingTime);

    if (elapsedTime >= _testDuration) {
      logger.log(LogLevel::TEST, "Ending Switching Test");
      simulatePowerRestore();
      _testinProgress = false;

      vTaskDelay(pdMS_TO_TICKS(100));  // Small delay before processing

      if (_dataCaptureOk) {
        logger.log(LogLevel::TEST, "Processing time captured");

        if (processTestImpl()) {
          logger.log(LogLevel::TEST, "Switching Time: ",
                     _data.switchTest[_currentTest].switchtime);
          sendEndSignal();
          valid_data = true;
          break;  // Exit the loop as the test was successful
        }

      } else {
        _data.switchTest[_currentTest].valid_data = false;
        logger.log(LogLevel::ERROR, "Invalid timing data, retrying...");

        retries++;

        if (retries >= maximum_retest_number) {
          logger.log(LogLevel::ERROR, "Max retries reached. Test failed.");
          break;
        }
      }

      _dataCaptureOk = false;  // Reset for the next iteration
    }

    vTaskDelay(pdMS_TO_TICKS(100));  // Delay to avoid busy-waiting
  }

  if (!valid_data) {
    logger.log(LogLevel::ERROR, "Test failed");
    return TEST_FAILED;
  } else {
    logger.log(LogLevel::SUCCESS, "Test completed Successfully");
    return TEST_SUCESSFUL;
  }
}
