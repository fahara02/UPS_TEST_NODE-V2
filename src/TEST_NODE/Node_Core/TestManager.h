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
#include "UPSError.h"
#include "UPSTesterSetup.h"

extern void IRAM_ATTR keyISR1(void* pvParameters);
extern void IRAM_ATTR keyISR2(void* pvParameters);
extern void IRAM_ATTR keyISR3(void* pvParameters);

using namespace Node_Core;
class SwitchTest;
extern SwitchTest* switchTest;

static TaskHandle_t TestManagerTaskHandle = NULL;
extern TaskHandle_t switchTestTaskHandle;
extern TaskHandle_t backupTimeTestTaskHandle;
extern TaskHandle_t efficiencyTestTaskHandle;
extern TaskHandle_t inputvoltageTestTaskHandle;
extern TaskHandle_t waveformTestTaskHandle;
extern TaskHandle_t tunepwmTestTaskHandle;

const uint16_t MAX_TESTS = 6;

using namespace Node_Core;

enum class TestManagerStatus : EventBits_t {
  NOT_IN_QUEUE = 1 << 0,
  PENDING = 1 << 1,
  RETEST = 1 << 2,
  DONE = 1 << 3
};

//
enum class TestOperatorStatus : EventBits_t {
  NOT_STARTED = 1 << 4,
  RUNNING = 1 << 5,
  SUCCESS = 1 << 6,
  FAILED = 1 << 7
};

struct TestStatus {
  TestManagerStatus managerStatus;
  TestOperatorStatus operatorStatus;
  TestStatus()
      : managerStatus(TestManagerStatus::PENDING),
        operatorStatus(TestOperatorStatus::NOT_STARTED) {}
};
// for outside use of  the class
struct RequiredTest {
  int TestNo;  // Unique test number
  TestType testtype;
  LoadPercentage loadlevel;
  TestStatus testStatus;
};

template <typename T, typename U, TestType testype>
class UPSTest;

template <typename T, typename U>
struct UPSTestRun {
  T* testinstance;  // Pointer to the test instance
  RequiredTest testRequired;
  U testData;
};

class TestManager {
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

private:
  TestManager();
  ~TestManager();
  static TestManager* instance;

  /* external class instances as private member and will be initialise in
   testmanager construction or in init phase */
  StateMachine* stateMachine = nullptr;

  UPSTestRun<SwitchTest, SwitchTestData> testsSW[MAX_TESTS];

  bool _initialized = false;
  bool _setupUpdated = false;
  bool _addedSwitchTest = false;
  uint8_t _numSwitchTest = 0;
  uint8_t _numBackupTimeTest = 0;

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

  void pauseAllTest();

  TestManager(const TestManager&) = delete;
  TestManager& operator=(const TestManager&) = delete;
};

#endif