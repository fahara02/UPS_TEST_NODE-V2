#ifndef STATE_DEFINES_H_
#define STATE_DEFINES_H_
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <cstdint>

namespace Node_Core {
using StateBits = uint32_t;

enum class State : StateBits {
  DEVICE_ON = 1 << 0,            // 00000001
  DEVICE_OK = 1 << 1,            // 00000010
  DEVICE_SETUP = 1 << 2,         // 00000100
  DEVICE_READY = 1 << 3,         // 00001000
  MANUAL_MODE = 1 << 4,          // 00010000
  AUTO_MODE = 1 << 5,            // 00100000
  TEST_START = 1 << 6,           // 01000000
  TEST_IN_PROGRESS = 1 << 7,     // 010000000
  CURRENT_TEST_OK = 1 << 8,      // 0100000000
  READY_NEXT_TEST = 1 << 9,      // 01000000000
  RETEST = 1 << 10,              // 010000000000
  SYSTEM_PAUSED = 1 << 11,       // 0100000000000
  ALL_TEST_DONE = 1 << 12,       // 01000000000000
  START_FROM_SAVE = 1 << 13,     // 010000000000000
  RECOVER_DATA = 1 << 14,        // 0100000000000000
  ADDENDUM_TEST_DATA = 1 << 15,  // 01000000000000000
  FAILED_TEST = 1 << 16,         // 010000000000000000
  TRANSPORT_DATA = 1 << 17,      // 0100000000000000000
  SYSTEM_TUNING = 1 << 18,       // 01000000000000000000
  FAULT = 1 << 19,               // 010000000000000000000
  MANUAL_NEXT_TEST = 1 << 20,    // 0100000000000000000000

  // INFO BITS WHICH WILL BE  SET AND CLEARED  WITH THE STATE MACHINE BUT NOT
  // PART OF ACTUAL STATE
  REPORT_AVAILABLE = 1 << 20,
  NEW_SETTING = 1 << 21,
  LOG_ERROR = 1 << 22,

};

enum class Event : EventBits_t {
  NONE = 1 << 0,                  // 00000000000000000000000000000001
  ERROR = 1 << 1,                 // 00000000000000000000000000000010
  SELF_CHECK_OK = 1 << 2,         // 00000000000000000000000000000100
  LOAD_BANK_CHECKED = 1 << 3,     // 00000000000000000000000000001000
  USER_TUNE = 1 << 4,             // 00000000000000000000000000010000
  NETWORK_DISCONNECTED = 1 << 5,  // 00000000000000000000000000100000
  SETTING_LOADED = 1 << 6,        // 00000000000000000000000001000000
  MANUAL_OVERRIDE = 1 << 7,       // 00000000000000000000000010000000
  AUTO_TEST_CMD = 1 << 8,         // 00000000000000000000000100000000
  INPUT_OUTPUT_READY = 1 << 9,    // 00000000000000000000001000000000
  DATA_CAPTURED = 1 << 10,        // 00000000000000000000010000000000
  VALID_DATA = 1 << 11,           // 00000000000000000000100000000000
  SAVE = 1 << 12,                 // 00000000000000000001000000000000
  TEST_LIST_EMPTY = 1 << 13,      // 00000000000000000010000000000000
  JSON_READY = 1 << 14,           // 00000000000000000100000000000000
  MANUAL_DATA_ENTRY = 1 << 15,    // 00000000000000001000000000000000
  TEST_FAILED = 1 << 16,          // 00000000000000010000000000000000
  RETEST = 1 << 17,               // 00000000000000100000000000000000
  SYSTEM_FAULT = 1 << 18,         // 00000000000001000000000000000000
  FAULT_CLEARED = 1 << 19,        // 00000000000010000000000000000000
  USER_PAUSED = 1 << 20,          // 00000000000100000000000000000000
  REPORT_SEND = 1 << 21,          // 00000000001000000000000000000000
  RESTART = 1 << 22,              // 00000000010000000000000000000000

};

}  // namespace Node_Core
#endif