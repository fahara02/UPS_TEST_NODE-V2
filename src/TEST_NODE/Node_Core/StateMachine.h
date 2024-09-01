#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "StateDefines.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include "NodeConstants.h"
#include "Settings.h"

namespace Node_Core
{
using OnStateChangedCallback = std::function<void(bool statedChanged, State newState)>;
class StateMachine
{
  public:
	static StateMachine& getInstance();

	

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
template<State Start, Event EventTrigger, State Next, Event ActionEvent, typename Guard = std::function<bool()>>
struct Row
{
    static Transition get_transition()
    {
        return {Start, EventTrigger, Next,
                []() {
                    StateMachine& instance = StateMachine::getInstance();
                    instance.NotifyStateChanged(Next);
                },
                Guard()};
    }
};



	void NotifyStateChanged(State state);
	void handleEvent(Event event);
	State getCurrentState() const;
	
    void registerStateChangeCallback(OnStateChangedCallback callback);

  private:
	friend class TestSync;
	StateMachine();
    

	
	std::atomic<State> _old_state{State::DEVICE_ON};
	std::atomic<State> current_state{State::DEVICE_ON};
	std::atomic<TestMode> _deviceMode{TestMode::MANUAL};
	std::atomic<bool> _dataCapturedFlag{false};
	std::atomic<int> _retryCount{0};

	OnStateChangedCallback _stateChangeCallback;

	bool isAutoMode() const;
	bool isManualMode() const;
	void setState(State new_state);
	void setMode(TestMode new_mode);
	void handleReport();
	void handleError();
    
	// Define the regular transitions using the Row template
	const std::array<Transition, 16> regular_transitions = StateMachine::TransitionTable(
		//Intial Startup
		Row<State::DEVICE_ON, Event::SELF_CHECK_OK, State::DEVICE_OK, Event::NONE>(),
		Row<State::DEVICE_OK, Event::SETTING_LOADED, State::DEVICE_SETUP, Event::NONE>(),
		Row<State::DEVICE_SETUP, Event::LOAD_BANK_CHECKED, State::DEVICE_READY, Event::NONE>(),
		//After Test Added and User Proceed Command(AUOT OR MANUAL MODE SAME FOR THESE)
        Row<State::DEVICE_READY, Event::NEW_TEST, State::READY_TO_PROCEED, Event::NONE>(),
		Row<State::READY_TO_PROCEED, Event::START, State::TEST_START, Event::NONE>(),
	    Row<State::TEST_START, Event::TEST_RUN_OK, State::TEST_RUNNING, Event::NONE>(),
	    Row<State::TEST_START, Event::TEST_FAILED, State::USER_CHECK_REQUIRED, Event::NONE>(),		
		Row<State::TEST_RUNNING, Event::TEST_FAILED, State::RETEST, Event::NONE>(),//retest logic in Test Sync
		Row<State::USER_CHECK_REQUIRED, Event::START, State::TEST_START, Event::NONE>(),
		
       
		Row<State::CURRENT_TEST_CHECK, Event::VALID_DATA, State::CURRENT_TEST_OK, Event::NONE>(),
		
		Row<State::CURRENT_TEST_CHECK, Event::TEST_FAILED, State::RETEST, Event::NONE>(),
		Row<State::CURRENT_TEST_OK, Event::SAVE, State::READY_NEXT_TEST, Event::NONE>(),

		
		Row<State::READY_NEXT_TEST, Event::TEST_LIST_EMPTY, State::ALL_TEST_DONE, Event::NONE>(),
		Row<State::CURRENT_TEST_OK, Event::TEST_FAILED, State::RECOVER_DATA, Event::NONE>(),
		Row<State::RECOVER_DATA, Event::SAVE, State::START_FROM_SAVE, Event::NONE>(),
		Row<State::ALL_TEST_DONE, Event::JSON_READY, State::TRANSPORT_DATA, Event::NONE>());

	// Define the special case transitions separately
	const std::array<Transition, 11> special_case_transitions = {
		{{State::TEST_RUNNING, Event::TEST_TIME_END, State::CURRENT_TEST_CHECK,
		  [this]() {
			  _dataCapturedFlag.store(false); // Reset the flag
			 
		  },
		  []() {
			  return true;
		  }},
		 {State::TEST_RUNNING, Event::DATA_CAPTURED, State::CURRENT_TEST_CHECK,
		  [this]() {
			  _dataCapturedFlag.store(true); // Set the flag
			
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
		 {State::SYSTEM_TUNING, Event::AUTO, State::RECOVER_DATA,
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
		 {State::SYSTEM_PAUSED, Event::AUTO, State::START_FROM_SAVE,
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
