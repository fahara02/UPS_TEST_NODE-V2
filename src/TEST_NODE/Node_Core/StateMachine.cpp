#include "StateMachine.h"
#include "Arduino.h"
#include "StateDefines.h"
#include <cstring>
#include <iostream>

namespace Node_Core {
StateMachine* StateMachine::instance = nullptr;

StateMachine::StateMachine()
    : current_state(State::DEVICE_ON), retry_count(0), max_retest(0) {

  _EgTestState = xEventGroupCreate();
}

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

void StateMachine::setState(State new_state) {
  current_state.store(new_state);
  // Add any additional logic required when the state changes
}

State StateMachine::getCurrentState() const { return current_state; }

void StateMachine::updateEventGroup(State state, bool set_bits) {
  EventBits_t bits = static_cast<EventBits_t>(state);
  Serial.print("updateEvent Triggered!");

  if (set_bits) {
    xEventGroupSetBits(_EgTestState, bits);
  } else {
    xEventGroupClearBits(_EgTestState, bits);
  }
}

void StateMachine::handleEvent(Event event) {

  // if (event == Event::SELF_CHECK_OK) {
  //   if (current_state == State::DEVICE_ON) {
  //     setState(State::DEVICE_READY);
  //     return;
  //   }
  // }

  // Manually search for the transition
  for (const auto& transition : transition_table) {
    if (transition.current_state == current_state
        && transition.event == event) {
      if (transition.guard()) {
        if (event == Event::WIFI_DISCONNECTED
            && current_state != State::DEVICE_READY) {
          retry_count = 0;
        }

        setState(transition.next_state);
        transition.action();
        return;
      }
    }
  }
}

// Convert State enum to string

}  // namespace Node_Core
