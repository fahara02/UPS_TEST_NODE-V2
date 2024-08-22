#ifndef UPS_TEST_H
#define UPS_TEST_H

#include "Arduino.h"
#include "HardwareConfig.h"
#include "Logger.h"
#include "TestData.h"
#include "TestManager.h"
#include "UPSTestBase.h"
#include "UPSTesterSetup.h"

using namespace Node_Core;
extern Logger& logger;

extern volatile bool mains_triggered;
extern volatile bool ups_triggered;

using namespace Node_Core;
extern UPSTesterSetup* TesterSetup;
extern TaskHandle_t switchTestTaskHandle;
extern TaskHandle_t backupTimeTestTaskHandle;
extern TaskHandle_t efficiencyTestTaskHandle;
extern TaskHandle_t inputvoltageTestTaskHandle;
extern TaskHandle_t waveformTestTaskHandle;
extern TaskHandle_t tunepwmTestTaskHandle;

template <typename T, typename U, TestType testype>
class UPSTest : public UPSTestBase {
public:
  static constexpr TestType test_type = testype;

  void init() override;
  U& data();
  void forceUpdate();
  void startTest();
  void stopTest();

  // implement by derived class
  virtual TestResult run(uint16_t testVARating, unsigned long testDuration) = 0;
  TaskHandle_t* createmainTask();
  TaskHandle_t* createmainTask(void (T::*MainTestTask)(void*),
                               const char* taskName);

protected:
  UPSTest();  // Protected constructor
  virtual ~UPSTest() = default;
  void (T::*testFunctions[6])(void*);
  struct TaskParams {
    T* instance;
    void (T::*taskFunc)(void*);
    SetupTaskParams setupParams;
  };

  // utility functions
  void setTestDuration(unsigned long duration);
  void setLoad(uint16_t testVARating);
  void selectLoadBank(uint16_t bankNumbers);
  void simulatePowerCut();
  void simulatePowerRestore();
  void sendEndSignal();
  void processTest(T& test);

  TaskHandle_t* getTaskhandle();

  // callbacks
  void onSpecUpdate(bool spec_updated, SetupSpec spec) {
    if (spec_updated) {
      _cfgSpec = spec;
    }
  }

  void onTestUpdate(bool test_updated, SetupTest setting) {
    if (test_updated) {
      _cfgTest = setting;
    }
  }

  void onTaskUpdate(bool task_updated, SetupTask setting) {
    if (task_updated) {
      _cfgTask = setting;
    }
  }

  void onTaskParamsUpdate(bool taskParams_updated, SetupTaskParams setting) {
    if (taskParams_updated) {
      _cfgTaskParam = setting;
    }
  }

  void onHardwareUpdate(bool hardware_updated, SetupHardware setting) {
    if (hardware_updated) {
      _cfgHardware = setting;
    }
  }

  // that needs derived implementation
  virtual void initTestdataImpl() = 0;
  virtual void MainTestTask(void* pvParameters) = 0;
  virtual void startTestCapture() = 0;
  virtual void stopTestCapture() = 0;
  virtual bool checkRange() = 0;
  virtual bool processTestImpl() = 0;

  U _data;
  SetupSpec _cfgSpec;
  SetupTest _cfgTest;
  SetupTask _cfgTask;
  SetupTaskParams _cfgTaskParam;
  SetupHardware _cfgHardware;
  SetupTuning _cfgTuning;

  bool _initialized;
  bool _runTest;
  bool _testinProgress;
  bool _dataCaptureRunning;
  bool _dataCaptureOk;
  uint8_t _currentTest;
  unsigned long _testDuration;

private:
  friend class TestManager;
  static T* instance;

  // Prevent copying
  UPSTest(const UPSTest&) = delete;
  UPSTest& operator=(const UPSTest&) = delete;
};

template <typename T, typename U, TestType testype>
T* UPSTest<T, U, testype>::instance = nullptr;

