#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "StateDefines.h"

// #include "freertos/event_groups.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>

namespace Node_Core {

class StateMachine {

public:
  EventGroupHandle_t _EgTestState = nullptr;
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
              []() {
                constexpr Event action_event = ActionEvent;

                instance->updateEventGroup(Start, false);
                instance->updateEventGroup(Next, true);
              },
              []() { return true; }};
    }
  };

  void handleEvent(Event event);
  State getCurrentState() const;
  void serializeTransitions(const char *filename);
  void deserializeTransitions(const char *filename);

  void setState(State new_state);

private:
  friend class TestManager;
  StateMachine();
  ~StateMachine();
  static StateMachine *instance;
  std::atomic<State> current_state{State::DEVICE_ON};
  std::atomic<int> retry_count{0};

  const int max_retries = 3;
  const int max_retest = 2;
  void updateEventGroup(State state, bool set_bits);
  // Define the transition table with 28 transitions
  const std::array<Transition, 20> transition_table
      = StateMachine::TransitionTable(

          // Mode Selection 5
          Row<State::DEVICE_ON, Event::SELF_CHECK_OK, State::DEVICE_OK,
              Event::NONE>(),
          Row<State::DEVICE_OK, Event::SETTING_LOADED, State::DEVICE_SETUP,
              Event::NONE>(),
          Row<State::DEVICE_SETUP, Event::LOAD_BANK_CHECKED,
              State::DEVICE_READY, Event::NONE>(),
          Row<State::DEVICE_READY, Event::MANUAL_OVERRIDE, State::MANUAL_MODE,
              Event::NONE>(),
          Row<State::DEVICE_READY, Event::AUTO_TEST_CMD, State::AUTO_MODE,
              Event::NONE>(),

          // Switching Test Sequence 8+5=13
          Row<State::AUTO_MODE, Event::INPUT_OUTPUT_READY, State::TEST_START,
              Event::NONE>(),
          Row<State::TEST_START, Event::DATA_CAPTURED, State::TEST_IN_PROGRESS,
              Event::NONE>(),
          Row<State::TEST_START, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::TEST_IN_PROGRESS, Event::VALID_DATA,
              State::CURRENT_TEST_OK, Event::NONE>(),
          Row<State::TEST_IN_PROGRESS, Event::TEST_FAILED, State::RETEST,
              Event::NONE>(),  // Added
          Row<State::CURRENT_TEST_OK, Event::SAVE, State::READY_NEXT_TEST,
              Event::NONE>(),
          Row<State::CURRENT_TEST_OK, Event::INPUT_OUTPUT_READY,
              State::READY_NEXT_TEST, Event::NONE>(),
          Row<State::CURRENT_TEST_OK, Event::TEST_LIST_EMPTY,
              State::ALL_TEST_DONE, Event::NONE>(),

          // Test Data Handling 13+4=17
          Row<State::ALL_TEST_DONE, Event::VALIDATE_TEST,
              State::REPORT_AVAILABLE, Event::NONE>(),
          Row<State::REPORT_AVAILABLE, Event::DATA, State::TRANSPORT_DATA,
              Event::NONE>(),
          Row<State::ALL_TEST_DONE, Event::MANUAL_DATA_ENTRY,
              State::ADDENDUM_TEST_DATA, Event::NONE>(),
          Row<State::ADDENDUM_TEST_DATA, Event::VALIDATE_TEST,
              State::REPORT_AVAILABLE, Event::NONE>(),

          // Fault Handling 17+3=20
          Row<State::DEVICE_READY, Event::SYSTEM_FAULT, State::FAULT,
              Event::NONE>(),
          Row<State::FAULT, Event::RETRY_OK, State::DEVICE_READY,
              Event::NONE>(),
          Row<State::FAULT, Event::RESTART, State::DEVICE_ON, Event::NONE>());

  StateMachine(const StateMachine &) = delete;
  StateMachine &operator=(const StateMachine &) = delete;
};

}  // namespace Node_Core

#endif  // STATE_MACHINE_H_
