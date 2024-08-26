#ifndef SWITCH_TEST_H
#define SWITCH_TEST_H
#include "TestData.h"
#include "UPSTest.h"
#include "TestManager.h"
using namespace Node_Core;

class SwitchTest : public UPSTest<SwitchTest>
{
  public:
	static constexpr TestType test_type = TestType::SwitchTest;
	static SwitchTest* getInstance()
	{
		return UPSTest<SwitchTest>::getInstance();
	}
	static void deleteInstance()
	{
		UPSTest<SwitchTest>::deleteInstance();
	}
	SwitchTestData& data();
	void init() override;
	TestResult run(uint16_t testVARating = 4000, unsigned long testduration = 10000) override;
	static void SwitchTestTask(void* pvParameters);
	~SwitchTest() override;

  protected:
  private:
	friend class TestManager;

	// static SwitchTest* instanceSW; // Static pointer to hold the instance
	SwitchTestData _data_SW;

	SetupSpec _cfgSpec_SW;
	SetupTest _cfgTest_SW;
	SetupTask _cfgTask_SW;
	SetupTaskParams _cfgTaskParam_SW;
	SetupHardware _cfgHardware_SW;
	SetupTuning _cfgTuning_SW;

	uint8_t _currentTest_SW = 0;
	unsigned long _testDuration_SW = 0;

	bool _initialized_SW;
	bool _testinProgress_SW = false;
	bool _dataCaptureOk_SW = false;
	bool _dataCaptureRunning_SW = false;
	bool _triggerTestOngoingEvent_SW = false;
	bool _triggerTestEndEvent_SW = false;
	bool _triggerDataCaptureEvent_SW = false;
	bool _triggerValidDataEvent_SW = false;
	bool _triggerDataSaveEvent_SW = false;

	void startTestCapture() override;
	void stopTestCapture() override;
	bool processTestImpl() override;

	bool checkTimerRange(unsigned long switchtime);
};

#endif // SWITCH_TEST_H
