#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "StateDefines.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>

namespace Node_Core
{

class StateMachine
{
  public:
	static StateMachine& getInstance();

	std::atomic<bool> dataCapturedFlag{false};

	using GuardFunction = std::function<bool()>;
	using ActionFunction = std::function<void()>;

	struct Transition
	{
		State current_state;
		Event event;
		State next_state;
		ActionFunction action;
		GuardFunction guard;
	};

	template<typename... Args>
	static std::array<Transition, sizeof...(Args)> TransitionTable(Args... args)
	{
		return {args.get_transition()...};
	}

	// Keeping the Row template with ActionEvent parameter
	template<State Start, Event EventTrigger, State Next, Event ActionEvent>
	struct Row
	{
		static Transition get_transition()
		{
			return {Start, EventTrigger, Next,
					[]() {
						StateMachine& instance = StateMachine::getInstance();
						instance.updateStateEventGroup(Start, false);
						instance.updateStateEventGroup(Next, true);
					},
					[]() {
						return true;
					}};
		}
	};

	void NotifySystemEventGroup(Event event, bool set_bits);
	void handleEvent(Event event);
	State getCurrentState() const;
	void setState(State new_state);
	EventGroupHandle_t getEventGroupSystemState() const
	{
		return systemStateEventGroup;
	}
	EventGroupHandle_t getEventGroupSystemEvent() const
	{
		return systemEventsEventGroup;
	}

  private:
	friend class TestSync;
	StateMachine();

	std::atomic<State> _old_state{State::DEVICE_ON};
	std::atomic<State> current_state{State::DEVICE_ON};
	std::atomic<int> retry_count{0};

	const int max_retries = 3;
	const int max_retest = 2;

	EventGroupHandle_t systemStateEventGroup = nullptr;
	EventGroupHandle_t systemEventsEventGroup = nullptr;

	void updateStateEventGroup(State state, bool set_bits);
	void handleEventbits(EventBits_t event_bits);
	void handleReport();
	void handleError();

	// Define the regular transitions using the Row template
	const std::array<Transition, 20> regular_transitions = StateMachine::TransitionTable(
		Row<State::DEVICE_ON, Event::SELF_CHECK_OK, State::DEVICE_OK, Event::NONE>(),
		Row<State::DEVICE_OK, Event::SETTING_LOADED, State::DEVICE_SETUP, Event::NONE>(),
		Row<State::DEVICE_SETUP, Event::LOAD_BANK_CHECKED, State::DEVICE_READY, Event::NONE>(),
		Row<State::DEVICE_READY, Event::MANUAL_OVERRIDE, State::MANUAL_MODE, Event::NONE>(),
		Row<State::DEVICE_READY, Event::AUTO_TEST_CMD, State::AUTO_MODE, Event::NONE>(),
		Row<State::MANUAL_MODE, Event::NETWORK_DISCONNECTED, State::FAULT, Event::NONE>(),
		Row<State::AUTO_MODE, Event::NETWORK_DISCONNECTED, State::FAULT, Event::NONE>(),
		Row<State::DEVICE_SETUP, Event::NETWORK_DISCONNECTED, State::FAULT, Event::NONE>(),
		Row<State::AUTO_MODE, Event::PENDING_TEST_FOUND, State::TEST_START, Event::NONE>(),
		Row<State::TEST_START, Event::TEST_ONGOING, State::TEST_IN_PROGRESS, Event::NONE>(),
		Row<State::TEST_START, Event::TEST_FAILED, State::RETEST, Event::NONE>(),
		Row<State::CURRENT_TEST_CHECK, Event::VALID_DATA, State::CURRENT_TEST_OK, Event::NONE>(),
		Row<State::TEST_IN_PROGRESS, Event::TEST_FAILED, State::RETEST, Event::NONE>(),
		Row<State::CURRENT_TEST_CHECK, Event::TEST_FAILED, State::RETEST, Event::NONE>(),
		Row<State::CURRENT_TEST_OK, Event::SAVE, State::READY_NEXT_TEST, Event::NONE>(),
		Row<State::READY_NEXT_TEST, Event::PENDING_TEST_FOUND, State::TEST_START, Event::NONE>(),
		Row<State::READY_NEXT_TEST, Event::TEST_LIST_EMPTY, State::ALL_TEST_DONE, Event::NONE>(),
		Row<State::CURRENT_TEST_OK, Event::TEST_FAILED, State::RECOVER_DATA, Event::NONE>(),
		Row<State::RECOVER_DATA, Event::SAVE, State::START_FROM_SAVE, Event::NONE>(),
		Row<State::ALL_TEST_DONE, Event::JSON_READY, State::TRANSPORT_DATA, Event::NONE>());

