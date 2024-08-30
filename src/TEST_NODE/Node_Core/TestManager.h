#ifndef TEST_MANAGER_H
#define TEST_MANAGER_H
#include "Arduino.h"
#include "HardwareConfig.h"
#include "Settings.h"
#include "StateMachine.h"
#include "TestData.h"
#include "Logger.h"
#include "TestSync.h"
#include "UPSError.h"
#include "UPSTesterSetup.h"
#include "UPSTest.h"

using namespace Node_Core;
const uint16_t MAX_TESTS = 6;
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
	static TestManager& getInstance();

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

	UPSTestRun _testList[MAX_TESTS];

	bool _initialized = false;
	bool _newEventTrigger = false;
	bool _setupUpdated = false;

	uint8_t _numSwitchTest = 0;
	uint8_t _numBackupTest = 0;
	uint8_t _numTest = 0;

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

	template<typename T, typename U>
	bool handleTestState(UPSTest<T, U>& testInstance, State managerState, int testIndex,
						 U* dataBuff = nullptr);

	TestManager(const TestManager&) = delete;
	TestManager& operator=(const TestManager&) = delete;
};

#endif