// Private Constructor
template <typename T, typename U, TestType testype>
UPSTest<T, U, testype>::UPSTest()
    : _data(),
      _cfgSpec(TesterSetup->specSetup()),
      _cfgTest(TesterSetup->testSetup()),
      _cfgTask(TesterSetup->taskSetup()),
      _cfgTaskParam(TesterSetup->paramSetup()),
      _cfgHardware(TesterSetup->hardwareSetup()),
      _initialized(false),
      _runTest(false),
      _testinProgress(false),
      _dataCaptureRunning(false),
      _dataCaptureOk(false),
      _currentTest(0),
      _testDuration(TesterSetup->testSetup().testDuration_ms) {

  TesterSetup->registerSpecCallback([this](bool spec_updated, SetupSpec spec) {
    this->onSpecUpdate(spec_updated, spec);
  });
  TesterSetup->registerTestCallback(
      [this](bool test_updated, SetupTest setting) {
        this->onTestUpdate(test_updated, setting);
      });
  TesterSetup->registerTaskCallback(
      [this](bool task_updated, SetupTask setting) {
        this->onTaskUpdate(task_updated, setting);
      });
  TesterSetup->registerTaskParamsCallback(
      [this](bool taskParams_updated, SetupTaskParams setting) {
        this->onTaskParamsUpdate(taskParams_updated, setting);
      });
  TesterSetup->registerHardwareCallback(
      [this](bool hardware_updated, SetupHardware setting) {
        this->onHardwareUpdate(hardware_updated, setting);
      });
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::init() {
  initTestdataImpl();
  createmainTask();
}

template <typename T, typename U, TestType testype>
U& UPSTest<T, U, testype>::data() {
  return _data;
}
template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::forceUpdate() {
  if (TesterSetup) {
    _cfgSpec = TesterSetup->specSetup();
    _cfgTest = TesterSetup->testSetup();
    _cfgTask = TesterSetup->taskSetup();
    _cfgTaskParam = TesterSetup->paramSetup();
    _cfgHardware = TesterSetup->hardwareSetup();
  };
}

template <typename T, typename U, TestType testype>
TaskHandle_t* UPSTest<T, U, testype>::getTaskhandle() {

  TaskHandle_t* taskHandle = nullptr;

  switch (T::test_type) {
    case TestType::SwitchTest:
      taskHandle = &switchTestTaskHandle;
      break;
    case TestType::BackupTimeTest:
      taskHandle = &backupTimeTestTaskHandle;
      break;
    case TestType::EfficiencyTest:
      taskHandle = &efficiencyTestTaskHandle;
      break;
    case TestType::InputVoltageTest:
      taskHandle = &inputvoltageTestTaskHandle;
      break;
    case TestType::WaveformTest:
      taskHandle = &waveformTestTaskHandle;
      break;
    case TestType::TunePWMTest:
      taskHandle = &tunepwmTestTaskHandle;
      break;
    default:
      Serial.println("Unknown test type!");
      return nullptr;
  }
  return taskHandle;
}
template <typename T, typename U, TestType testype>
TaskHandle_t* UPSTest<T, U, testype>::createmainTask() {
  const char* taskName
      = testTypeToString(T::test_type);  // Assume this function is defined
  return createmainTask(testFunctions[0], taskName);
}

template <typename T, typename U, TestType testype>
TaskHandle_t* UPSTest<T, U, testype>::createmainTask(
    void (T::*MainTestTask)(void*), const char* taskName) {
  TaskHandle_t* taskHandle = getTaskhandle();

  if (taskHandle == nullptr) {
    Serial.println("Error: Task handle is null.");
    return nullptr;
  }

  auto taskFunction = [](void* param) {
    TaskParams* taskParams = static_cast<TaskParams*>(param);
    if (taskParams && taskParams->taskFunc) {
      // Call the member function on the instance
      (taskParams->instance->*taskParams->taskFunc)(param);
    } else {
      Serial.println("Error: TaskParams or task function is null.");
    }

    delete taskParams;  // Clean up the parameter structure
  };

  TaskParams* params = new (std::nothrow)
      TaskParams{static_cast<T*>(this), MainTestTask, _cfgTaskParam};
  if (params == nullptr) {
    Serial.println("Error: Memory allocation for TaskParams failed.");
    return nullptr;
  }

  if (MainTestTask == nullptr) {
    Serial.println("Error: MainTestTask function pointer is null.");
    delete params;  // Clean up
    return nullptr;
  }

  // Debug prints
  Serial.println("Creating task...");
  Serial.print("taskparam varating: ");
  Serial.println(params->setupParams.task_TestVARating);
  Serial.print("taskparam duration: ");
  Serial.println(params->setupParams.task_testDuration_ms);
  Serial.print("task stack: ");
  Serial.println(_cfgTask.mainTest_taskStack);
  Serial.print("task priority: ");
  Serial.println(_cfgTask.mainTest_taskIdlePriority);
  Serial.print("task core: ");
  Serial.println(_cfgTask.mainTest_taskCore);

  BaseType_t result = xTaskCreatePinnedToCore(
      taskFunction, taskName, _cfgTask.mainTest_taskStack,
      params,  // Pass TaskParams
      _cfgTask.mainTest_taskIdlePriority, taskHandle,
      _cfgTask.mainTest_taskCore);

  if (result != pdPASS) {
    Serial.println("Task creation failed!");
    delete params;  // Clean up if task creation fails
    return nullptr;
  }

  return taskHandle;
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::startTest() {
  TaskHandle_t* taskHandle = getTaskhandle();
  vTaskResume(taskHandle);
  _runTest = true;
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::stopTest() {
  TaskHandle_t* taskHandle = getTaskhandle();
  vTaskSuspend(taskHandle);
  _runTest = false;
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::setTestDuration(unsigned long duration) {
  _testDuration = duration;
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::setLoad(uint16_t testVARating) {
  const uint16_t maxVARating
      = _cfgSpec.Rating_va;  // Assuming maxVA rating is in TestSettings
  uint16_t singlebankVA = maxVARating / 4;
  uint16_t dualbankVA = (maxVARating / 4) * 2;
  uint16_t triplebankVA = (maxVARating / 4) * 3;
  uint16_t reqbankNumbers = 0;
  uint16_t duty = 0;
  uint16_t pwmValue = 0;
  uint16_t adjustpwm = 0;

  if (testVARating <= singlebankVA) {
    reqbankNumbers = 1;
    duty = (testVARating * 100) / singlebankVA;
    pwmValue = map(testVARating, 0, singlebankVA, 0, 255);
    adjustpwm = _cfgTuning.adjust_pwm_25P;
  } else if (testVARating > singlebankVA && testVARating <= dualbankVA) {
    reqbankNumbers = 2;
    duty = (testVARating * 100) / dualbankVA;
    pwmValue = map(testVARating, 0, dualbankVA, 0, 255);
    adjustpwm = _cfgTuning.adjust_pwm_50P;
  } else if (testVARating > dualbankVA && testVARating <= triplebankVA) {
    reqbankNumbers = 3;
    duty = (testVARating * 100) / triplebankVA;
    pwmValue = map(testVARating, 0, triplebankVA, 0, 255);
    adjustpwm = _cfgTuning.adjust_pwm_75P;
  } else if (testVARating > triplebankVA && testVARating <= maxVARating) {
    reqbankNumbers = 4;
    duty = (testVARating * 100) / maxVARating;
    pwmValue = map(testVARating, 0, maxVARating, 0, 255);
    adjustpwm = _cfgTuning.adjust_pwm_100P;
  }

  uint16_t set_pwmValue = pwmValue + adjustpwm;
  ledcWrite(_cfgHardware.pwmchannelNo, set_pwmValue);

  selectLoadBank(reqbankNumbers);
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::selectLoadBank(uint16_t bankNumbers) {
  digitalWrite(LOAD25P_ON_PIN, bankNumbers >= 1 ? HIGH : LOW);
  digitalWrite(LOAD50P_ON_PIN, bankNumbers >= 2 ? HIGH : LOW);
  digitalWrite(LOAD75P_ON_PIN, bankNumbers >= 3 ? HIGH : LOW);
  digitalWrite(LOAD_FULL_ON_PIN, bankNumbers >= 4 ? HIGH : LOW);
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::sendEndSignal() {
  digitalWrite(TEST_END_INT_PIN, HIGH);
  vTaskDelay(pdMS_TO_TICKS(10));
  digitalWrite(TEST_END_INT_PIN, LOW);
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::simulatePowerCut() {
  digitalWrite(UPS_POWER_CUT_PIN, HIGH);  // Simulate power cut
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::simulatePowerRestore() {
  digitalWrite(UPS_POWER_CUT_PIN, LOW);  // Simulate Mains Power In
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::processTest(T& test) {
  // Placeholder for processing the test; type-specific implementations should
  // override processTestImpl
  processTestImpl();
}

#endif  // UPS_TEST_H
