#ifndef BACKUP_TEST_H
#define BACKUP_TEST_H

#include "TestData.h"
#include "UPSTest.h"
#include "TestManager.h"
extern QueueHandle_t BackupTestDataQueue;
extern TaskHandle_t backupTestTaskHandle;
class BackupTest : public UPSTest<BackupTest, BackupTestData>
{
  public:
	static constexpr TestType test_type = TestType::BackupTest;
	TestType getTestType() const override
	{
		return TestType::BackupTest;
	}

	QueueHandle_t getQueue() const override
	{
		return BackupTestDataQueue;
	}
	const char* testTypeName() override
	{
		return "BackupTest";
	}
	TaskHandle_t getTaskHandle() const override
	{
		return backupTestTaskHandle;
	}
	bool isTestRunning() const override
	{
		return _testinProgress_BT;
	}

	bool isTestEnded() const override
	{
		return _triggerTestEndEvent_BT;
	}

	bool isdataCaptureOk() const override
	{
		return _dataCaptureOk_BT;
	}
	void markTestAsDone() override
	{
		_sendTestData_BT = false;
	}
	static BackupTest* getInstance()
	{
		return UPSTest<BackupTest, BackupTestData>::getInstance();
	}

	void deleteInstance()
	{
		UPSTest<BackupTest, BackupTestData>::deleteInstance();
	}
	BackupTestData& data() override;
	void init() override;
	TestResult run(uint16_t testVARating = 4000, unsigned long testduration = 10000) override;
	static void BackupTestTask(void* pvParameters);
	void UpdateSettings();
	~BackupTest() override;

  private:
	friend class TestManager;

	SetupSpec _cfgSpec_BT;
	SetupTest _cfgTest_BT;
	SetupTask _cfgTask_BT;
	SetupTaskParams _cfgTaskParam_BT;
	SetupHardware _cfgHardware_BT;
	SetupTuning _cfgTuning_BT;

	uint8_t _currentTest_BT = 0;
	unsigned long _testDuration_BT = 0;
	TestResult _currentTestResult = TestResult::TEST_PENDING;

	bool _initialized_BT = false;
	bool _testinProgress_BT = false;
	bool _dataCaptureRunning_BT = false;
	bool _dataCaptureOk_BT = false;
	bool _triggerTestOngoingEvent_BT = false;
	bool _triggerTestEndEvent_BT = false;
	bool _triggerDataCaptureEvent_BT = false;
	bool _triggerValidDataEvent_BT = false;
	bool _triggerDataSaveEvent_BT = false;
	bool _sendTestData_BT = false;

	BackupTestData _data_BT = BackupTestData();
	void startTestCapture() override;
	void stopTestCapture() override;
	bool processTestImpl() override;

	bool checkBackupRange(unsigned long backuptime);
};

#endif