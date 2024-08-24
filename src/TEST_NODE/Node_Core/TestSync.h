#ifndef TEST_SYNC_H
#define TEST_SYNC_H

#include "Arduino.h"
#include "Logger.h"
#include "TestData.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

enum class LoadLevel : uint8_t {
  LEVEL_25,  // 0
  LEVEL_50,  // 1
  LEVEL_75,  // 2
  LEVEL_100  // 3
};

enum class TestStatusManager {
  NOT_IN_QUEUE = 0b00,
  PENDING = 0b01,
  RETEST = 0b10,
  DONE = 0b11
};

enum class TestStatusTester {
  NOT_STARTED = 0b00,
  RUNNING = 0b01,
  SUCCESS = 0b10,
  FAILED = 0b11
};

struct RequiredTest {
  TestType testName;
  LoadLevel level;
};

using namespace Node_Core;
extern Logger& logger;

class TestSync {
public:
  static TestSync& getInstance() {
    static TestSync instance;
    return instance;
  }

  void testBitEncoding() {
    EventBits_t testBits = encodeEventBits(
        0, TestType::SwitchTest, LoadLevel::LEVEL_25,
        TestStatusManager::PENDING, TestStatusTester::NOT_STARTED);
    EventBits_t shiftedBits = testBits << (0 * 9);
    logger.log(LogLevel::WARNING, "Test bits: %d", testBits);
    logger.logBinary(LogLevel::WARNING, shiftedBits);
  }

  void addTests(RequiredTest testList[], size_t numTests) {
    logger.log(LogLevel::WARNING, "Starting addTests...");
    logger.log(LogLevel::WARNING, "Number of tests to add: ", numTests);

    if (numTests > MAX_TESTS) {
      numTests = MAX_TESTS;
    }

    resetTests();

    for (size_t i = 0; i < numTests; ++i) {
      logger.log(LogLevel::WARNING, "Adding test %d", i);
      logger.log(LogLevel::WARNING,
                 "Test type: ", static_cast<int>(testList[i].testName));
      logger.log(LogLevel::WARNING,
                 "Load level: ", static_cast<int>(testList[i].level));

      int groupIndex = i / TESTS_PER_GROUP;
      int bitIndexWithinGroup = i % TESTS_PER_GROUP;
      if (groupIndex >= EVENT_GROUP_COUNT) {
        logger.log(LogLevel::WARNING, "Group index exceeded, breaking...");
        break;
      }

      // Encode the bits
      EventBits_t eventBits = encodeEventBits(
          bitIndexWithinGroup, testList[i].testName, testList[i].level,
          TestStatusManager::PENDING, TestStatusTester::NOT_STARTED);

      // Shift the bits to the correct position within the event group
      EventBits_t shiftedBits = eventBits << (bitIndexWithinGroup * 9);

      logger.logBinary(LogLevel::WARNING, shiftedBits);

      xEventGroupSetBits(eventGroups[groupIndex], shiftedBits);
    }

    logger.log(LogLevel::WARNING, "Finished adding tests.");
  }