	// Define the special case transitions separately
	const std::array<Transition, 12> special_case_transitions = {
		{{State::TEST_IN_PROGRESS, Event::TEST_TIME_END, State::CURRENT_TEST_CHECK,
		  [this]() {
			  dataCapturedFlag.store(false); // Reset the flag
			  updateStateEventGroup(State::TEST_IN_PROGRESS, false);
			  updateStateEventGroup(State::CURRENT_TEST_CHECK, true);
		  },
		  []() {
			  return true;
		  }},
		 {State::TEST_IN_PROGRESS, Event::DATA_CAPTURED, State::CURRENT_TEST_CHECK,
		  [this]() {
			  dataCapturedFlag.store(true); // Set the flag
			  updateStateEventGroup(State::TEST_IN_PROGRESS, false);
			  updateStateEventGroup(State::CURRENT_TEST_CHECK, true);
		  },
		  []() {
			  return true;
		  }},

		 {State::CURRENT_TEST_CHECK, Event::TEST_TIME_END, State::CURRENT_TEST_CHECK,
		  []() {
			  // No state change, but handle the event
		  },
		  []() {
			  return true;
		  }},

		 {State::ALL_TEST_DONE, Event::TEST_FAILED, State::RECOVER_DATA,
		  [this]() {
			  handleError();
		  },
		  []() {
			  return true;
		  }},
		 {State::ADDENDUM_TEST_DATA, Event::JSON_READY, State::TRANSPORT_DATA,
		  [this]() {
			  handleReport();
		  },
		  []() {
			  return true;
		  }},
		 {State::ADDENDUM_TEST_DATA, Event::TEST_FAILED, State::RECOVER_DATA,
		  [this]() {
			  handleError();
		  },
		  []() {
			  return true;
		  }},
		 {State::SYSTEM_TUNING, Event::AUTO_TEST_CMD, State::RECOVER_DATA,
		  [this]() {
			  handleReport();
		  },
		  []() {
			  return true;
		  }},
		 {State::FAULT, Event::FAULT_CLEARED, State::RECOVER_DATA,
		  [this]() {
			  handleReport();
		  },
		  []() {
			  return true;
		  }},
		 {State::SYSTEM_PAUSED, Event::AUTO_TEST_CMD, State::START_FROM_SAVE,
		  [this]() {
			  handleReport();
		  },
		  []() {
			  return true;
		  }},
		 {State::START_FROM_SAVE, Event::PENDING_TEST_FOUND, State::TEST_START,
		  []() {
			  // No state change, but handle the event
		  },
		  []() {
			  return true;
		  }},
		 {State::FAULT, Event::RESTART, State::DEVICE_ON,
		  [this]() {
			  handleError();
		  },
		  []() {
			  return true;
		  }},
		 {State::FAULT, Event::RESTART, State::DEVICE_ON,
		  [this]() {
			  handleReport();
		  },
		  []() {
			  return true;
		  }}}};

	// Combine both arrays into the final transition table
	const std::array<Transition, 32> transition_table = [this] {
		std::array<Transition, 32> result = {}; // Initialize an empty array of 32 transitions

		// Copy the regular transitions
		std::copy(regular_transitions.begin(), regular_transitions.end(), result.begin());

		// Copy the special case transitions
		std::copy(special_case_transitions.begin(), special_case_transitions.end(),
				  result.begin() + regular_transitions.size());

		return result;
	}();

	StateMachine(const StateMachine&) = delete;
	StateMachine& operator=(const StateMachine&) = delete;
};

} // namespace Node_Core

#endif // STATE_MACHINE_H_
