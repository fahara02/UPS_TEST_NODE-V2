#ifndef SETUP_DEFINES_H
#define SETUP_DEFINES_H
namespace Node_Core
{
enum class TestMode
{
	AUTO = 0,
	MANUAL = 1
};
enum class SettingType
{
	ALL,
	SPEC,
	TEST,
	TASK,
	TASK_PARAMS,
	HARDWARE,
	NETWORK,
	MODBUS,
	REPORT
};


} // namespace Node_Core

#endif