  // update test for any side
  void updateTest(int testIndex, TestType testType, LoadLevel loadLevel,
                  TestStatusManager managerStatus,
                  TestStatusTester testerStatus) {
    int groupIndex = testIndex / TESTS_PER_GROUP;
    // int bitIndex = testIndex % TESTS_PER_GROUP;

    if (groupIndex >= EVENT_GROUP_COUNT)
      return;

    EventBits_t testMask = encodeEventBits(testIndex, testType, loadLevel,
                                           TestStatusManager::NOT_IN_QUEUE,
                                           TestStatusTester::NOT_STARTED);
    xEventGroupClearBits(eventGroups[groupIndex], testMask);

    EventBits_t newEventBits = encodeEventBits(testIndex, testType, loadLevel,
                                               managerStatus, testerStatus);
    xEventGroupSetBits(eventGroups[groupIndex], newEventBits);
  }
  // update test by manager
  void updateTest(int testIndex, TestType testType, LoadLevel loadLevel,
                  TestStatusManager managerStatus) {
    int groupIndex = testIndex / TESTS_PER_GROUP;
    // int bitIndex = testIndex % TESTS_PER_GROUP;

    if (groupIndex >= EVENT_GROUP_COUNT)
      return;

    EventBits_t currentBits = xEventGroupGetBits(eventGroups[groupIndex]);
    TestStatusTester currentTesterStatus
        = decodeTesterStatus(currentBits, testIndex);

    EventBits_t testMask
        = encodeEventBits(testIndex, testType, loadLevel,
                          TestStatusManager::NOT_IN_QUEUE, currentTesterStatus);
    xEventGroupClearBits(eventGroups[groupIndex], testMask);

    EventBits_t newEventBits = encodeEventBits(
        testIndex, testType, loadLevel, managerStatus, currentTesterStatus);
    xEventGroupSetBits(eventGroups[groupIndex], newEventBits);
  }
  // update test by tester
  void updateTest(int testIndex, TestType testType, LoadLevel loadLevel,
                  TestStatusTester testerStatus) {
    int groupIndex = testIndex / TESTS_PER_GROUP;
    //  int bitIndex = testIndex % TESTS_PER_GROUP;

    if (groupIndex >= EVENT_GROUP_COUNT)
      return;

    EventBits_t currentBits = xEventGroupGetBits(eventGroups[groupIndex]);
    TestStatusManager currentManagerStatus
        = decodeManagerStatus(currentBits, testIndex);

    EventBits_t testMask
        = encodeEventBits(testIndex, testType, loadLevel, currentManagerStatus,
                          TestStatusTester::NOT_STARTED);
    xEventGroupClearBits(eventGroups[groupIndex], testMask);

    EventBits_t newEventBits = encodeEventBits(
        testIndex, testType, loadLevel, currentManagerStatus, testerStatus);
    xEventGroupSetBits(eventGroups[groupIndex], newEventBits);
  }

  TestType getNextTest() {
    for (int groupIndex = 0; groupIndex < EVENT_GROUP_COUNT; ++groupIndex) {
      EventBits_t eventBits = xEventGroupGetBits(eventGroups[groupIndex]);

      for (int testIndex = 0; testIndex < TESTS_PER_GROUP; ++testIndex) {
        int bitOffset = testIndex * 9;  // Each test occupies 9 bits
        EventBits_t statusMask = 0b11
                                 << (bitOffset + 2);  // Extract TestStatus bits
        EventBits_t pendingStatus
            = static_cast<EventBits_t>(TestStatusManager::PENDING)
              << (bitOffset + 2);

        if ((eventBits & statusMask) == pendingStatus) {
          EventBits_t typeMask = 0b11
                                 << (bitOffset + 6);  // Extract TestType bits
          return static_cast<TestType>((eventBits & typeMask)
                                       >> (bitOffset + 6));
        }
      }
    }
    return TestType::SwitchTest;  // Default if no pending tests found
  }

  LoadLevel getLoadLevel(int testIndex) {
    int groupIndex = testIndex / TESTS_PER_GROUP;
    int bitIndex = testIndex % TESTS_PER_GROUP;

    if (groupIndex >= EVENT_GROUP_COUNT)
      return LoadLevel::LEVEL_25;

    EventBits_t eventBits = xEventGroupGetBits(eventGroups[groupIndex]);
    return static_cast<LoadLevel>((eventBits >> (bitIndex * 9 + 4)) & 0b11);
  }

  int getPendingTests(RequiredTest pendingTests[]) {
    size_t count = 0;

    for (int groupIndex = 0; groupIndex < EVENT_GROUP_COUNT; ++groupIndex) {
      EventBits_t eventBits = xEventGroupGetBits(eventGroups[groupIndex]);

      for (int testIndex = 0; testIndex < TESTS_PER_GROUP; ++testIndex) {
        int bitOffset = testIndex * 9;
        EventBits_t statusMask = 0b11 << (bitOffset + 2);
        EventBits_t pendingStatus
            = static_cast<EventBits_t>(TestStatusManager::PENDING)
              << (bitOffset + 2);

        if ((eventBits & statusMask) == pendingStatus) {
          if (count >= MAX_TESTS) {
            return count;  // Stop if we've reached the maximum number of tests
          }

          EventBits_t levelMask = 0b11 << (bitOffset + 4);
          EventBits_t typeMask = 0b11 << (bitOffset + 6);

          pendingTests[count].testName = static_cast<TestType>(
              (eventBits & typeMask) >> (bitOffset + 6));
          pendingTests[count].level = static_cast<LoadLevel>(
              (eventBits & levelMask) >> (bitOffset + 4));
          ++count;
        }
      }
    }
    return count;
  }

