#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "StateDefines.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>

namespace Node_Core {

class StateMachine {

public:
  using GuardFunction = std::function<bool()>;
  using ActionFunction = std::function<void()>;

  struct Transition {
    State current_state;
    Event event;
    State next_state;
    ActionFunction action;
    GuardFunction guard;
  };

  template <typename... Args>
  static std::array<Transition, sizeof...(Args)> TransitionTable(Args... args) {
    return {args.get_transition()...};
  }

  // Keeping the Row template with ActionEvent parameter
  template <State Start, Event EventTrigger, State Next, Event ActionEvent>
  struct Row {
    static Transition get_transition() {
      return {Start, EventTrigger, Next,
              []() { /* Action based on ActionEvent */ },
              []() { return true; }};
    }
  };

  StateMachine();
  ~StateMachine();

  void handleEvent(Event event);
  State getCurrentState() const;
  void serializeTransitions(const char *filename);
  void deserializeTransitions(const char *filename);

  void saveState(State state) {
    // Save to persistent storage
  }

  State loadState() {
    // Load from persistent storage
    return State::DEVICE_SHUTDOWN;
  }

  void setState(State new_state);

#ifdef UNIT_TEST
  void setMockAction(ActionFunction mock_action) {
    this->mock_action = mock_action;
  }

  void setMockGuard(GuardFunction mock_guard) { this->mock_guard = mock_guard; }
#endif

private:
  std::atomic<State> current_state{State::DEVICE_SHUTDOWN};
  std::atomic<int> retry_count{0};
  const int max_retries = 3;
  const int max_retest = 2;

