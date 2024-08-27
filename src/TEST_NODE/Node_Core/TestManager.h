#ifndef TEST_MANAGER_H
#define TEST_MANAGER_H
#include "Arduino.h"
#include "HardwareConfig.h"
#include "Settings.h"
#include "StateMachine.h"
#include "TestData.h"
#include "UPSError.h"
#include "UPSTesterSetup.h"
#include "UPSTest.h"

extern void IRAM_ATTR keyISR1(void* pvParameters);
extern void IRAM_ATTR keyISR2(void* pvParameters);
extern void IRAM_ATTR keyISR3(void* pvParameters);

using namespace Node_Core;

const uint16_t MAX_TESTS = 6;

using namespace Node_Core;

enum class TestManagerStatus : EventBits_t
{
	NOT_IN_QUEUE = 1 << 0,
	PENDING = 1 << 1,
	RETEST = 1 << 2,
	DONE = 1 << 3
};

//
enum class TestOperatorStatus : EventBits_t
{
	NOT_STARTED = 1 << 4,
	RUNNING = 1 << 5,
	SUCCESS = 1 << 6,
	FAILED = 1 << 7
};

struct TestStatus
{
	TestManagerStatus managerStatus;
	TestOperatorStatus operatorStatus;
	TestStatus() :
		managerStatus(TestManagerStatus::PENDING), operatorStatus(TestOperatorStatus::NOT_STARTED)
	{
	}
};
// for outside use of  the class
struct RequiredTest
{
	int TestNo; // Unique test number
	TestType testtype;
	LoadPercentage loadlevel;
	bool addTest = true;
	RequiredTest() :
		TestNo(0), testtype(TestType::SwitchTest), loadlevel(LoadPercentage::LOAD_25P),
		addTest(true)
	{
	}
	RequiredTest(int testNo, TestType type, LoadPercentage level, bool add) :
		TestNo(testNo), testtype(type), loadlevel(level), addTest(add)
	{
	}
};

struct UPSTestRun
{
	RequiredTest testRequired;
	TestStatus testStatus;
	UPSTestRun() :
		testRequired(), // Calls the default constructor of RequiredTest
		testStatus() // Calls the default constructor of TestStatus
	{
	}
};

class TestManager
{
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
	void UpdateSettings();

  private:
	TestManager();
	~TestManager();
	static TestManager* instance;

	UPSTestRun _testList[MAX_TESTS];

	bool _initialized = false;
	bool _newEventTrigger = false;
	bool _setupUpdated = false;
	bool _addedSwitchTest = false;
	uint8_t _numSwitchTest = 0;
	uint8_t _numBackupTest = 0;

	State _stateManager;
	SetupSpec _cfgSpec;
	SetupTest _cfgTest;
	SetupTask _cfgTask;
	SetupTaskParams _cfgTaskParam;
	SetupHardware _cfgHardware;

	void setupPins();
	void configureInterrupts();
	void createISRTasks();
	void createTestTasks();
	void createManagerTasks();

	void pauseAllTest();

	// helper functions switchTest
	bool isTestPendingAndNotStarted(const UPSTestRun& test);
	void logPendingTest(const UPSTestRun& test);
	void configureTest(LoadPercentage load);

	template<typename T>
	void handleTestState(UPSTest<T>* testInstance, State managerState);

	TestManager(const TestManager&) = delete;
	TestManager& operator=(const TestManager&) = delete;
};

#endif