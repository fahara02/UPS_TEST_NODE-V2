#ifndef TEST_SYNC_H
#define TEST_SYNC_H

#include "Arduino.h"
#include "Logger.h"
#include "TestData.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

using namespace Node_Core;
extern Logger &logger;
extern EventGroupHandle_t eventGroupTest;
constexpr int MAX_TEST = 6;

class TestSync {
public:
  static TestSync &getInstance() {
    static TestSync instance;
    return instance;
  }
  void init() {
    eventGroupTest = xEventGroupCreate();
    resetAllBits();  // Clear all event bits after creating the event group
    logger.log(LogLevel::INFO, "testSync initialisation");
  }
  void startTest(TestType test) {
    EventBits_t test_eventbits = static_cast<EventBits_t>(test);
    xEventGroupSetBits(eventGroupTest, test_eventbits);
    logger.log(LogLevel::SUCCESS, "for test %s set Eventbit to->",
               testTypeToString(test));
    int result = xEventGroupGetBits(eventGroupTest);
    logger.logBinary(LogLevel::WARNING, result);
  };
  void stopTest(TestType test) {
    EventBits_t test_eventbits = static_cast<EventBits_t>(test);
    xEventGroupClearBits(eventGroupTest, test_eventbits);
    logger.log(LogLevel::SUCCESS, "for test %s cleared Eventbit to->",
               testTypeToString(test));
    int result = xEventGroupGetBits(eventGroupTest);
    logger.logBinary(LogLevel::WARNING, result);
    vTaskDelay(pdMS_TO_TICKS(50));
  };

private:
  TestSync() {}

  TestSync(const TestSync &) = delete;
  TestSync &operator=(const TestSync &) = delete;

  void resetAllBits() {
    xEventGroupClearBits(eventGroupTest, ALL_TEST_BITS);
    int result = xEventGroupGetBits(eventGroupTest);
    logger.logBinary(LogLevel::WARNING, result);
  }

  static const EventBits_t ALL_TEST_BITS = (1 << MAX_TEST) - 1;
  // Dynamically calculate all bits
};

#endif  // TEST_SYNC_H
