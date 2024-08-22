#ifndef TEST_REQ_H
#define TEST_REQ_H
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
    // Ensure we don't exceed the bit limit
    if (count > 2) {
      count = 2;  // Limit to 2 tests to avoid exceeding 18 bits
    }
    resetTests();  // Clear previous tests

    for (size_t i = 0; i < count; ++i) {
      int testIndex = i;
      int bitShift = testIndex * 9;
      if (bitShift + 9 > 18) {
        break;  // Stop if adding this test would exceed the 18-bit limit
      }
      updateTest(testIndex, testList[i].testName, testList[i].level,
                 TestStatusManager::PENDING);
    }
  }

  // For the both side
  void updateTest(int testIndex, TestType testType, LoadLevel loadLevel,
                  TestStatusManager managerStatus,
                  TestStatusTester testerStatus) {
    // Calculate the bits for the specific test and load level
    EventBits_t testMask = encodeEventBits(testIndex, testType, loadLevel,
                                           TestStatusManager::NOT_IN_QUEUE,
                                           TestStatusTester::NOT_STARTED);

    // Clear only the bits related to this specific test and load level
    xEventGroupClearBits(eventGroup, testMask);

    // Encode the new status and set the bits
    EventBits_t newEventBits = encodeEventBits(testIndex, testType, loadLevel,
                                               managerStatus, testerStatus);
    xEventGroupSetBits(eventGroup, newEventBits);
  }
  // Update test status from the manager side
  void updateTest(int testIndex, TestType testType, LoadLevel loadLevel,
                  TestStatusManager managerStatus) {
    EventBits_t currentBits = xEventGroupGetBits(eventGroup);

    // Extract the current tester status
    TestStatusTester currentTesterStatus
        = decodeTesterStatus(currentBits, testIndex);

    // Mask out the current bits related to the manager status for this test
    EventBits_t testMask
        = encodeEventBits(testIndex, testType, loadLevel,
                          TestStatusManager::NOT_IN_QUEUE, currentTesterStatus);
    xEventGroupClearBits(eventGroup, testMask);

    // Encode the new manager status along with the preserved tester status
    EventBits_t newEventBits = encodeEventBits(
        testIndex, testType, loadLevel, managerStatus, currentTesterStatus);
    xEventGroupSetBits(eventGroup, newEventBits);
  }
  // Update test status from the tester side
  void updateTest(int testIndex, TestType testType, LoadLevel loadLevel,
                  TestStatusTester testerStatus) {
    EventBits_t currentBits = xEventGroupGetBits(eventGroup);

    // Extract the current manager status
    TestStatusManager currentManagerStatus
        = decodeManagerStatus(currentBits, testIndex);

    // Mask out the current bits related to the tester status for this test
    EventBits_t testMask
        = encodeEventBits(testIndex, testType, loadLevel, currentManagerStatus,
                          TestStatusTester::NOT_STARTED);
    xEventGroupClearBits(eventGroup, testMask);

    // Encode the new tester status along with the preserved manager status
    EventBits_t newEventBits = encodeEventBits(
        testIndex, testType, loadLevel, currentManagerStatus, testerStatus);
    xEventGroupSetBits(eventGroup, newEventBits);
  }
  // Get the pending test type for the tester
  TestType getNextTest() {
    EventBits_t eventBits = xEventGroupGetBits(eventGroup);

    for (int testIndex = 0; testIndex < 2; ++testIndex) {
      EventBits_t mask
          = 0b11 << (testIndex * 9 + 2);  // Mask for manager status bits
      EventBits_t pendingStatus
          = static_cast<EventBits_t>(TestStatusManager::PENDING)
            << (testIndex * 9 + 2);

      // Check if the test has a PENDING status
      if ((eventBits & mask) == pendingStatus) {
        return static_cast<TestType>((eventBits >> (testIndex * 9 + 6)) & 0b11);
      }
    }
    return TestType::SwitchTest;  // Default return if no PENDING test is found
  }

  // Get the load level for a specific test type
  LoadLevel getLoadLevel(int testIndex) {
    EventBits_t eventBits = xEventGroupGetBits(eventGroup);
    return static_cast<LoadLevel>((eventBits >> (testIndex * 9 + 4)) & 0b11);
  }

  // Get all pending tests from manager side
  int getPendingTests(RequiredTest pendingTests[], size_t maxTests) {
    EventBits_t eventBits = xEventGroupGetBits(eventGroup);
    size_t count = 0;

    for (int testIndex = 0; testIndex < 2; ++testIndex) {
      EventBits_t mask = TEST_BITS_MASK << (testIndex * 9);
      EventBits_t testBits = (eventBits & mask) >> (testIndex * 9);

      if ((testBits & 0b11)
          == static_cast<EventBits_t>(TestStatusManager::PENDING)) {
        if (count >= maxTests) {
          break;  // Exit if we've reached the maximum number of tests to return
        }

        pendingTests[count].testName
            = static_cast<TestType>((testBits >> 6) & 0b11);
        pendingTests[count].level
            = static_cast<LoadLevel>((testBits >> 4) & 0b11);
        ++count;
      }
    }

    return count;  // Return the number of pending tests found
  }
  // Retrieve the TestType for a specific index in the pendingTests array
  TestType getTestType(const RequiredTest pendingTests[], size_t testIndex) {
    if (testIndex >= MAX_TESTS) {
      // Handle invalid index, maybe return a default or throw an exception
      return TestType::SwitchTest;  // Default or error value
    }
    return pendingTests[testIndex].testName;
  }

  // Retrieve the LoadLevel for a specific index in the pendingTests array
  LoadLevel getLoadLevel(const RequiredTest pendingTests[], size_t testIndex) {
    if (testIndex >= MAX_TESTS) {
      // Handle invalid index, maybe return a default or throw an exception
      return LoadLevel::LEVEL_25;  // Default or error value
    }
    return pendingTests[testIndex].level;
  }

  // Reset all tests (clears all bits)
  void resetTests() { xEventGroupClearBits(eventGroup, TEST_BITS_MASK); }

