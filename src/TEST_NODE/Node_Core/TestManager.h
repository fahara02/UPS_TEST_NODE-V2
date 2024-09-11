#ifndef TEST_MANAGER_H
#define TEST_MANAGER_H
#include "Arduino.h"
#include <atomic>
#include "HardwareConfig.h"
#include "Settings.h"
#include "StateMachine.h"
#include "TestData.h"
#include "Logger.h"
#include "TestSync.h"
#include "UPSError.h"
#include "UPSTesterSetup.h"
#include "UPSTest.h"
#include "NodeConstants.h"
#include "Settings.h"

using namespace Node_Core;

class TestManager : public SettingsObserver
{
  public:
	static TestManager& getInstance();

	void init();

	void updateState(State state);
	void updateMode(TestMode mode);

	void onSettingsUpdate(SettingType type, const void* settings) override;
	void ReconfigureTaskParams();
	void passEvent(Event event);

	void addTests(RequiredTest testList[], int numTest);

	QueueHandle_t switchTestDataQueue;
	QueueHandle_t backupTestDataQueue;

  private:
	TestManager();

	bool _initialized = false;
	bool _newEventTrigger = false;
	bool _setupUpdated = false;

	uint8_t _numTest = 0;

	std::atomic<State> _currentState{State::DEVICE_ON};
	std::atomic<TestMode> _deviceMode{TestMode::MANUAL};

	UPSTestRun _testList[MAX_TEST];

	SetupSpec _cfgSpec;
	SetupTest _cfgTest;
	SetupTaskParams _cfgTaskParam;
	SetupHardware _cfgHardware;

	void setupPins();
	void configureInterrupts();
	void createISRTasks();
	void createTestTasks();
	void createManagerTasks();

	// helper functions switchTest
	bool isTestPendingAndNotStarted(const UPSTestRun& test);
	void logPendingTest(const UPSTestRun& test);
	void configureTest(LoadPercentage load);

	void initializeTestInstances();

	static void TestManagerTask(void* pvParameters);
	static void onMainsPowerLossTask(void* pvParameters);
	static void onUPSPowerGainTask(void* pvParameters);
	static void onUPSPowerLossTask(void* pvParameters);

	template<typename T, typename U>
	bool handleTestState(UPSTest<T, U>& testInstance, State managerState, int testIndex,
						 U* dataBuff = nullptr);

	TestManager(const TestManager&) = delete;
	TestManager& operator=(const TestManager&) = delete;
};

#endif