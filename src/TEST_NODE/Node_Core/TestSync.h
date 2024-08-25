#ifndef TEST_SYNC_H
#define TEST_SYNC_H

#include "Arduino.h"
#include "Logger.h"
#include "TestData.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

// Load Level: Bits 0-3
enum class LoadLevel : uint16_t {
  LEVEL_25 = 1 << 0,  // 0001
  LEVEL_50 = 1 << 1,  // 0010
  LEVEL_75 = 1 << 2,  // 0100
  LEVEL_100 = 1 << 3  // 1000
};

// Test Status Manager: Bits 4-7
enum class TestStatusManager : uint16_t {
  NOT_IN_QUEUE = 1 << 4,  // 0001 0000
  PENDING = 1 << 5,       // 0010 0000
  RETEST = 1 << 6,        // 0100 0000
  DONE = 1 << 7           // 1000 0000
};

// Test Status Tester: Bits 8-11
enum class TestStatusTester : uint16_t {
  NOT_STARTED = 1 << 8,  // 0001 0000 0000
  RUNNING = 1 << 9,      // 0010 0000 0000
  SUCCESS = 1 << 10,     // 0100 0000 0000
  FAILED = 1 << 11       // 1000 0000 0000
};

struct RequiredTest {
  TestType testName;
  LoadLevel level;
};

class TestSync {
public:
  // Get the instance of the singleton
  static TestSync& getInstance() {
    static TestSync instance;
    return instance;
  }
  void addTest(RequiredTest tests[], size_t numTests) {

    for (size_t i = 0; i < numTests; ++i) {
      // Set event bits for the test in TestSync
      setEventBits(tests[i].testName, tests[i].level,
                   TestStatusManager::PENDING, TestStatusTester::NOT_STARTED);
    }
  }
  void getPendingTest(RequiredTest tests[], size_t& numTests) {

    size_t testIndex = 0;

    for (int i = 0; i < TestSync::NUM_TEST_TYPES; ++i) {
      EventBits_t bits = getEventBits(static_cast<TestType>(i));

      if (bits & static_cast<uint16_t>(TestStatusManager::PENDING)) {

        for (int j = 0; j <= 3; ++j) {
          if (bits & (1 << j)) {
            tests[testIndex].testName = static_cast<TestType>(i);
            tests[testIndex].level = static_cast<LoadLevel>(1 << j);
            testIndex++;
          }
        }
      }
    }

    numTests = testIndex;
  }

  void setEventBits(TestType testType, LoadLevel loadLevel,
                    TestStatusManager managerStatus,
                    TestStatusTester testerStatus) {
    // Enter critical section
    portENTER_CRITICAL(&mux_);

    // Set the event bits in the appropriate event group
    xEventGroupSetBits(eventGroups_[static_cast<uint8_t>(testType)],
                       static_cast<uint16_t>(loadLevel)
                           | static_cast<uint16_t>(managerStatus)
                           | static_cast<uint16_t>(testerStatus));

    // Exit critical section
    portEXIT_CRITICAL(&mux_);
  }

  // Method to clear event bits
  void clearEventBits(TestType testType, LoadLevel loadLevel,
                      TestStatusManager managerStatus,
                      TestStatusTester testerStatus) {
    // Enter critical section
    portENTER_CRITICAL(&mux_);

    // Clear the event bits in the appropriate event group
    xEventGroupClearBits(eventGroups_[static_cast<uint8_t>(testType)],
                         static_cast<uint16_t>(loadLevel)
                             | static_cast<uint16_t>(managerStatus)
                             | static_cast<uint16_t>(testerStatus));

    // Exit critical section
    portEXIT_CRITICAL(&mux_);
  }
  EventBits_t getEventBits(TestType testType) {
    // Enter critical section
    portENTER_CRITICAL(&mux_);

    // Get the current event bits from the appropriate event group
    EventBits_t bits
        = xEventGroupGetBits(eventGroups_[static_cast<uint8_t>(testType)]);

    // Exit critical section
    portEXIT_CRITICAL(&mux_);

    return bits;
  }

private:
  TestSync() {

    for (int i = 0; i < NUM_TEST_TYPES; ++i) {
      eventGroups_[i] = xEventGroupCreate();
      if (eventGroups_[i] == NULL) {
      }
    }
  }

  ~TestSync() {

    for (int i = 0; i < NUM_TEST_TYPES; ++i) {
      if (eventGroups_[i] != NULL) {
        vEventGroupDelete(eventGroups_[i]);
      }
    }
  }

  static constexpr int NUM_TEST_TYPES = 6;  // Number of test types
  EventGroupHandle_t eventGroups_[NUM_TEST_TYPES];
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;

  TestSync(const TestSync&) = delete;
  TestSync& operator=(const TestSync&) = delete;
};
#endif  // TEST_SYNC_H
