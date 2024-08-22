#ifndef TEST_DATA_H
#define TEST_DATA_H
#include <stdint.h>
enum TestResult { TEST_FAILED = 0, TEST_SUCCESSFUL = 1 };
enum class ISRType { MAINS_TRIGGERED, UPS_TRIGGERED };
enum class TestType {
  SwitchTest,
  BackupTimeTest,
  EfficiencyTest,
  InputVoltageTest,
  WaveformTest,
  TunePWMTest,
};

static const char* testTypeToString(TestType type) {
  switch (type) {
    case TestType::SwitchTest:
      return "SwitchTest";
    case TestType::BackupTimeTest:
      return "BackupTimeTest";
    case TestType::EfficiencyTest:
      return "EfficiencyTest";
    case TestType::InputVoltageTest:
      return "InputVoltageTest";
    case TestType::WaveformTest:
      return "WaveformTest";
    case TestType::TunePWMTest:
      return "TunePWMTest";
    default:
      return "UnknownTest";
  }
}
enum LoadPercentage {
  LOAD_0P = 0,
  LOAD_25P = 25,
  LOAD_50P = 50,
  LOAD_75P = 75,
  LOAD_100P = 100
};
struct SwitchTestData {

  struct TestData {
    uint8_t testNo;
    unsigned long testTimestamp;
    unsigned long switchtime;
    unsigned long starttime;
    unsigned long endtime;
    LoadPercentage load_percentage : 7;  // Adjusted to cover all possible
    bool valid_data : 1;
  } switchTest[5];
};
#endif