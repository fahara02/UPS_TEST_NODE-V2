#ifndef STATE_DEFINES_H_
#define STATE_DEFINES_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <cstdint>

namespace Node_Core {
using StateBits = uint32_t;

enum class State : StateBits {
  DEVICE_ON = 1 << 0,
  DEVICE_OK = 1 << 1,
  DEVICE_SETUP = 1 << 2,
  DEVICE_READY = 1 << 3,
  MANUAL_MODE = 1 << 4,
  AUTO_MODE = 1 << 5,
  TEST_START = 1 << 6,
  TEST_IN_PROGRESS = 1 << 7,
  CURRENT_TEST_OK = 1 << 8,
  READY_NEXT_TEST = 1 << 9,
  RETEST = 1 << 10,
  SYSTEM_PAUSED = 1 << 11,
  ALL_TEST_DONE = 1 << 12,
  START_FROM_SAVE = 1 << 13,
  RECOVER_DATA = 1 << 14,
  ADDENDUM_TEST_DATA = 1 << 15,
  FAILED_TEST = 1 << 16,
  TRANSPORT_DATA = 1 << 17,
  SYSTEM_TUNING = 1 << 18,
  FAULT = 1 << 19,
  // MANUAL_NEXT_TEST = 1 << 20,
  REPORT_AVAILABLE = 1 << 20,
  // NEW_TEST = 1 << 22,
  // LOG_ERROR = 1 << 23,
};

enum class Event : EventBits_t {
  NONE = 1 << 0,
  ERROR = 1 << 1,
  SELF_CHECK_OK = 1 << 2,
  LOAD_BANK_CHECKED = 1 << 3,
  USER_TUNE = 1 << 4,
  NETWORK_DISCONNECTED = 1 << 5,
  SETTING_LOADED = 1 << 6,
  MANUAL_OVERRIDE = 1 << 7,
  AUTO_TEST_CMD = 1 << 8,
  INPUT_OUTPUT_READY = 1 << 9,
  DATA_CAPTURED = 1 << 10,
  VALID_DATA = 1 << 11,
  SAVE = 1 << 12,
  TEST_LIST_EMPTY = 1 << 13,
  JSON_READY = 1 << 14,
  MANUAL_DATA_ENTRY = 1 << 15,
  TEST_FAILED = 1 << 16,
  RETEST = 1 << 17,
  SYSTEM_FAULT = 1 << 18,
  FAULT_CLEARED = 1 << 19,
  USER_PAUSED = 1 << 20,
  REPORT_SEND = 1 << 21,
  RESTART = 1 << 22,
};

static const char* stateToString(State state) {
  switch (state) {
    case State::DEVICE_ON:
      return "DEVICE_ON";
    case State::DEVICE_OK:
      return "DEVICE_OK";
    case State::DEVICE_SETUP:
      return "DEVICE_SETUP";
    case State::DEVICE_READY:
      return "DEVICE_READY";
    case State::MANUAL_MODE:
      return "MANUAL_MODE";
    case State::AUTO_MODE:
      return "AUTO_MODE";
    case State::TEST_START:
      return "TEST_START";
    case State::TEST_IN_PROGRESS:
      return "TEST_IN_PROGRESS";
    case State::CURRENT_TEST_OK:
      return "CURRENT_TEST_OK";
    case State::READY_NEXT_TEST:
      return "READY_NEXT_TEST";
    case State::RETEST:
      return "RETEST";
    case State::SYSTEM_PAUSED:
      return "SYSTEM_PAUSED";
    case State::ALL_TEST_DONE:
      return "ALL_TEST_DONE";
    case State::START_FROM_SAVE:
      return "START_FROM_SAVE";
    case State::RECOVER_DATA:
      return "RECOVER_DATA";
    case State::ADDENDUM_TEST_DATA:
      return "ADDENDUM_TEST_DATA";
    case State::FAILED_TEST:
      return "FAILED_TEST";
    case State::TRANSPORT_DATA:
      return "TRANSPORT_DATA";
    case State::SYSTEM_TUNING:
      return "SYSTEM_TUNING";
    case State::FAULT:
      return "FAULT";
    // case State::MANUAL_NEXT_TEST:
    //   return "MANUAL_NEXT_TEST";
    // case State::REPORT_AVAILABLE:
    //   return "REPORT_AVAILABLE";
    // case State::NEW_TEST:
    //   return "NEW_TEST";
    // case State::LOG_ERROR:
    //   return "LOG_ERROR";
    default:
      return "UNKNOWN_STATE";
  }
}

static const char* eventToString(Event event) {
  switch (event) {
    case Event::NONE:
      return "NONE";
    case Event::ERROR:
      return "ERROR";
    case Event::SELF_CHECK_OK:
      return "SELF_CHECK_OK";
    case Event::LOAD_BANK_CHECKED:
      return "LOAD_BANK_CHECKED";
    case Event::USER_TUNE:
      return "USER_TUNE";
    case Event::NETWORK_DISCONNECTED:
      return "NETWORK_DISCONNECTED";
    case Event::SETTING_LOADED:
      return "SETTING_LOADED";
    case Event::MANUAL_OVERRIDE:
      return "MANUAL_OVERRIDE";
    case Event::AUTO_TEST_CMD:
      return "AUTO_TEST_CMD";
    case Event::INPUT_OUTPUT_READY:
      return "INPUT_OUTPUT_READY";
    case Event::DATA_CAPTURED:
      return "DATA_CAPTURED";
    case Event::VALID_DATA:
      return "VALID_DATA";
    case Event::SAVE:
      return "SAVE";
    case Event::TEST_LIST_EMPTY:
      return "TEST_LIST_EMPTY";
    case Event::JSON_READY:
      return "JSON_READY";
    case Event::MANUAL_DATA_ENTRY:
      return "MANUAL_DATA_ENTRY";
    case Event::TEST_FAILED:
      return "TEST_FAILED";
    case Event::RETEST:
      return "RETEST";
    case Event::SYSTEM_FAULT:
      return "SYSTEM_FAULT";
    case Event::FAULT_CLEARED:
      return "FAULT_CLEARED";
    case Event::USER_PAUSED:
      return "USER_PAUSED";
    case Event::REPORT_SEND:
      return "REPORT_SEND";
    case Event::RESTART:
      return "RESTART";
    default:
      return "UNKNOWN_EVENT";
  }
}

}  // namespace Node_Core

#endif  // STATE_DEFINES_H_