  const std::array<Transition, 43> transition_table
      = StateMachine::TransitionTable(
          // Device Initialization and Setup
          Row<State::DEVICE_SHUTDOWN, Event::POWER_ON, State::DEVICE_ON,
              Event::NONE>(),
          Row<State::DEVICE_ON, Event::SELF_CHECK_OK, State::DEVICE_OK,
              Event::NONE>(),
          Row<State::DEVICE_OK, Event::WIFI_CONNECTED, State::DEVICE_CONNECTED,
              Event::NONE>(),
          Row<State::DEVICE_OK, Event::WIFI_DISCONNECTED,
              State::DEVICE_DISCONNECTED, Event::NONE>(),
          Row<State::DEVICE_DISCONNECTED, Event::RETRY_CONNECT,
              State::RECONNECT_NETWORK, Event::NONE>(),
          Row<State::RECONNECT_NETWORK, Event::WIFI_CONNECTED,
              State::DEVICE_CONNECTED, Event::NONE>(),
          Row<State::DEVICE_CONNECTED, Event::SETTING_LOADED,
              State::DEVICE_READY, Event::NONE>(),

          // Mode Selection
          Row<State::DEVICE_READY, Event::MANUAL_OVERRRIDE, State::MANUAL_MODE,
              Event::NONE>(),
          Row<State::DEVICE_READY, Event::AUTO_TEST_CMD, State::AUTO_MODE,
              Event::NONE>(),

          // Switching Test Sequence
          Row<State::AUTO_MODE, Event::LOAD_BANK_ONLINE,
              State::SWITCHING_TEST_START, Event::NONE>(),
          Row<State::SWITCHING_TEST_START, Event::TIMER_READY,
              State::SWITCHING_TEST_25P_START, Event::NONE>(),
          Row<State::SWITCHING_TEST_25P_START, Event::LOAD_ON_OFF_25P,
              State::SWITCHING_TEST_25P_DONE, Event::NONE>(),
          Row<State::SWITCHING_TEST_25P_DONE, Event::TIME_CAPTURE_OK,
              State::SWITCHING_TEST_50P_START, Event::NONE>(),
          Row<State::SWITCHING_TEST_50P_START, Event::LOAD_ON_OFF_50P,
              State::SWITCHING_TEST_50P_DONE, Event::NONE>(),
          Row<State::SWITCHING_TEST_50P_DONE, Event::TIME_CAPTURE_OK,
              State::SWITCHING_TEST_50P_DONE, Event::NONE>(),
          Row<State::SWITCHING_TEST_50P_DONE, Event::TIME_CAPTURE_OK,
              State::SWITCHING_TEST_75P_START, Event::NONE>(),
          Row<State::SWITCHING_TEST_75P_START, Event::LOAD_ON_OFF_75P,
              State::SWITCHING_TEST_75P_DONE, Event::NONE>(),
          Row<State::SWITCHING_TEST_75P_DONE, Event::TIME_CAPTURE_OK,
              State::SWITCHING_TEST_FULLLOAD_START, Event::NONE>(),
          Row<State::SWITCHING_TEST_FULLLOAD_START, Event::FULL_LOAD_ON_OFF,
              State::SWITCHING_TEST_FULLLOAD_DONE, Event::NONE>(),
          Row<State::SWITCHING_TEST_FULLLOAD_DONE, Event::TIME_CAPTURE_OK,
              State::SWITCHING_TEST_CHECK, Event::NONE>(),
          Row<State::SWITCHING_TEST_CHECK, Event::TEST_SUCCESS,
              State::SWITCHING_TEST_OK, Event::NONE>(),
          Row<State::SWITCHING_TEST_OK, Event::SAVE, State::SAVE_TEST_DATA,
              Event::NONE>(),

          Row<State::SWITCHING_TEST_CHECK, Event::TEST_FAILED,
              State::SWITCHING_TEST_START, Event::NONE>(),

          // Efficiency Test Sequence
          Row<State::READY_NEXT_TEST, Event::INPUT_OUTPUT_READY,
              State::EFFICIENCY_TEST_START, Event::NONE>(),
          Row<State::EFFICIENCY_TEST_START, Event::MESURED_DATA_RECEIVED,
              State::EFFICIENCY_TEST_DONE, Event::NONE>(),
          Row<State::EFFICIENCY_TEST_DONE, Event::POWER_MEASURE_OK,
              State::EFFICIENCY_TEST_CHECK, Event::NONE>(),
          Row<State::EFFICIENCY_TEST_CHECK, Event::TEST_SUCCESS,
              State::EFFICIENCY_TEST_OK, Event::NONE>(),
          Row<State::EFFICIENCY_TEST_OK, Event::SAVE, State::SAVE_TEST_DATA,
              Event::NONE>(),
          Row<State::EFFICIENCY_TEST_CHECK, Event::TEST_FAILED,
              State::EFFICIENCY_TEST_START, Event::NONE>(),

          // Backup Time Test Sequence
          Row<State::READY_NEXT_TEST, Event::TIMER_READY,
              State::BACKUP_TIME_TEST_START, Event::NONE>(),
          Row<State::BACKUP_TIME_TEST_START, Event::MESURED_DATA_RECEIVED,
              State::BACKUP_TIME_TEST_DONE, Event::NONE>(),
          Row<State::BACKUP_TIME_TEST_DONE, Event::VALID_BACKUP_TIME,
              State::BACKUP_TIME_TEST_CHECK, Event::NONE>(),
          Row<State::BACKUP_TIME_TEST_CHECK, Event::TEST_SUCCESS,
              State::BACKUP_TIME_TEST_OK, Event::NONE>(),
          Row<State::BACKUP_TIME_TEST_OK, Event::SAVE, State::ALL_TEST_DONE,
              Event::NONE>(),
          Row<State::BACKUP_TIME_TEST_CHECK, Event::TEST_FAILED,
              State::BACKUP_TIME_TEST_START, Event::NONE>(),

          // Test Data Handling
          Row<State::SAVE_TEST_DATA, Event::DATA, State::READY_NEXT_TEST,
              Event::NONE>(),
          Row<State::ALL_TEST_DONE, Event::TRANSPORT_DATA,
              State::REPORT_AVAILABLE, Event::NONE>(),
          Row<State::REPORT_AVAILABLE, Event::PRINT_DATA,
              State::PRINT_TEST_DATA, Event::NONE>(),
          Row<State::ALL_TEST_DONE, Event::MANUAL_DATA_ENTRY,
              State::ADDENDUM_TEST_DATA, Event::NONE>(),
          Row<State::ADDENDUM_TEST_DATA, Event::TRANSPORT_DATA,
              State::REPORT_AVAILABLE, Event::NONE>(),

          // Fault Handling
          Row<State::DEVICE_READY, Event::SYSTEM_FAULT, State::FAULT,
              Event::NONE>(),
          Row<State::FAULT, Event::RETRY_OK, State::DEVICE_READY,
              Event::NONE>(),
          Row<State::FAULT, Event::RESTART, State::DEVICE_ON, Event::NONE>());
};

}  // namespace Node_Core

#endif  // STATE_MACHINE_H_
