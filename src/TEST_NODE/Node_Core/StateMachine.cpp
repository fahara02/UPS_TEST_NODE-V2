#include "StateMachine.h"
#include "StateDefines.h"
#include <cstring>
#include <iostream>
namespace Node_Core {
StateMachine* StateMachine::instance = nullptr;

StateMachine::StateMachine()
    : current_state(State::DEVICE_ON), retry_count(0), max_retest(0) {}

StateMachine::~StateMachine() {}

StateMachine* StateMachine::getInstance() {
  if (instance == nullptr) {
    instance = new StateMachine();
  }
  return instance;
}

void StateMachine::deleteInstance() {
  delete instance;
  instance = nullptr;
}

void StateMachine::handleEvent(Event event) {

  // Handle special case for WIFI_DISCONNECTED
  if (event == Event::SELF_CHECK_OK) {

    if (current_state == State::DEVICE_ON) {
      if (retry_count < max_retries) {
        retry_count++;
        setState(State::DEVICE_READY);
      } else {
        setState(State::DEVICE_READY);
      }
      return;  // Exit early to prevent further processing
    }
  }

  // Manually search for the transition
  for (const auto& transition : transition_table) {
    if (transition.current_state == current_state
        && (transition.event == event || transition.event == Event::NONE)) {
      if (!transition.guard || transition.guard()) {
        setState(transition.next_state);
        if (transition.action) {
          transition.action();
        }
        // Reset retry_count after successful transition
        retry_count = 0;
      }
      return;  // Exit after handling the event
    }
  }
}

// Convert State enum to string

}  // namespace Node_Core
