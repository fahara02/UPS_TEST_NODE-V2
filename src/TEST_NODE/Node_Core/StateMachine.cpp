#include "StateMachine.h"
#include "Arduino.h"
#include "Logger.h"
#include "StateDefines.h"
#include <cstring>
#include <iostream>

using namespace Node_Core;
extern Logger& logger;
namespace Node_Core {
StateMachine* StateMachine::instance = nullptr;

StateMachine::StateMachine()
    : _old_state(State::DEVICE_ON),
      current_state(State::DEVICE_ON),
      retry_count(0),
      max_retest(0) {

  TestState_EventGroup = xEventGroupCreate();
  SystemEvents_EventGroup = xEventGroupCreate();
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

void StateMachine::updateStateEventGroup(State state, bool set_bits) {
  EventBits_t bits = static_cast<EventBits_t>(state);
  logger.log(LogLevel::INFO, "New update State Event");

  if (set_bits) {
    xEventGroupSetBits(TestState_EventGroup, bits);
  } else {
    xEventGroupClearBits(TestState_EventGroup, bits);
  }
}
void StateMachine::NotifySystemEventGroup(Event event, bool set_bits) {
  EventBits_t bits_event = static_cast<EventBits_t>(event);
  logger.log(LogLevel::INFO, "New System Event received, Processing");
  if (set_bits) {
    xEventGroupSetBits(SystemEvents_EventGroup, bits_event);
  } else {
    xEventGroupClearBits(SystemEvents_EventGroup, bits_event);
  }
}

void StateMachine::handleEventbits(EventBits_t event_bits) {
  Event event = static_cast<Event>(event_bits);
  instance->handleEvent(event);
}

void StateMachine::handleEvent(Event event) {

  if (event == Event::SYSTEM_FAULT) {
    State old_state = instance->getCurrentState();
    _old_state.store(old_state);
    State new_state = State::FAULT;
    setState(new_state);
    updateStateEventGroup(current_state, true);  // set the current statebit
    updateStateEventGroup(old_state, false);  // clear the old state  state bit
    return;
  }
  if (event == Event::USER_PAUSED) {
    State old_state = instance->getCurrentState();
    _old_state.store(old_state);
    State new_state = State::SYSTEM_PAUSED;
    setState(new_state);
    updateStateEventGroup(new_state, true);   // set the current statebit
    updateStateEventGroup(old_state, false);  // clear the old state  state bit
    return;
  }

  if (event == Event::USER_TUNE) {
    State old_state = instance->getCurrentState();
    _old_state.store(old_state);
    State new_state = State::SYSTEM_TUNING;
    setState(new_state);
    updateStateEventGroup(new_state, true);   // set the current statebit
    updateStateEventGroup(old_state, false);  // clear the old state  state bit
    return;
  }

  if (event == Event::ERROR) {
    handleError();
    return;
  }
  if (event == Event::REPORT_SEND) {
    handleReport();
    return;
  }
  // Manually search for the transition
  for (const auto& transition : transition_table) {
    if (transition.current_state == current_state
        && transition.event == event) {
      if (transition.guard()) {
        State old_state = current_state;
        _old_state.store(old_state);
        setState(transition.next_state);
        logger.log(LogLevel::INFO, "state changed");
        transition.action();
        return;
      }
    }
  }
}

void StateMachine::handleError() {
  updateStateEventGroup(State::LOG_ERROR, true);
}

void StateMachine::handleReport() {
  updateStateEventGroup(State::REPORT_AVAILABLE, true);
}
// Convert State enum to string

}  // namespace Node_Core
