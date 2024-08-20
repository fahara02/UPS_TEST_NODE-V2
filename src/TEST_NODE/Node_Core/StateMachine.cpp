#include "StateMachine.h"
#include "StateDefines.h"
#include <cstring>
#include <iostream>
namespace Node_Core {

StateMachine::StateMachine()
    : current_state(State::DEVICE_SHUTDOWN), retry_count(0), max_retest(0) {}

StateMachine::~StateMachine() {}

void StateMachine::handleEvent(Event event) {

  std::cout << "Handling event: " << eventToString(event)
            << " from state: " << stateToString(current_state) << std::endl;

  // Handle special case for WIFI_DISCONNECTED
  if (event == Event::RETRY_CONNECT) {
    if (current_state != State::DEVICE_CONNECTED) {
      if (retry_count < max_retries) {
        retry_count++;
        setState(State::RECONNECT_NETWORK);
      } else {
        setState(State::NETWORK_TIMEOUT);
      }
      return;  // Exit early to prevent further processing
    }
  }

  // Handle re-test events based on the new transition table
  if (event == Event::TEST_FAILED) {
    switch (current_state) {
      case State::SWITCHING_TEST_CHECK:
        if (retry_count < max_retest) {
          retry_count++;
          setState(State::SWITCHING_TEST_START);  // Restart the switching test
        } else {
          setState(State::SWITCHING_TEST_FAILED);  // Max retries exceeded
        }
        return;

      case State::EFFICIENCY_TEST_CHECK:
        if (retry_count < max_retest) {
          retry_count++;
          setState(State::EFFICIENCY_TEST_START);  // Restart the efficiency
                                                   // test
        } else {
          setState(State::EFFICIENCY_TEST_FAILED);  // Max retries exceeded
        }
        return;

      case State::BACKUP_TIME_TEST_CHECK:
        if (retry_count < max_retest) {
          retry_count++;
          setState(State::BACKUP_TIME_TEST_START);  // Restart the backup time
                                                    // test
        } else {
          setState(State::BACKUP_TIME_TEST_FAILED);  // Max retries exceeded
        }
        return;

      default:
        // No specific handling for other states
        break;
    }
  }

  // Manually search for the transition
  for (const auto &transition : transition_table) {
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