  int getPendingTestNumber() {
    int pendingTestCount = 0;

    for (int groupIndex = 0; groupIndex < EVENT_GROUP_COUNT; ++groupIndex) {
      EventBits_t eventBits = xEventGroupGetBits(eventGroups[groupIndex]);

      for (int testIndex = 0; testIndex < TESTS_PER_GROUP; ++testIndex) {
        EventBits_t mask = 0b11 << (testIndex * 9 + 2);
        EventBits_t pendingStatus
            = static_cast<EventBits_t>(TestStatusManager::PENDING)
              << (testIndex * 9 + 2);

        if ((eventBits & mask) == pendingStatus) {
          ++pendingTestCount;
        }
      }
    }

    return pendingTestCount;
  }

  TestType getTestType(const RequiredTest pendingTests[], size_t testIndex) {
    if (testIndex >= MAX_TESTS) {
      return TestType::SwitchTest;
    }
    return pendingTests[testIndex].testName;
  }

  LoadLevel getLoadLevel(const RequiredTest pendingTests[], size_t testIndex) {
    if (testIndex >= MAX_TESTS) {
      return LoadLevel::LEVEL_25;
    }
    return pendingTests[testIndex].level;
  }

  void resetTests() {
    for (int groupIndex = 0; groupIndex < EVENT_GROUP_COUNT; ++groupIndex) {
      xEventGroupClearBits(eventGroups[groupIndex], TEST_BITS_MASK);
    }
  }

private:
  static constexpr int MAX_TESTS = 10;
  static constexpr int TESTS_PER_GROUP = 2;
  static constexpr int EVENT_GROUP_COUNT
      = (MAX_TESTS + TESTS_PER_GROUP - 1) / TESTS_PER_GROUP;
  static constexpr EventBits_t TEST_BITS_MASK = 0x3FFFF;

  EventGroupHandle_t eventGroups[EVENT_GROUP_COUNT];

  TestSync() {
    for (int i = 0; i < EVENT_GROUP_COUNT; ++i) {
      eventGroups[i] = xEventGroupCreate();
    }
  }

  EventBits_t encodeEventBits(int localgroup_testIndex, TestType testType,
                              LoadLevel loadLevel,
                              TestStatusManager managerStatus,
                              TestStatusTester testerStatus) {

    if (localgroup_testIndex > 1) {
      localgroup_testIndex = 1;
    }
    // Encoding each field with appropriate bit positions
    EventBits_t eventBits
        = (static_cast<EventBits_t>(testType) << 6)         // TestType
          | (static_cast<EventBits_t>(loadLevel) << 4)      // LoadLevel
          | (static_cast<EventBits_t>(managerStatus) << 2)  // ManagerStatus
          | static_cast<EventBits_t>(testerStatus);         // TesterStatus
    return eventBits << (localgroup_testIndex * 9);
  }

  TestStatusManager decodeManagerStatus(EventBits_t eventBits, int testIndex) {
    int bitIndex = testIndex % TESTS_PER_GROUP;
    return static_cast<TestStatusManager>((eventBits >> (bitIndex * 9 + 2))
                                          & 0b11);
  }

  TestStatusTester decodeTesterStatus(EventBits_t eventBits, int testIndex) {
    int bitIndex = testIndex % TESTS_PER_GROUP;
    return static_cast<TestStatusTester>((eventBits >> (bitIndex * 9 + 6))
                                         & 0b11);
  }
};

#endif  // TEST_SYNC_H
