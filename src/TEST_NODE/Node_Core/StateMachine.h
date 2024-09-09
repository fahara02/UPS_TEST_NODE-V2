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
#include <nvs_flash.h>

namespace Node_Core
{
extern Preferences preferences;
using OnStateChangedCallback = std::function<void(bool stateChanged, State newState)>;
using OnModeChangedCallback = std::function<void(bool modeChanged, TestMode newMode)>;

class StateMachine
{
  public:
	friend class TestSync;
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
	void init();
	void handleEvent(Event event);
	void handleMode(TestMode mode);

	State getCurrentState() const;

	void registerStateChangeCallback(OnStateChangedCallback callback);
	void registerModeChangeCallback(OnStateChangedCallback callback);

	bool isAutoMode() const;
	bool isManualMode() const;

  private:
	StateMachine();

	std::atomic<State> _old_state{State::DEVICE_ON};
	std::atomic<State> _currentState{State::DEVICE_ON};
	std::atomic<TestMode> _deviceMode{TestMode::MANUAL};
	std::atomic<bool> _dataCapturedFlag{false};
	std::atomic<int> _retryCount{0};

	OnStateChangedCallback _stateChangeCallback;
	OnModeChangedCallback _modeChangeCallback;
	bool isValidState(uint32_t state);

	void setMode(TestMode new_mode);
	void setState(State new_state);
	void saveState(State new_state);

	void NotifyStateChanged(State state);
	void NotifyModeChanged(TestMode mode);
	void NotifyRejectTest();

	static void handleReport();
	static void handleError();

	static const std::array<Transition, 23> regular_transitions;
	static const std::array<Transition, 12> special_case_transitions;
	static const std::array<Transition, 35> transition_table;

	static const Transition* getRegularTransitions();
	static const Transition* getSpecialCaseTransitions();
	static const Transition* getTransitionTable();

	QueueHandle_t eventQueue;
	QueueHandle_t modeQueue;

	TaskHandle_t eventProcessortaskhandle = NULL;
	static void eventProcessorTask(void* params);
	void processEvent(Event event);
	StateMachine(const StateMachine&) = delete;
	StateMachine& operator=(const StateMachine&) = delete;
};

} // namespace Node_Core

#endif // STATE_MACHINE_H_
