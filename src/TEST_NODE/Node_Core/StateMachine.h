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
  static StateMachine *getInstance();
  static void deleteInstance();

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

  void handleEvent(Event event);
  State getCurrentState() const;
  void serializeTransitions(const char *filename);
  void deserializeTransitions(const char *filename);

  void setState(State new_state);

private:
  StateMachine();
  ~StateMachine();
  static StateMachine *instance;
  std::atomic<State> current_state{State::DEVICE_ON};
  std::atomic<int> retry_count{0};
  std::atomic<StateBits> state_bits{0};  // StateBits to manage state as bits
  const int max_retries = 3;
  const int max_retest = 2;

  StateMachine(const StateMachine &) = delete;
  StateMachine &operator=(const StateMachine &) = delete;

  // Define the transition table with 28 transitions
  const std::array<Transition, 39> transition_table
      = StateMachine::TransitionTable(

          // Mode Selection
          Row<State::DEVICE_READY, Event::MANUAL_OVERRIDE, State::MANUAL_MODE,
              Event::NONE>(),
          Row<State::DEVICE_READY, Event::AUTO_TEST_CMD, State::AUTO_MODE,
              Event::NONE>(),

          // Switching Test Sequence
          Row<State::AUTO_MODE, Event::INPUT_OUTPUT_READY,
              State::SWITCHING_TEST_START, Event::NONE>(),
          Row<State::SWITCHING_TEST_START, Event::TEST_DONE,
              State::SWITCHING_TEST_25P_DONE, Event::NONE>(),
          Row<State::SWITCHING_TEST_START, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::SWITCHING_TEST_25P_DONE, Event::DATA_CAPTURE_OK,
              State::SWITCHING_TEST_50P_START, Event::NONE>(),
          Row<State::SWITCHING_TEST_25P_DONE, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::SWITCHING_TEST_50P_START, Event::TEST_DONE,
              State::SWITCHING_TEST_50P_DONE, Event::NONE>(),
          Row<State::SWITCHING_TEST_50P_START, Event::TEST_FAILED,
              State::RETEST,
              Event::NONE>(),  // Added
          Row<State::SWITCHING_TEST_50P_DONE, Event::DATA_CAPTURE_OK,
              State::SWITCHING_TEST_75P_START, Event::NONE>(),
          Row<State::SWITCHING_TEST_50P_DONE, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::SWITCHING_TEST_75P_START, Event::TEST_DONE,
              State::SWITCHING_TEST_75P_DONE, Event::NONE>(),
          Row<State::SWITCHING_TEST_75P_START, Event::TEST_FAILED,
              State::RETEST,
              Event::NONE>(),  // Added
          Row<State::SWITCHING_TEST_75P_DONE, Event::DATA_CAPTURE_OK,
              State::SWITCHING_TEST_FULLLOAD_START, Event::NONE>(),
          Row<State::SWITCHING_TEST_75P_DONE, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::SWITCHING_TEST_FULLLOAD_START, Event::TEST_DONE,
              State::SWITCHING_TEST_FULLLOAD_DONE, Event::NONE>(),
          Row<State::SWITCHING_TEST_FULLLOAD_START, Event::TEST_FAILED,
              State::RETEST, Event::NONE>(),  // Added
          Row<State::SWITCHING_TEST_FULLLOAD_DONE, Event::DATA_CAPTURE_OK,
              State::SWITCHING_TEST_OK, Event::NONE>(),
          Row<State::SWITCHING_TEST_FULLLOAD_DONE, Event::TEST_FAILED,
              State::RETEST, Event::NONE>(),  // Added
          Row<State::SWITCHING_TEST_OK, Event::SAVE, State::READY_NEXT_TEST,
              Event::NONE>(),

          // Efficiency Test Sequence
          Row<State::READY_NEXT_TEST, Event::INPUT_OUTPUT_READY,
              State::EFFICIENCY_TEST_START, Event::NONE>(),
          Row<State::EFFICIENCY_TEST_START, Event::TEST_DONE,
              State::EFFICIENCY_TEST_DONE, Event::NONE>(),
          Row<State::EFFICIENCY_TEST_START, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::EFFICIENCY_TEST_DONE, Event::DATA_CAPTURE_OK,
              State::EFFICIENCY_TEST_OK, Event::NONE>(),
          Row<State::EFFICIENCY_TEST_DONE, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::EFFICIENCY_TEST_OK, Event::SAVE, State::READY_NEXT_TEST,
              Event::NONE>(),

          // Backup Time Test Sequence
          Row<State::READY_NEXT_TEST, Event::INPUT_OUTPUT_READY,
              State::BACKUP_TIME_TEST_START, Event::NONE>(),
          Row<State::BACKUP_TIME_TEST_START, Event::TEST_DONE,
              State::BACKUP_TIME_TEST_DONE, Event::NONE>(),
          Row<State::BACKUP_TIME_TEST_START, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::BACKUP_TIME_TEST_DONE, Event::DATA_CAPTURE_OK,
              State::BACKUP_TIME_TEST_OK, Event::NONE>(),
          Row<State::BACKUP_TIME_TEST_DONE, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::BACKUP_TIME_TEST_OK, Event::SAVE, State::ALL_TEST_DONE,
              Event::NONE>(),

          // Test Data Handling
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
