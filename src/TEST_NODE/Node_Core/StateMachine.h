#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include <Preferences.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include "NodeConstants.h"
#include "StateDefines.h"
#include "Settings.h"
#include "sdkconfig.h"

namespace Node_Core
{
extern Preferences preferences;
using OnStateChangedCallback = std::function<void(bool stateChanged, State newState)>;

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

	template<State Start, Event EventTrigger, State Next, Event ActionEvent>
	struct Row
	{
		static Transition get_transition(
			ActionFunction action = nullptr, GuardFunction guard = [] {
				return true;
			})
		{
			return {Start, EventTrigger, Next,
					action ? action :
							 []() {
								 StateMachine& instance = StateMachine::getInstance();
								 instance.NotifyStateChanged(Next);
							 },
					guard};
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
	bool isValidState(uint32_t state);

	void setState(State new_state);
	void saveState(State new_state);
	void setMode(TestMode new_mode);
	void saveCurrentStateToNVS();

	static void handleReport();
	static void handleError();

	static const std::array<Transition, 20> regular_transitions;
	static const std::array<Transition, 12> special_case_transitions;
	static const std::array<Transition, 32> transition_table;

	static const Transition* getRegularTransitions();
	static const Transition* getSpecialCaseTransitions();
	static const Transition* getTransitionTable();

	StateMachine(const StateMachine&) = delete;
	StateMachine& operator=(const StateMachine&) = delete;
};

static State getDefaultStateFromConfig()
{
#if CONFIG_DEFAULT_STATE_DEVICE_ON
	return State::DEVICE_ON;
#elif CONFIG_DEFAULT_STATE_DEVICE_OK
	return State::DEVICE_OK;
#elif CONFIG_DEFAULT_STATE_DEVICE_SETUP
	return State::DEVICE_SETUP;
#elif CONFIG_DEFAULT_STATE_DEVICE_READY
	return State::DEVICE_READY;
#elif CONFIG_DEFAULT_STATE_READY_TO_PROCEED
	return State::READY_TO_PROCEED;
#elif CONFIG_DEFAULT_STATE_TEST_START
	return State::TEST_START;
#elif CONFIG_DEFAULT_STATE_TEST_RUNNING
	return State::TEST_RUNNING;
#elif CONFIG_DEFAULT_STATE_CURRENT_TEST_CHECK
	return State::CURRENT_TEST_CHECK;
#elif CONFIG_DEFAULT_STATE_CURRENT_TEST_OK
	return State::CURRENT_TEST_OK;
#elif CONFIG_DEFAULT_STATE_READY_NEXT_TEST
	return State::READY_NEXT_TEST;
#elif CONFIG_DEFAULT_STATE_MANUAL_NEXT_TEST
	return State::MANUAL_NEXT_TEST;
#elif CONFIG_DEFAULT_STATE_RETEST
	return State::RETEST;
#elif CONFIG_DEFAULT_STATE_SYSTEM_PAUSED
	return State::SYSTEM_PAUSED;
#elif CONFIG_DEFAULT_STATE_ALL_TEST_DONE
	return State::ALL_TEST_DONE;
#elif CONFIG_DEFAULT_STATE_START_FROM_SAVE
	return State::START_FROM_SAVE;
#elif CONFIG_DEFAULT_STATE_RECOVER_DATA
	return State::RECOVER_DATA;
#elif CONFIG_DEFAULT_STATE_ADDENDUM_TEST_DATA
	return State::ADDENDUM_TEST_DATA;
#elif CONFIG_DEFAULT_STATE_FAILED_TEST
	return State::FAILED_TEST;
#elif CONFIG_DEFAULT_STATE_TRANSPORT_DATA
	return State::TRANSPORT_DATA;
#elif CONFIG_DEFAULT_STATE_SYSTEM_TUNING
	return State::SYSTEM_TUNING;
#elif CONFIG_DEFAULT_STATE_FAULT
	return State::FAULT;
#elif CONFIG_DEFAULT_STATE_USER_CHECK_REQUIRED
	return State::USER_CHECK_REQUIRED;
#elif CONFIG_DEFAULT_STATE_WAITING_FOR_USER
	return State::WAITING_FOR_USER;
#else
	return State::DEVICE_ON; // Fallback state if nothing is set
#endif
}

} // namespace Node_Core

#endif // STATE_MACHINE_H_
