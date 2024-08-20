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
      TaskParams* params = (TaskParams*)pvParameters;
      Serial.print("Switchtask VA rating is: ");
      Serial.println(params->setupParams.task_TestVARating);
      Serial.print("Switchtask duration is: ");
      Serial.println(params->setupParams.task_testDuration_ms);

      instance->run(params->setupParams.task_TestVARating,
                    params->setupParams.task_testDuration_ms);
      Serial.print("Switch Stack High Water Mark: ");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack usage
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
    Serial.println("time capture started...");
    Serial.print("start time:");
    Serial.println(_data.switchTest[_currentTest].starttime);
  }

  else {
    Serial.println("Debounce...");
  }
}

// Stop the test
void SwitchTest::stopTestCapture() {
  if (_dataCaptureRunning) {
    _data.switchTest[_currentTest].endtime = millis();
    Serial.println("time capture done...");
    Serial.print("stop time:");
    Serial.println(_data.switchTest[_currentTest].endtime);
    Serial.print("switch time:");
    Serial.println(_data.switchTest[_currentTest].endtime
                   - _data.switchTest[_currentTest].starttime);
    _dataCaptureRunning = false;
    _dataCaptureOk = true;  // Set flag to process timing data

    if (_dataCaptureOk) {
      Serial.println("time capture ok recorded");
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
      Serial.print("Starting test attempt ");
      Serial.println(retries + 1);
      simulatePowerCut();
      currentTestStartTime = millis();
      _testinProgress = true;
    }

    unsigned long elapsedTime = millis() - currentTestStartTime;
    long remainingTime = _testDuration - elapsedTime;
    Serial.print("test duration set is:");
    Serial.println(_testDuration);

    Serial.print("Remaining single test time: ");
    Serial.println(remainingTime);

    if (elapsedTime >= _testDuration) {
      Serial.println("Ending switch test...");
      simulatePowerRestore();
      _testinProgress = false;

      vTaskDelay(pdMS_TO_TICKS(100));  // Small delay before processing

      if (_dataCaptureOk) {
        Serial.println("Processing time capture...");

        if (processTestImpl()) {
          Serial.print("Switching Time: ");
          Serial.print(_data.switchTest[_currentTest].switchtime);
          Serial.println(" ms");
          sendEndSignal();
          valid_data = true;
          break;  // Exit the loop as the test was successful
        }

      } else {
        _data.switchTest[_currentTest].valid_data = false;
        Serial.println("Invalid timing data, retrying...");
        retries++;

        if (retries >= maximum_retest_number) {
          Serial.println("Max retries reached. Test failed.");
          break;
        }
      }

      _dataCaptureOk = false;  // Reset for the next iteration
    }

    vTaskDelay(pdMS_TO_TICKS(100));  // Delay to avoid busy-waiting
  }

  if (!valid_data) {
    Serial.println("Test duration elapsed or test failed.");
    return TEST_FAILED;
  } else {
    Serial.println("Test completed successfully.");
    return TEST_SUCESSFUL;
  }
}
