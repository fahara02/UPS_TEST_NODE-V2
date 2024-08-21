#ifndef TEST_MANAGER_H
#define TEST_MANAGER_H
#include "Arduino.h"
#include "HardwareConfig.h"
#include "Settings.h"
#include "StateMachine.h"
#include "TestData.h"

#include "UPSTestBase.h"

#include "UPSTest.h"

#include "SwitchTest.h"
#include "UPSTesterSetup.h"

extern void IRAM_ATTR keyISR1(void* pvParameters);
extern void IRAM_ATTR keyISR2(void* pvParameters);
extern void IRAM_ATTR keyISR3(void* pvParameters);

using namespace Node_Core;

static TaskHandle_t TestManagerTaskHandle = NULL;
extern TaskHandle_t switchTestTaskHandle;
extern TaskHandle_t backupTimeTestTaskHandle;
extern TaskHandle_t efficiencyTestTaskHandle;
extern TaskHandle_t inputvoltageTestTaskHandle;
extern TaskHandle_t waveformTestTaskHandle;
extern TaskHandle_t tunepwmTestTaskHandle;

const uint16_t MAX_TESTS = 6;

using namespace Node_Core;

enum class TestStatus {
  TEST_PENDING,
  TEST_RUNNING,
  TEST_COMPLETED,
  TEST_FAILED
};

template <typename T, typename U, TestType testype>
class UPSTest;

class SwitchTest;
static SwitchTest* switchTest = nullptr;

template <typename T, typename U, TestType testype>
struct UPSTestRun {
  UPSTest<T, U, testype>* test;  // Pointer to the specific UPS test
  TestStatus status;             // Enum to track the status of the
  U data;  // Data related to the test (U is the data type)
};

class TestManager {
public:
  static TestManager* getInstance();
  static void deleteInstance();
  UPSTestRun<SwitchTest, SwitchTestData, TestType::SwitchTest>
      testsSwitch[MAX_TESTS];

  void init();

  void runTests();
  void manageTests();
  void terminateTest();

  static void TestManagerTask(void* pvParameters);
  static void onMainsPowerLossTask(void* pvParameters);
  static void onUPSPowerGainTask(void* pvParameters);
  static void onUPSPowerLossTask(void* pvParameters);
  void initializeTestInstances();

private:
  TestManager();
  ~TestManager();
  static TestManager* instance;
  bool _initialized = false;
  bool _setupUpdated = false;
  uint8_t numTests;

  StateMachine* stateMachine = nullptr;
  State _currentstate;
  SetupSpec _cfgSpec;
  SetupTest _cfgTest;
  SetupTask _cfgTask;
  SetupTaskParams _cfgTaskParam;
  SetupHardware _cfgHardware;

  void setupPins();
  void configureInterrupts();
  void createManagerTasks();
  void createISRTasks();

  TestManager(const TestManager&) = delete;
  TestManager& operator=(const TestManager&) = delete;
};

#endif