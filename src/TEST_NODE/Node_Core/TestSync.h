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

class TestSync {
public:
  static TestSync &getInstance() {
    static TestSync instance;
    return instance;
  }

  void startTest(TestType test) {
    EventBits_t test_eventbits = static_cast<EventBits_t>(test);
    xEventGroupSetBits(eventGroupTest, test_eventbits);
    logger.log(LogLevel::SUCCESS, "for test %s set Eventbit to->",
               testTypeToString(test));
    logger.logBinary(LogLevel::WARNING, test_eventbits);
  };
  void stopTest(TestType test) {
    EventBits_t test_eventbits = static_cast<EventBits_t>(test);
    xEventGroupClearBits(eventGroupTest, test_eventbits);
    logger.log(LogLevel::SUCCESS, "for test %s cleared Eventbit to->",
               testTypeToString(test));
    logger.logBinary(LogLevel::WARNING, test_eventbits);
    vTaskDelay(pdMS_TO_TICKS(50));
  };

private:
  TestSync() { eventGroupTest = xEventGroupCreate(); }

  TestSync(const TestSync &) = delete;
  TestSync &operator=(const TestSync &) = delete;
};
#endif  // TEST_SYNC_H