private:
  EventGroupHandle_t eventGroup;

  TestSync() { eventGroup = xEventGroupCreate(); }

  EventBits_t encodeEventBits(int testIndex, TestType testType,
                              LoadLevel loadLevel,
                              TestStatusManager managerStatus,
                              TestStatusTester testerStatus) {
    EventBits_t eventBits = (static_cast<EventBits_t>(testType) << 6)
                            | (static_cast<EventBits_t>(loadLevel) << 4)
                            | (static_cast<EventBits_t>(managerStatus) << 2)
                            | static_cast<EventBits_t>(testerStatus);

    // Shift the bits based on the test index
    return eventBits << (testIndex * 9);
  }

  EventBits_t createMask(TestType testType, LoadLevel loadLevel,
                         bool clearManagerStatus, bool clearTesterStatus) {
    EventBits_t mask = 0;
    if (clearManagerStatus) {
      mask |= ((static_cast<uint8_t>(testType) << 6)
               | (static_cast<uint8_t>(loadLevel) << 4));
    }
    if (clearTesterStatus) {
      mask |= (static_cast<uint8_t>(testType) << 6)
              | (static_cast<uint8_t>(loadLevel) << 4) | (0b11 << 2);
    }
    return mask;
  }
  // Decode current manager status from the event bits
  TestStatusManager decodeManagerStatus(EventBits_t eventBits, int testIndex) {
    // Shift the bits back to the original position and mask out the manager
    // status bits
    EventBits_t shiftedBits = (eventBits >> (testIndex * 9)) & 0b1100;
    return static_cast<TestStatusManager>(shiftedBits >> 2);
  }

  // Decode current tester status from the event bits
  TestStatusTester decodeTesterStatus(EventBits_t eventBits, int testIndex) {
    // Shift the bits back to the original position and mask out the tester
    // status bits
    EventBits_t shiftedBits = (eventBits >> (testIndex * 9)) & 0b11;
    return static_cast<TestStatusTester>(shiftedBits);
  }

  TestSync(const TestSync &) = delete;
  TestSync &operator=(const TestSync &) = delete;
  // Define all test bits for clearing
  static constexpr EventBits_t TEST_BITS_MASK = 0x3FFFF;  // Mask for 18 bits
  static constexpr int MAX_TESTS = 2;                     // 2* 18 bits
};
#endif