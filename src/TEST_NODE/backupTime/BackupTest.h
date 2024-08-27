#ifndef BACKUP_TEST_H
#define BACKUP_TEST_H

#include "TestData.h"
#include "UPSTest.h"
#include "TestManager.h"

class BackupTest : public UPSTest<BackupTest>
{
  public:
	static constexpr TestType test_type = TestType::BackupTest;

	BackupTest* getInstance()
	{
		return UPSTest<BackupTest>::getInstance();
	}

	void deleteInstance()
	{
		UPSTest<BackupTest>::deleteInstance();
	}

	void init() override;
	TestResult run(uint16_t testVARating = 4000, unsigned long testduration = 10000) override;
	static void BackupTestTask(void* pvParameters);
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

	bool _initialized_BT = false;
	bool _testinProgress_BT = false;
	bool _dataCaptureRunning_BT = false;
	bool _dataCaptureOk_BT = false;
	bool _triggerTestOngoingEvent_BT = false;
	bool _triggerTestEndEvent_BT = false;
	bool _triggerDataCaptureEvent_BT = false;
	bool _triggerValidDataEvent_BT = false;
	bool _triggerDataSaveEvent_BT = false;

	BackupTestData _data_BT;
	void startTestCapture() override;
	void stopTestCapture() override;
	bool processTestImpl() override;

	bool checkBackupRange(unsigned long backuptime);
};

#endif