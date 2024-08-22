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
  EventGroupHandle_t TestState_EventGroup = nullptr;
  EventGroupHandle_t SystemEvents_EventGroup = nullptr;

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

                instance->updateStateEventGroup(Start, false);
                instance->updateStateEventGroup(Next, true);
              },
              []() { return true; }};
    }
  };
  void NotifySystemEventGroup(Event event, bool set_bits);

  void handleEvent(Event event);
  State getCurrentState() const;
  //   void serializeTransitions(const char *filename);
  //   void deserializeTransitions(const char *filename);

  void setState(State new_state);

private:
  friend class TestManager;
  StateMachine();
  ~StateMachine();
  static StateMachine *instance;
  std::atomic<State> _old_state{State::DEVICE_ON};
  std::atomic<State> current_state{State::DEVICE_ON};
  std::atomic<int> retry_count{0};

  const int max_retries = 3;
  const int max_retest = 2;
  void updateStateEventGroup(State state, bool set_bits);
  void handleEventbits(EventBits_t event_bits);
  void handleReport();
  void handleError();

  // Define the transition table with 28 transitions
  const std::array<Transition, 29> transition_table
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
          Row<State::MANUAL_MODE, Event::NETWORK_DISCONNECTED, State::FAULT,
              Event::NONE>(),
          Row<State::AUTO_MODE, Event::NETWORK_DISCONNECTED, State::DEVICE_OK,
              Event::NONE>(),
          Row<State::DEVICE_SETUP, Event::NETWORK_DISCONNECTED,
              State::DEVICE_OK, Event::NONE>(),

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
          Row<State::READY_NEXT_TEST, Event::INPUT_OUTPUT_READY,
              State::TEST_START, Event::NONE>(),
          Row<State::CURRENT_TEST_OK, Event::TEST_LIST_EMPTY,
              State::ALL_TEST_DONE, Event::NONE>(),
          Row<State::CURRENT_TEST_OK, Event::TEST_FAILED, State::RECOVER_DATA,
              Event::NONE>(),
          Row<State::RECOVER_DATA, Event::SAVE, State::START_FROM_SAVE,
              Event::NONE>(),

          // Test Data Handling 13+4=17
          Row<State::ALL_TEST_DONE, Event::JSON_READY, State::TRANSPORT_DATA,
              Event::NONE>(),
          Row<State::ALL_TEST_DONE, Event::TEST_FAILED, State::RECOVER_DATA,
              Event::NONE>(),
          Row<State::START_FROM_SAVE, Event::INPUT_OUTPUT_READY,
              State::TEST_START, Event::NONE>(),
          Row<State::ALL_TEST_DONE, Event::MANUAL_DATA_ENTRY,
              State::ADDENDUM_TEST_DATA, Event::NONE>(),
          Row<State::ADDENDUM_TEST_DATA, Event::JSON_READY,
              State::TRANSPORT_DATA, Event::NONE>(),
          Row<State::ADDENDUM_TEST_DATA, Event::TEST_FAILED,
              State::RECOVER_DATA, Event::NONE>(),

          Row<State::SYSTEM_TUNING, Event::AUTO_TEST_CMD, State::RECOVER_DATA,
              Event::NONE>(),
          Row<State::FAULT, Event::FAULT_CLEARED, State::RECOVER_DATA,
              Event::NONE>(),
          Row<State::SYSTEM_PAUSED, Event::AUTO_TEST_CMD,
              State::START_FROM_SAVE, Event::NONE>(),

          // Fault Handling 17+3=20
          Row<State::DEVICE_READY, Event::SYSTEM_FAULT, State::FAULT,
              Event::NONE>(),
          Row<State::FAULT, Event::RESTART, State::DEVICE_ON, Event::NONE>());

  StateMachine(const StateMachine &) = delete;
  StateMachine &operator=(const StateMachine &) = delete;
};

}  // namespace Node_Core

#endif  // STATE_MACHINE_H_
