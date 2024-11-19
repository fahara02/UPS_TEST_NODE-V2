#ifndef NODE_UTILITY_HPP
#define NODE_UTILITY_HPP
#include "PZEM_Measure.hpp"
#include"wsDefines.hpp"
#include"SetupDefines.h"
#include"StateDefines.h"
namespace Node_Utility
{
class ToString
{
  public:
	static const char* model(Node_Core::PZEMModel model);
	static const char* Phase(Node_Core::Phase phase);
	static const char* PZEMState(Node_Core::PZEMState state);
	static const char* wsPowerDataType(Node_Core::wsPowerDataType type);
	static const char* setting(Node_Core::SettingType setting);
	static const char* state(Node_Core::State state);
	static const char* event(Node_Core::Event event);
};
} // namespace Node_Utility
#endif