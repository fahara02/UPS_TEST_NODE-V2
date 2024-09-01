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
	void setState(State new_state);
	void setMode(TestMode new_mode);
	static void handleReport();
	static void handleError();

	static std::array<Transition, 18> createRegularTransitions();
	static std::array<Transition, 11> createSpecialCaseTransitions();
	const std::array<Transition, 18> regular_transitions = createRegularTransitions();
	const std::array<Transition, 11> special_case_transitions = createSpecialCaseTransitions();
	static const std::array<Transition, 29> transition_table;

	StateMachine(const StateMachine&) = delete;
	StateMachine& operator=(const StateMachine&) = delete;
};

} // namespace Node_Core

#endif // STATE_MACHINE_H_
