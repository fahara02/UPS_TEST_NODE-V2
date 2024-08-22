#ifndef SWITCH_TEST_H
#define SWITCH_TEST_H

#include "TestData.h"
#include "UPSTest.h"

class SwitchTest
    : public UPSTest<SwitchTest, SwitchTestData, TestType::SwitchTest> {
public:
  static SwitchTest* getInstance();
  static void deleteInstance();

  TestResult run(uint16_t testVARating = 4000,
                 unsigned long testduration = 10000) override;
  void MainTestTask(void* pvParameters) override;

private:
  friend class TestManager;
  ~SwitchTest() override;
  static SwitchTest* instance;  // Static pointer to hold the instance

  SwitchTestData _data;

  void initTestdataImpl() override;

  void startTestCapture() override;
  void stopTestCapture() override;

  bool processTestImpl() override;

  bool checkTimerRange(unsigned long switchtime);
};

#endif  // SWITCH_TEST_H
