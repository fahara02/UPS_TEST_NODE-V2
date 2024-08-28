#ifndef SWITCH_TEST_H
#define SWITCH_TEST_H
#include "TestData.h"
#include "UPSTest.h"
#include "TestManager.h"
using namespace Node_Core;
extern QueueHandle_t SwitchTestDataQueue;
extern TaskHandle_t switchTestTaskHandle;
class SwitchTest : public UPSTest<SwitchTest, SwitchTestData>
{
  public:
	static constexpr TestType test_type = TestType::SwitchTest;

	TestType getTestType() const override
	{
		return TestType::SwitchTest;
	}

	QueueHandle_t getQueue() const override
	{
		return SwitchTestDataQueue;
	}
	TaskHandle_t getTaskHandle() const override
	{
		return switchTestTaskHandle;
	}

	const char* testTypeName() override
	{
		return "SwitchTest";
	}

	bool isTestEnded() const override
	{
		if(_triggerTestEndEvent_SW)
		{
			return true;
		}
		return false;
	};

	bool isdataCaptureOk() const override
	{
		if(_dataCaptureOk_SW)
		{
			return true;
		}
		return false;
	}
	void markTestAsDone() override
	{
		_sendTestData_SW = false;
	}

	static SwitchTest* getInstance()
	{
		return UPSTest<SwitchTest, SwitchTestData>::getInstance();
	}
	static void deleteInstance()
	{
		UPSTest<SwitchTest, SwitchTestData>::deleteInstance();
	}
	SwitchTestData& data() override;
	void init() override;
	TestResult run(uint16_t testVARating = 4000, unsigned long testduration = 10000) override;
	static void SwitchTestTask(void* pvParameters);

	void UpdateSettings();
	~SwitchTest() override;

  protected:
  private:
	friend class TestManager;

	// static SwitchTest* instanceSW; // Static pointer to hold the instance
	SwitchTestData _data_SW = SwitchTestData();

	SetupSpec _cfgSpec_SW;
	SetupTest _cfgTest_SW;
	SetupTask _cfgTask_SW;
	SetupTaskParams _cfgTaskParam_SW;
	SetupHardware _cfgHardware_SW;
	SetupTuning _cfgTuning_SW;

	uint8_t _currentTest_SW = 0;
	TestResult _currentTestResult = TestResult::TEST_PENDING;
	unsigned long _testDuration_SW = 0;

	bool _initialized_SW = false;
	bool _testinProgress_SW = false;
	bool _dataCaptureOk_SW = false;
	bool _dataCaptureRunning_SW = false;
	bool _triggerTestOngoingEvent_SW = false;
	bool _triggerTestEndEvent_SW = false;
	bool _triggerDataCaptureEvent_SW = false;
	bool _triggerValidDataEvent_SW = false;
	bool _triggerDataSaveEvent_SW = false;
	bool _sendTestData_SW = false;

	void startTestCapture() override;
	void stopTestCapture() override;
	bool processTestImpl() override;

	bool checkTimerRange(unsigned long switchtime);
};

#endif // SWITCH_TEST_H
