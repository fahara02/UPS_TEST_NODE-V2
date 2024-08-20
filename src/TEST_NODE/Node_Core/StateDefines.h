#ifndef STATE_DEFINES_H_
#define STATE_DEFINES_H_
#include <cstring>

namespace Node_Core {

enum class State {

  DEVICE_SHUTDOWN = 0,
  DEVICE_ON = 1,
  DEVICE_OK = 2,
  DEVICE_CONNECTED = 3,
  DEVICE_READY = 4,
  DEVICE_DISCONNECTED = 5,
  RECONNECT_NETWORK = 6,
  NETWORK_TIMEOUT = 7,
  RETEST = 8,
  MAX_RETEST = 9,
  MANUAL_MODE,
  AUTO_MODE,
  SWITCHING_TEST_START,
  SWITCHING_TEST_25P_START,
  SWITCHING_TEST_25P_DONE,
  SWITCHING_TEST_50P_START,
  SWITCHING_TEST_50P_DONE,
  SWITCHING_TEST_75P_START,
  SWITCHING_TEST_75P_DONE,
  SWITCHING_TEST_FULLLOAD_START,
  SWITCHING_TEST_FULLLOAD_DONE,
  SWITCHING_TEST_CHECK,
  SWITCHING_TEST_OK,
  SWITCHING_TEST_FAILED,
  READY_NEXT_TEST,
  EFFICIENCY_TEST_START,
  EFFICIENCY_TEST_DONE,
  EFFICIENCY_TEST_CHECK,
  EFFICIENCY_TEST_OK,
  EFFICIENCY_TEST_FAILED,
  BACKUP_TIME_TEST_START,
  BACKUP_TIME_TEST_DONE,
  BACKUP_TIME_TEST_CHECK,
  BACKUP_TIME_TEST_OK,
  BACKUP_TIME_TEST_FAILED,
  SAVE_TEST_DATA,
  ALL_TEST_DONE,
  ADDENDUM_TEST_DATA,
  REPORT_AVAILABLE,
  PRINT_TEST_DATA,
  FAULT,
  IDLE,

};

enum class Event {
  NONE = -1,
  ERROR = 0,
  POWER_ON,
  POWER_OFF,
  SELF_CHECK_OK,
  WIFI_CONNECTED,
  WIFI_DISCONNECTED,
  SETTING_LOADED,
  MANUAL_OVERRRIDE,
  AUTO_TEST_CMD,
  LOAD_BANK_ONLINE,
  LOAD_ON_OFF_25P,
  LOAD_ON_OFF_50P,
  LOAD_ON_OFF_75P,
  FULL_LOAD_ON_OFF,
  TIME_CAPTURE_OK,
  VALIDATE_TEST,
  INPUT_OUTPUT_READY,
  MESURED_DATA_RECEIVED,
  POWER_MEASURE_OK,
  TIMER_READY,
  VALID_BACKUP_TIME,
  SAVE,
  DATA,
  MANUAL_DATA_ENTRY,
  TRANSPORT_DATA,
  PRINT_DATA,
  TEST_SUCCESS,
  TEST_FAILED,
  RETEST,
  RETEST_OK,
  RETRY_CONNECT,
  RETRY_OK,
  SYSTEM_FAULT,
  POWER_DEVICE_DOWN,
  SWITCHING_DEVICE_DOWN,
  RESTART

};

static const char *stateToString(State state) {
  switch (state) {
    case State::DEVICE_SHUTDOWN:
      return "DEVICE_SHUTDOWN";
    case State::DEVICE_ON:
      return "DEVICE_ON";
    case State::DEVICE_OK:
      return "DEVICE_OK";
    case State::DEVICE_CONNECTED:
      return "DEVICE_CONNECTED";
    case State::DEVICE_READY:
      return "DEVICE_READY";
    case State::DEVICE_DISCONNECTED:
      return "DEVICE_DISCONNECTED";
    case State::RECONNECT_NETWORK:
      return "RECONNECT_NETWORK";
    case State::NETWORK_TIMEOUT:
      return "NETWORK_TIMEOUT";
    case State::RETEST:
      return "RETEST";
    case State::MAX_RETEST:
      return "MAX_RETEST";
    case State::MANUAL_MODE:
      return "MANUAL_MODE";
    case State::AUTO_MODE:
      return "AUTO_MODE";
    case State::SWITCHING_TEST_START:
      return "SWITCHING_TEST_START";
    case State::SWITCHING_TEST_25P_START:
      return "SWITCHING_TEST_25P_START";
    case State::SWITCHING_TEST_25P_DONE:
      return "SWITCHING_TEST_25P_DONE";
    case State::SWITCHING_TEST_50P_START:
      return "SWITCHING_TEST_50P_START";
    case State::SWITCHING_TEST_50P_DONE:
      return "SWITCHING_TEST_50P_DONE";
    case State::SWITCHING_TEST_75P_START:
      return "SWITCHING_TEST_75P_START";
    case State::SWITCHING_TEST_75P_DONE:
      return "SWITCHING_TEST_75P_DONE";
    case State::SWITCHING_TEST_FULLLOAD_START:
      return "SWITCHING_TEST_FULLLOAD_START";
    case State::SWITCHING_TEST_FULLLOAD_DONE:
      return "SWITCHING_TEST_FULLLOAD_DONE";
    case State::SWITCHING_TEST_CHECK:
      return "SWITCHING_TEST_CHECK";
    case State::SWITCHING_TEST_OK:
      return "SWITCHING_TEST_OK";
    case State::SWITCHING_TEST_FAILED:
      return "SWITCHING_TEST_FAILED";
    case State::READY_NEXT_TEST:
      return "READY_NEXT_TEST";
    case State::EFFICIENCY_TEST_START:
      return "EFFICIENCY_TEST_START";
    case State::EFFICIENCY_TEST_DONE:
      return "EFFICIENCY_TEST_DONE";
    case State::EFFICIENCY_TEST_CHECK:
      return "EFFICIENCY_TEST_CHECK";
    case State::EFFICIENCY_TEST_OK:
      return "EFFICIENCY_TEST_OK";
    case State::EFFICIENCY_TEST_FAILED:
      return "EFFICIENCY_TEST_FAILED";
    case State::BACKUP_TIME_TEST_START:
      return "BACKUP_TIME_TEST_START";
    case State::BACKUP_TIME_TEST_DONE:
      return "BACKUP_TIME_TEST_DONE";
    case State::BACKUP_TIME_TEST_CHECK:
      return "BACKUP_TIME_TEST_CHECK";
    case State::BACKUP_TIME_TEST_OK:
      return "BACKUP_TIME_TEST_OK";
    case State::BACKUP_TIME_TEST_FAILED:
      return "BACKUP_TIME_TEST_FAILED";
    case State::SAVE_TEST_DATA:
      return "SAVE_TEST_DATA";
    case State::ALL_TEST_DONE:
      return "ALL_TEST_DONE";
    case State::ADDENDUM_TEST_DATA:
      return "ADDENDUM_TEST_DATA";
    case State::REPORT_AVAILABLE:
      return "REPORT_AVAILABLE";
    case State::PRINT_TEST_DATA:
      return "PRINT_TEST_DATA";
    case State::FAULT:
      return "FAULT";
    case State::IDLE:
      return "IDLE";
    default:
      return "UNKNOWN_STATE";
  }
}

// Convert Event enum to string
static const char *eventToString(Event event) {
  switch (event) {
    case Event::NONE:
      return "NONE";
    case Event::ERROR:
      return "ERROR";
    case Event::POWER_ON:
      return "POWER_ON";
    case Event::POWER_OFF:
      return "POWER_OFF";
    case Event::SELF_CHECK_OK:
      return "SELF_CHECK_OK";
    case Event::WIFI_CONNECTED:
      return "WIFI_CONNECTED";
    case Event::WIFI_DISCONNECTED:
      return "WIFI_DISCONNECTED";
    case Event::SETTING_LOADED:
      return "SETTING_LOADED";
    case Event::MANUAL_OVERRRIDE:
      return "MANUAL_OVERRRIDE";
    case Event::AUTO_TEST_CMD:
      return "AUTO_TEST_CMD";
    case Event::LOAD_BANK_ONLINE:
      return "LOAD_BANK_ONLINE";
    case Event::LOAD_ON_OFF_25P:
      return "LOAD_ON_OFF_25P";
    case Event::LOAD_ON_OFF_50P:
      return "LOAD_ON_OFF_50P";
    case Event::LOAD_ON_OFF_75P:
      return "LOAD_ON_OFF_75P";
    case Event::FULL_LOAD_ON_OFF:
      return "FULL_LOAD_ON_OFF";
    case Event::TIME_CAPTURE_OK:
      return "TIME_CAPTURE_OK";
    case Event::VALIDATE_TEST:
      return "VALIDATE_TEST";
    case Event::INPUT_OUTPUT_READY:
      return "INPUT_OUTPUT_READY";
    case Event::MESURED_DATA_RECEIVED:
      return "MESURED_DATA_RECEIVED";
    case Event::POWER_MEASURE_OK:
      return "POWER_MEASURE_OK";
    case Event::TIMER_READY:
      return "TIMER_READY";
    case Event::VALID_BACKUP_TIME:
      return "VALID_BACKUP_TIME";
    case Event::SAVE:
      return "SAVE";
    case Event::DATA:
      return "DATA";
    case Event::MANUAL_DATA_ENTRY:
      return "MANUAL_DATA_ENTRY";
    case Event::TRANSPORT_DATA:
      return "TRANSPORT_DATA";
    case Event::PRINT_DATA:
      return "PRINT_DATA";
    case Event::TEST_SUCCESS:
      return "TEST_SUCCESS";
    case Event::TEST_FAILED:
      return "TEST_FAILED";
    case Event::RETEST:
      return "RETEST";
    case Event::RETEST_OK:
      return "RETEST_OK";
    case Event::RETRY_CONNECT:
      return "RETRY_CONNECT";
    case Event::RETRY_OK:
      return "RETRY_OK";

    case Event::SYSTEM_FAULT:
      return "SYSTEM_FAULT";
    case Event::POWER_DEVICE_DOWN:
      return "POWER_DEVICE_DOWN";
    case Event::SWITCHING_DEVICE_DOWN:
      return "SWITCHING_DEVICE_DOWN";
    case Event::RESTART:
      return "RESTART";
    default:
      return "UNKNOWN_EVENT";
  }
}

// Convert string to State enum
static State stringToState(const char *str) {
  if (std::strcmp(str, "DEVICE_SHUTDOWN") == 0)
    return State::DEVICE_SHUTDOWN;
  if (std::strcmp(str, "DEVICE_ON") == 0)
    return State::DEVICE_ON;
  if (std::strcmp(str, "DEVICE_OK") == 0)
    return State::DEVICE_OK;
  if (std::strcmp(str, "DEVICE_CONNECTED") == 0)
    return State::DEVICE_CONNECTED;
  if (std::strcmp(str, "DEVICE_READY") == 0)
    return State::DEVICE_READY;
  if (std::strcmp(str, "DEVICE_DISCONNECTED") == 0)
    return State::DEVICE_DISCONNECTED;
  if (std::strcmp(str, "RECONNECT_NETWORK") == 0)
    return State::RECONNECT_NETWORK;
  if (std::strcmp(str, "NETWORK_TIMEOUT") == 0)
    return State::NETWORK_TIMEOUT;
  if (std::strcmp(str, "RETEST") == 0)
    return State::RETEST;
  if (std::strcmp(str, "MAX_RETEST") == 0)
    return State::MAX_RETEST;
  if (std::strcmp(str, "MANUAL_MODE") == 0)
    return State::MANUAL_MODE;
  if (std::strcmp(str, "AUTO_MODE") == 0)
    return State::AUTO_MODE;
  if (std::strcmp(str, "SWITCHING_TEST_START") == 0)
    return State::SWITCHING_TEST_START;
  if (std::strcmp(str, "SWITCHING_TEST_25P_START") == 0)
    return State::SWITCHING_TEST_25P_START;
  if (std::strcmp(str, "SWITCHING_TEST_25P_DONE") == 0)
    return State::SWITCHING_TEST_25P_DONE;
  if (std::strcmp(str, "SWITCHING_TEST_50P_START") == 0)
    return State::SWITCHING_TEST_50P_START;
  if (std::strcmp(str, "SWITCHING_TEST_50P_DONE") == 0)
    return State::SWITCHING_TEST_50P_DONE;
  if (std::strcmp(str, "SWITCHING_TEST_75P_START") == 0)
    return State::SWITCHING_TEST_75P_START;
  if (std::strcmp(str, "SWITCHING_TEST_75P_DONE") == 0)
    return State::SWITCHING_TEST_75P_DONE;
  if (std::strcmp(str, "SWITCHING_TEST_FULLLOAD_START") == 0)
    return State::SWITCHING_TEST_FULLLOAD_START;
  if (std::strcmp(str, "SWITCHING_TEST_FULLLOAD_DONE") == 0)
    return State::SWITCHING_TEST_FULLLOAD_DONE;
  if (std::strcmp(str, "SWITCHING_TEST_CHECK") == 0)
    return State::SWITCHING_TEST_CHECK;
  if (std::strcmp(str, "SWITCHING_TEST_OK") == 0)
    return State::SWITCHING_TEST_OK;
  if (std::strcmp(str, "SWITCHING_TEST_FAILED") == 0)
    return State::SWITCHING_TEST_FAILED;
  if (std::strcmp(str, "READY_NEXT_TEST") == 0)
    return State::READY_NEXT_TEST;
  if (std::strcmp(str, "EFFICIENCY_TEST_START") == 0)
    return State::EFFICIENCY_TEST_START;
  if (std::strcmp(str, "EFFICIENCY_TEST_DONE") == 0)
    return State::EFFICIENCY_TEST_DONE;
  if (std::strcmp(str, "EFFICIENCY_TEST_CHECK") == 0)
    return State::EFFICIENCY_TEST_CHECK;
  if (std::strcmp(str, "EFFICIENCY_TEST_OK") == 0)
    return State::EFFICIENCY_TEST_OK;
  if (std::strcmp(str, "EFFICIENCY_TEST_FAILED") == 0)
    return State::EFFICIENCY_TEST_FAILED;
  if (std::strcmp(str, "BACKUP_TIME_TEST_START") == 0)
    return State::BACKUP_TIME_TEST_START;
  if (std::strcmp(str, "BACKUP_TIME_TEST_DONE") == 0)
    return State::BACKUP_TIME_TEST_DONE;
  if (std::strcmp(str, "BACKUP_TIME_TEST_CHECK") == 0)
    return State::BACKUP_TIME_TEST_CHECK;
  if (std::strcmp(str, "BACKUP_TIME_TEST_OK") == 0)
    return State::BACKUP_TIME_TEST_OK;
  if (std::strcmp(str, "BACKUP_TIME_TEST_FAILED") == 0)
    return State::BACKUP_TIME_TEST_FAILED;
  if (std::strcmp(str, "SAVE_TEST_DATA") == 0)
    return State::SAVE_TEST_DATA;
  if (std::strcmp(str, "ALL_TEST_DONE") == 0)
    return State::ALL_TEST_DONE;
  if (std::strcmp(str, "ADDENDUM_TEST_DATA") == 0)
    return State::ADDENDUM_TEST_DATA;
  if (std::strcmp(str, "REPORT_AVAILABLE") == 0)
    return State::REPORT_AVAILABLE;
  if (std::strcmp(str, "PRINT_TEST_DATA") == 0)
    return State::PRINT_TEST_DATA;
  if (std::strcmp(str, "FAULT") == 0)
    return State::FAULT;
  if (std::strcmp(str, "IDLE") == 0)
    return State::IDLE;
  return State::IDLE;  // Default or unknown state
}

// Convert string to Event enum
static Event stringToEvent(const char *str) {
  if (std::strcmp(str, "NONE") == 0)
    return Event::NONE;
  if (std::strcmp(str, "ERROR") == 0)
    return Event::ERROR;
  if (std::strcmp(str, "POWER_ON") == 0)
    return Event::POWER_ON;
  if (std::strcmp(str, "POWER_OFF") == 0)
    return Event::POWER_OFF;
  if (std::strcmp(str, "SELF_CHECK_OK") == 0)
    return Event::SELF_CHECK_OK;
  if (std::strcmp(str, "WIFI_CONNECTED") == 0)
    return Event::WIFI_CONNECTED;
  if (std::strcmp(str, "WIFI_DISCONNECTED") == 0)
    return Event::WIFI_DISCONNECTED;
  if (std::strcmp(str, "SETTING_LOADED") == 0)
    return Event::SETTING_LOADED;
  if (std::strcmp(str, "MANUAL_OVERRRIDE") == 0)
    return Event::MANUAL_OVERRRIDE;
  if (std::strcmp(str, "AUTO_TEST_CMD") == 0)
    return Event::AUTO_TEST_CMD;
  if (std::strcmp(str, "LOAD_BANK_ONLINE") == 0)
    return Event::LOAD_BANK_ONLINE;
  if (std::strcmp(str, "LOAD_ON_OFF_25P") == 0)
    return Event::LOAD_ON_OFF_25P;
  if (std::strcmp(str, "LOAD_ON_OFF_50P") == 0)
    return Event::LOAD_ON_OFF_50P;
  if (std::strcmp(str, "LOAD_ON_OFF_75P") == 0)
    return Event::LOAD_ON_OFF_75P;
  if (std::strcmp(str, "FULL_LOAD_ON_OFF") == 0)
    return Event::FULL_LOAD_ON_OFF;
  if (std::strcmp(str, "TIME_CAPTURE_OK") == 0)
    return Event::TIME_CAPTURE_OK;
  if (std::strcmp(str, "VALIDATE_TEST") == 0)
    return Event::VALIDATE_TEST;
  if (std::strcmp(str, "INPUT_OUTPUT_READY") == 0)
    return Event::INPUT_OUTPUT_READY;
  if (std::strcmp(str, "MESURED_DATA_RECEIVED") == 0)
    return Event::MESURED_DATA_RECEIVED;
  if (std::strcmp(str, "POWER_MEASURE_OK") == 0)
    return Event::POWER_MEASURE_OK;
  if (std::strcmp(str, "TIMER_READY") == 0)
    return Event::TIMER_READY;
  if (std::strcmp(str, "VALID_BACKUP_TIME") == 0)
    return Event::VALID_BACKUP_TIME;
  if (std::strcmp(str, "SAVE") == 0)
    return Event::SAVE;
  if (std::strcmp(str, "DATA") == 0)
    return Event::DATA;
  if (std::strcmp(str, "MANUAL_DATA_ENTRY") == 0)
    return Event::MANUAL_DATA_ENTRY;
  if (std::strcmp(str, "TRANSPORT_DATA") == 0)
    return Event::TRANSPORT_DATA;
  if (std::strcmp(str, "PRINT_DATA") == 0)
    return Event::PRINT_DATA;
  if (std::strcmp(str, "TEST_SUCCESS") == 0)
    return Event::TEST_SUCCESS;
  if (std::strcmp(str, "TEST_FAILED") == 0)
    return Event::TEST_FAILED;
  if (std::strcmp(str, "RETEST") == 0)
    return Event::RETEST;
  if (std::strcmp(str, "RETEST_OK") == 0)
    return Event::RETEST_OK;
  if (std::strcmp(str, "RETRY_CONNECT") == 0)
    return Event::RETRY_CONNECT;
  if (std::strcmp(str, "RETRY_OK") == 0)
    return Event::RETRY_OK;

  if (std::strcmp(str, "SYSTEM_FAULT") == 0)
    return Event::SYSTEM_FAULT;
  if (std::strcmp(str, "POWER_DEVICE_DOWN") == 0)
    return Event::POWER_DEVICE_DOWN;
  if (std::strcmp(str, "SWITCHING_DEVICE_DOWN") == 0)
    return Event::SWITCHING_DEVICE_DOWN;
  if (std::strcmp(str, "RESTART") == 0)
    return Event::RESTART;
  return Event::NONE;  // Default or unknown event
}

}  // namespace Node_Core
#endif