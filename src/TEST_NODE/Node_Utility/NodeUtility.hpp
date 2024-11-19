#ifndef NODE_UTILITY_HPP
#define NODE_UTILITY_HPP
#include "PZEM_Measure.hpp"
#include"wsDefines.hpp"
namespace Node_Utility
{
class ToString
{
  public:
	static const char* model(Node_Core::PZEMModel model);
	static const char* Phase(Node_Core::Phase phase);
	static const char* State(Node_Core::PZEMState state);
	static const char* wsPowerDataType(Node_Core::wsPowerDataType type);
	
};
} // namespace Node_Utility
#endif