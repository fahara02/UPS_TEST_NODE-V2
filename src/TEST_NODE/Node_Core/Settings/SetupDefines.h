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
static const char* settingToString(SettingType setting)
{
	switch(setting)
	{
		// System Events
		case SettingType::ALL:
			return "all";
		case SettingType::SPEC:
			return "spec";
		case SettingType::TEST:
			return "test";
		case SettingType::TASK:
			return "task";
		case SettingType::TASK_PARAMS:
			return "task_params";
		case SettingType::REPORT:
			return "report";
		case SettingType::HARDWARE:
			return "hardware";
		case SettingType::MODBUS:
			return "modbus";
		case SettingType::NETWORK:
			return "network";
		default:
			return "NONE";
	}
}

} // namespace Node_Core

#endif