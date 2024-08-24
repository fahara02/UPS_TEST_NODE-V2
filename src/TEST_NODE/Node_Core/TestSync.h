#ifndef TEST_SYNC_H
#define TEST_SYNC_H

#include "TestData.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

enum class LoadLevel { LEVEL_25 = 0, LEVEL_50, LEVEL_75, LEVEL_100 };

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

class TestSync {
public:
  static TestSync &getInstance() {
    static TestSync instance;
    return instance;
  }

  void addTests(RequiredTest testList[], size_t count) {
    if (count > MAX_TESTS) {
      count = MAX_TESTS;
    }

    resetTests();

    for (size_t i = 0; i < count; ++i) {  // Correct loop condition
      int groupIndex = i / TESTS_PER_GROUP;
      int bitIndex = i % TESTS_PER_GROUP;
      if (groupIndex >= EVENT_GROUP_COUNT)
        break;

      EventBits_t eventBits = encodeEventBits(
          i, testList[i].testName, testList[i].level,
          TestStatusManager::PENDING, TestStatusTester::NOT_STARTED);

      xEventGroupSetBits(eventGroups[groupIndex], eventBits);
    }
  }
  // update test for any side
  void updateTest(int testIndex, TestType testType, LoadLevel loadLevel,
                  TestStatusManager managerStatus,
                  TestStatusTester testerStatus) {
    int groupIndex = testIndex / TESTS_PER_GROUP;
    int bitIndex = testIndex % TESTS_PER_GROUP;

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
    int bitIndex = testIndex % TESTS_PER_GROUP;

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
    int bitIndex = testIndex % TESTS_PER_GROUP;

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
        EventBits_t mask = 0b11 << (testIndex * 9 + 2);
        EventBits_t pendingStatus
            = static_cast<EventBits_t>(TestStatusManager::PENDING)
              << (testIndex * 9 + 2);

        if ((eventBits & mask) == pendingStatus) {
          return static_cast<TestType>((eventBits >> (testIndex * 9 + 6))
                                       & 0b11);
        }
      }
    }
    return TestType::SwitchTest;
  }

  LoadLevel getLoadLevel(int testIndex) {
    int groupIndex = testIndex / TESTS_PER_GROUP;
    int bitIndex = testIndex % TESTS_PER_GROUP;

    if (groupIndex >= EVENT_GROUP_COUNT)
      return LoadLevel::LEVEL_25;

    EventBits_t eventBits = xEventGroupGetBits(eventGroups[groupIndex]);
    return static_cast<LoadLevel>((eventBits >> (bitIndex * 9 + 4)) & 0b11);
  }

  int getPendingTests(RequiredTest pendingTests[], size_t maxTests) {
    size_t count = 0;

    for (int groupIndex = 0; groupIndex < EVENT_GROUP_COUNT; ++groupIndex) {
      EventBits_t eventBits = xEventGroupGetBits(eventGroups[groupIndex]);

      for (int testIndex = 0; testIndex < TESTS_PER_GROUP; ++testIndex) {
        EventBits_t mask = 0b11 << (testIndex * 9 + 2);
        EventBits_t pendingStatus
            = static_cast<EventBits_t>(TestStatusManager::PENDING)
              << (testIndex * 9 + 2);

        if ((eventBits & mask) == pendingStatus) {
          if (count >= maxTests)
            return count;

          pendingTests[count].testName = static_cast<TestType>(
              (eventBits >> (testIndex * 9 + 6)) & 0b11);
          pendingTests[count].level = static_cast<LoadLevel>(
              (eventBits >> (testIndex * 9 + 4)) & 0b11);
          ++count;
        }
      }
    }
    return count;
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
  static constexpr int TESTS_PER_GROUP = 18;
  static constexpr int EVENT_GROUP_COUNT
      = (MAX_TESTS + TESTS_PER_GROUP - 1) / TESTS_PER_GROUP;
  static constexpr EventBits_t TEST_BITS_MASK = 0x3FFFF;

  EventGroupHandle_t eventGroups[EVENT_GROUP_COUNT];

  TestSync() {
    for (int i = 0; i < EVENT_GROUP_COUNT; ++i) {
      eventGroups[i] = xEventGroupCreate();
    }
  }

  EventBits_t encodeEventBits(int testIndex, TestType testType,
                              LoadLevel loadLevel,
                              TestStatusManager managerStatus,
                              TestStatusTester testerStatus) {
    EventBits_t eventBits = (static_cast<EventBits_t>(testType) << 6)
                            | (static_cast<EventBits_t>(loadLevel) << 4)
                            | (static_cast<EventBits_t>(managerStatus) << 2)
                            | static_cast<EventBits_t>(testerStatus);

    int bitIndex = testIndex % TESTS_PER_GROUP;
    return eventBits << (bitIndex * 9);
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
