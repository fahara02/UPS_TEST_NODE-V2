#include "UPSTesterSetup.h"
#include "FS.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

namespace Node_Core
{

UPSTesterSetup::UPSTesterSetup()
{
	// // Initialize LittleFS
	// if (!LittleFS.begin()) {
	//     Serial.println("Failed to mount LittleFS");
	// }

	// // Load settings from LittleFS
	// loadSettings();
}

UPSTesterSetup& UPSTesterSetup::getInstance()
{
	static UPSTesterSetup instance;

	return instance;
}

void UPSTesterSetup::updateSettings(SettingType settingType, const void* newSetting)
{
	bool allsettings_updated = false;

	switch(settingType)
	{
		case SettingType::SPEC:
			_spec = *static_cast<const SetupSpec*>(newSetting);

			if(_specSetCallback)
			{
				_specSetCallback(true, _spec);
			}
			break;

		case SettingType::TEST:
		{
			const SetupTest* newTest = static_cast<const SetupTest*>(newSetting);
			bool modeChanged = (_testSetting.mode != newTest->mode);
			_testSetting = *newTest;

			if(_testSetCallback)
			{
				_testSetCallback(true, _testSetting);
			}
			if(modeChanged && _testModeCallback)
			{
				_testModeCallback(_testSetting.mode);
			}
		}
		break;

		case SettingType::TASK:
			_taskSetting = *static_cast<const SetupTask*>(newSetting);

			if(_taskSetCallback)
			{
				_taskSetCallback(true, _taskSetting);
			}
			break;

		case SettingType::TASK_PARAMS:
			_taskParamsSetting = *static_cast<const SetupTaskParams*>(newSetting);

			if(_taskParamsSetCallback)
			{
				_taskParamsSetCallback(true, _taskParamsSetting);
			}
			break;

		case SettingType::HARDWARE:
			_hardwareSetting = *static_cast<const SetupHardware*>(newSetting);

			if(_hardwareSetCallback)
			{
				_hardwareSetCallback(true, _hardwareSetting);
			}
			break;

		case SettingType::NETWORK:
			_networkSetting = *static_cast<const SetupNetwork*>(newSetting);

			if(_commSetCallback)
			{
				_commSetCallback(true, _networkSetting);
			}
			break;

		case SettingType::MODBUS:
			_modbusSetting = *static_cast<const SetupModbus*>(newSetting);

			if(_modbusSetCallback)
			{
				_modbusSetCallback(true, _modbusSetting);
			}
			break;

		case SettingType::REPORT:
			_reportSetting = *static_cast<const SetupReport*>(newSetting);

			if(_reportSetCallback)
			{
				_reportSetCallback(true, _reportSetting);
			}
			break;

		case SettingType::ALL:
			_allSetting = *static_cast<const SetupUPSTest*>(newSetting);

			allsettings_updated = true; // Assume all settings are updated directly
			break;

		default:
			Serial.println("Unknown setting type");
			return;
	}

	// Determine if all settings have been updated recently
	unsigned long currentTime = millis();
	int updatedSettingsCount = 0;

	if(currentTime - _spec.lastUpdate() <= ONE_DAY_MS)
		updatedSettingsCount++;
	if(currentTime - _testSetting.lastUpdate() <= ONE_DAY_MS)
		updatedSettingsCount++;
	if(currentTime - _taskSetting.lastsetting_updated <= ONE_DAY_MS)
		updatedSettingsCount++;
	if(currentTime - _taskParamsSetting.lastsetting_updated <= ONE_DAY_MS)
		updatedSettingsCount++;
	if(currentTime - _hardwareSetting.lastsetting_updated <= ONE_DAY_MS)
		updatedSettingsCount++;
	if(currentTime - _networkSetting.lastsetting_updated <= ONE_DAY_MS)
		updatedSettingsCount++;
	if(currentTime - _modbusSetting.lastsetting_updated <= ONE_DAY_MS)
		updatedSettingsCount++;
	if(currentTime - _reportSetting.lastUpdate() <= ONE_DAY_MS)
		updatedSettingsCount++;

	// If most settings are updated, consider all settings updated
	if(updatedSettingsCount >= 6)
	{
		allsettings_updated = true;
		_allSetting.lastsetting_updated = currentTime;
		updatedSettingsCount = 0;
	}
	if(_user_update_call)
	{
		allsettings_updated = true;
		_allSetting.lastsetting_updated = currentTime;
		_user_update_call = false;
	}
	// Trigger the all settings callback if available
	if(_allSettingCallback)
	{
		_allSettingCallback(allsettings_updated, _allSetting);
	}

	serializeSettings("/tester_settings.json");
}

void UPSTesterSetup::loadSettings(SettingType settingType, const void* newSetting)
{
	deserializeSettings("/tester_settings.json");
}

template<typename T, typename U, typename V>
bool UPSTesterSetup::setField(T& setup, const U Field, V Fieldvalue)
{
	if(setup.setField(Field, Fieldvalue))
	{
		SettingType settingType = setup.typeofSetting;
		switch(settingType)
		{
			case SettingType::ALL:
				if(_allSettingCallback)
				{
					_allSettingCallback(true, setup);
				}
				break;

			case SettingType::SPEC:
				if(_specSetCallback)
				{
					_specSetCallback(true, setup.spec);
				}
				break;

			case SettingType::TEST:
				if(_testSetCallback)
				{
					_testSetCallback(true, setup.testSetting);
				}
				break;

			case SettingType::TASK:
				if(_taskSetCallback)
				{
					_taskSetCallback(true, setup.taskSetting);
				}
				break;

			case SettingType::TASK_PARAMS:
				if(_taskParamsSetCallback)
				{
					_taskParamsSetCallback(true, setup.paramsSetting);
				}
				break;

			case SettingType::HARDWARE:
				if(_hardwareSetCallback)
				{
					_hardwareSetCallback(true, setup.hardwareSetting);
				}
				break;

			case SettingType::NETWORK:
				if(_commSetCallback)
				{
					_commSetCallback(true, setup.commSetting);
				}
				break;

			case SettingType::MODBUS:
				if(_modbusSetCallback)
				{
					_modbusSetCallback(true, setup.modbusSetting);
				}
				break;

			case SettingType::REPORT:
				if(_reportSetCallback)
				{
					_reportSetCallback(true, setup.reportSetting);
				}
				break;

			default:
				return false;
		}

		return true;
	}
	return false;
}

template<typename T, typename U>
bool UPSTesterSetup::updateField(T& currentField, const T& newField,
								 std::function<void(bool, U)> callback, U setup)
{
	if(currentField != newField)
	{
		currentField = newField;
		if(callback)
		{
			callback(true, setup);
		}
		return true; // Field was updated
	}
	return false; // No update occurred
}

void UPSTesterSetup::notifySpecUpdated(const SetupSpec newSpec, bool SaveSetting)
{
	bool updated = false;

	// Update individual fields using the updated template function
	updated |= updateField(_spec.Rating_va, newSpec.Rating_va, _specSetCallback, _spec);
	updated |=
		updateField(_spec.RatedVoltage_volt, newSpec.RatedVoltage_volt, _specSetCallback, _spec);
	updated |=
		updateField(_spec.RatedCurrent_amp, newSpec.RatedCurrent_amp, _specSetCallback, _spec);
	updated |= updateField(_spec.MinInputVoltage_volt, newSpec.MinInputVoltage_volt,
						   _specSetCallback, _spec);
	updated |= updateField(_spec.MaxInputVoltage_volt, newSpec.MaxInputVoltage_volt,
						   _specSetCallback, _spec);
	updated |=
		updateField(_spec.AvgSwitchTime_ms, newSpec.AvgSwitchTime_ms, _specSetCallback, _spec);
	updated |=
		updateField(_spec.AvgBackupTime_ms, newSpec.AvgBackupTime_ms, _specSetCallback, _spec);

	if(updated)
	{
		//_spec.lastUpdate()= millis();
		if(SaveSetting)
		{
			serializeSettings("/tester_settings.json");
		}
	}
}

void UPSTesterSetup::serializeSettings(const char* filename)
{
	DynamicJsonDocument doc(1024);

	// Fill JSON with current settings
	doc["spec"]["Rating_va"] = _spec.Rating_va;
	doc["spec"]["RatedVoltage_volt"] = _spec.RatedVoltage_volt;
	doc["spec"]["RatedCurrent_amp"] = _spec.RatedCurrent_amp;
	doc["spec"]["MinInputVoltage_volt"] = _spec.MinInputVoltage_volt;
	doc["spec"]["MaxInputVoltage_volt"] = _spec.MaxInputVoltage_volt;
	doc["spec"]["AvgSwitchTime_ms"] = _spec.AvgSwitchTime_ms;
	doc["spec"]["AvgBackupTime_ms"] = _spec.AvgBackupTime_ms;

	doc["test"]["TestStandard"] = _testSetting.TestStandard;
	doc["test"]["mode"] = static_cast<int>(_testSetting.mode);
	doc["test"]["inputVoltage_volt"] = _testSetting.inputVoltage_volt;
	doc["test"]["min_valid_switch_time_ms"] = _testSetting.min_valid_switch_time_ms;
	doc["test"]["max_valid_switch_time_ms"] = _testSetting.max_valid_switch_time_ms;
	doc["test"]["ToleranceSwitchTime_ms"] = _testSetting.ToleranceSwitchTime_ms;
	doc["test"]["ToleranceBackUpTime_ms"] = _testSetting.ToleranceBackUpTime_ms;
	doc["test"]["MaxRetest"] = _testSetting.MaxRetest;

	doc["task"]["mainTest_taskCore"] = _taskSetting.mainTest_taskCore;
	doc["task"]["mainsISR_taskCore"] = _taskSetting.mainsISR_taskCore;
	doc["task"]["upsISR_taskCore"] = _taskSetting.upsISR_taskCore;
	doc["task"]["mainTest_taskIdlePriority"] = _taskSetting.mainTest_taskIdlePriority;
	doc["task"]["mainsISR_taskIdlePriority"] = _taskSetting.mainsISR_taskIdlePriority;
	doc["task"]["upsISR_taskIdlePriority"] = _taskSetting.upsISR_taskIdlePriority;
	doc["task"]["mainTest_taskStack"] = _taskSetting.mainTest_taskStack;
	doc["task"]["mainsISR_taskStack"] = _taskSetting.mainsISR_taskStack;
	doc["task"]["upsISR_taskStack"] = _taskSetting.upsISR_taskStack;
	doc["task"]["lastsetting_updated"] = _taskSetting.lastsetting_updated;

	doc["task_params"]["flag_mains_power_loss"] = _taskParamsSetting.flag_mains_power_loss;
	doc["task_params"]["flag_ups_power_gain"] = _taskParamsSetting.flag_ups_power_gain;
	doc["task_params"]["task_TestVARating"] = _taskParamsSetting.task_TestVARating;
	doc["task_params"]["testDuration_ms"] = _taskParamsSetting.task_testDuration_ms;
	doc["task_params"]["lastsetting_updated"] = _taskParamsSetting.lastsetting_updated;

	doc["hardware"]["pwmchannelNo"] = _hardwareSetting.pwmchannelNo;
	doc["hardware"]["pwmResolusion_bits"] = _hardwareSetting.pwmResolusion_bits;
	doc["hardware"]["pwmduty_set"] = _hardwareSetting.pwmduty_set;
	doc["hardware"]["lastsetting_updated"] = _hardwareSetting.lastsetting_updated;

	doc["TuningSetting"]["adjust_pwm_25P"] = _TuningSetting.adjust_pwm_25P;
	doc["TuningSetting"]["adjust_pwm_50P"] = _TuningSetting.adjust_pwm_50P;
	doc["TuningSetting"]["adjust_pwm_75P"] = _TuningSetting.adjust_pwm_75P;
	doc["TuningSetting"]["adjust_pwm_100P"] = _TuningSetting.adjust_pwm_100P;

	doc["network"]["AP_SSID"] = _networkSetting.AP_SSID;
	doc["network"]["AP_PASS"] = _networkSetting.AP_PASS;
	doc["network"]["STA_SSID"] = _networkSetting.STA_SSID;
	doc["network"]["STA_PASS"] = _networkSetting.STA_PASS;
	doc["network"]["STA_IP"] = _networkSetting.STA_IP.toString();
	doc["network"]["STA_GW"] = _networkSetting.STA_GW.toString();
	doc["network"]["STA_SN"] = _networkSetting.STA_SN.toString();
	doc["network"]["DHCP"] = _networkSetting.DHCP;
	doc["network"]["max_retry"] = _networkSetting.max_retry;
	doc["network"]["reconnectTimeout_ms"] = _networkSetting.reconnectTimeout_ms;
	doc["network"]["networkTimeout_ms"] = _networkSetting.networkTimeout_ms;
	doc["network"]["refreshConnectionAfter_ms"] = _networkSetting.refreshConnectionAfter_ms;
	doc["network"]["lastsetting_updated"] = _networkSetting.lastsetting_updated;

	doc["modbus"]["baudrate"] = _modbusSetting.baudrate;
	doc["modbus"]["parity"] = _modbusSetting.parity;
	doc["modbus"]["databits"] = _modbusSetting.databits;
	doc["modbus"]["stopbits"] = _modbusSetting.stopbits;
	doc["modbus"]["slaveID"] = _modbusSetting.slaveID;
	doc["modbus"]["lastsetting_updated"] = _modbusSetting.lastsetting_updated;

	doc["report"]["enableReport"] = _reportSetting.enableReport;
	doc["report"]["ReportFormat"] = _reportSetting.reportFormat;
	doc["report"]["clientName"] = _reportSetting.clientName;
	doc["report"]["brandName"] = _reportSetting.brandName;
	doc["report"]["serialNumber"] = _reportSetting.serialNumber;
	doc["report"]["sampleNumber"] = _reportSetting.sampleNumber;
	doc["report"]["lastsetting_updated"] = _reportSetting.lastUpdate();

	// Open file for writing
	File file = LittleFS.open(filename, "w");
	if(!file)
	{
		Serial.println("Failed to open file for writing");
		return;
	}

	// Serialize JSON to file
	serializeJson(doc, file);
	file.close();
}

void UPSTesterSetup::deserializeSettings(const char* filename)
{
	// Open file for reading
	File file = LittleFS.open(filename, "r");
	if(!file)
	{
		Serial.println("Failed to open file for reading");
		return;
	}

	// Allocate a JSON document with the appropriate capacity
	DynamicJsonDocument doc(1024);

	// Deserialize the JSON from the file
	DeserializationError error = deserializeJson(doc, file);
	if(error)
	{
		Serial.println("Failed to parse settings file");
		file.close();
		return;
	}
	file.close();

	// Read the JSON into the internal settings structures, retaining existing
	// values as fallback
	_spec.Rating_va = doc["spec"]["Rating_va"] | _spec.Rating_va;
	_spec.RatedVoltage_volt = doc["spec"]["RatedVoltage_volt"] | _spec.RatedVoltage_volt;
	_spec.RatedCurrent_amp = doc["spec"]["RatedCurrent_amp"] | _spec.RatedCurrent_amp;
	_spec.MinInputVoltage_volt = doc["spec"]["MinInputVoltage_volt"] | _spec.MinInputVoltage_volt;
	_spec.MaxInputVoltage_volt = doc["spec"]["MaxInputVoltage_volt"] | _spec.MaxInputVoltage_volt;
	_spec.AvgSwitchTime_ms = doc["spec"]["AvgSwitchTime_ms"] | _spec.AvgSwitchTime_ms;
	_spec.AvgBackupTime_ms = doc["spec"]["AvgBackupTime_ms"] | _spec.AvgBackupTime_ms;

	_testSetting.TestStandard = doc["test"]["TestStandard"] | _testSetting.TestStandard;
	_testSetting.mode =
		static_cast<TestMode>(doc["test"]["mode"] | static_cast<int>(_testSetting.mode));
	_testSetting.inputVoltage_volt =
		doc["test"]["inputVoltage_volt"] | _testSetting.inputVoltage_volt;
	_testSetting.min_valid_switch_time_ms =
		doc["test"]["min_valid_switch_time_ms"] | _testSetting.min_valid_switch_time_ms;
	_testSetting.max_valid_switch_time_ms =
		doc["test"]["max_valid_switch_time_ms"] | _testSetting.max_valid_switch_time_ms;
	_testSetting.ToleranceSwitchTime_ms =
		doc["test"]["ToleranceSwitchTime_ms"] | _testSetting.ToleranceSwitchTime_ms;
	_testSetting.ToleranceBackUpTime_ms =
		doc["test"]["ToleranceBackUpTime_ms"] | _testSetting.ToleranceBackUpTime_ms;
	_testSetting.MaxRetest = doc["test"]["MaxRetest"] | _testSetting.MaxRetest;

	_taskSetting.mainTest_taskCore =
		doc["task"]["mainTest_taskCore"] | _taskSetting.mainTest_taskCore;
	_taskSetting.mainsISR_taskCore =
		doc["task"]["mainsISR_taskCore"] | _taskSetting.mainsISR_taskCore;
	_taskSetting.upsISR_taskCore = doc["task"]["upsISR_taskCore"] | _taskSetting.upsISR_taskCore;
	_taskSetting.mainTest_taskIdlePriority =
		doc["task"]["mainTest_taskIdlePriority"] | _taskSetting.mainTest_taskIdlePriority;
	_taskSetting.mainsISR_taskIdlePriority =
		doc["task"]["mainsISR_taskIdlePriority"] | _taskSetting.mainsISR_taskIdlePriority;
	_taskSetting.upsISR_taskIdlePriority =
		doc["task"]["upsISR_taskIdlePriority"] | _taskSetting.upsISR_taskIdlePriority;
	_taskSetting.mainTest_taskStack =
		doc["task"]["mainTest_taskStack"] | _taskSetting.mainTest_taskStack;
	_taskSetting.mainsISR_taskStack =
		doc["task"]["mainsISR_taskStack"] | _taskSetting.mainsISR_taskStack;
	_taskSetting.upsISR_taskStack = doc["task"]["upsISR_taskStack"] | _taskSetting.upsISR_taskStack;
	_taskSetting.lastsetting_updated =
		doc["task"]["lastsetting_updated"] | _taskSetting.lastsetting_updated;

	_taskParamsSetting.flag_mains_power_loss =
		doc["task_params"]["flag_mains_power_loss"] | _taskParamsSetting.flag_mains_power_loss;
	_taskParamsSetting.flag_ups_power_gain =
		doc["task_params"]["flag_ups_power_gain"] | _taskParamsSetting.flag_ups_power_gain;
	_taskParamsSetting.task_TestVARating =
		doc["task_params"]["task_TestVARating"] | _taskParamsSetting.task_TestVARating;
	_taskParamsSetting.task_testDuration_ms =
		doc["task_params"]["task_testDuration_ms"] | _taskParamsSetting.task_testDuration_ms;
	_taskParamsSetting.lastsetting_updated =
		doc["task_params"]["lastsetting_updated"] | _taskParamsSetting.lastsetting_updated;

	_hardwareSetting.pwmchannelNo = doc["hardware"]["pwmchannelNo"] | _hardwareSetting.pwmchannelNo;
	_hardwareSetting.pwmResolusion_bits =
		doc["hardware"]["pwmResolusion_bits"] | _hardwareSetting.pwmResolusion_bits;
	_hardwareSetting.pwmduty_set = doc["hardware"]["pwmduty_set"] | _hardwareSetting.pwmduty_set;

	_hardwareSetting.lastsetting_updated =
		doc["hardware"]["lastsetting_updated"] | _hardwareSetting.lastsetting_updated;

	_TuningSetting.adjust_pwm_25P =
		doc["TuningSetting"]["adjust_pwm_25P"] | _TuningSetting.adjust_pwm_25P;
	_TuningSetting.adjust_pwm_50P =
		doc["TuningSetting"]["adjust_pwm_50P"] | _TuningSetting.adjust_pwm_50P;
	_TuningSetting.adjust_pwm_75P =
		doc["TuningSetting"]["adjust_pwm_75P"] | _TuningSetting.adjust_pwm_75P;
	_TuningSetting.adjust_pwm_100P =
		doc["TuningSetting"]["adjust_pwm_100P"] | _TuningSetting.adjust_pwm_100P;

	_networkSetting.AP_SSID = doc["network"]["AP_SSID"] | _networkSetting.AP_SSID;
	_networkSetting.AP_PASS = doc["network"]["AP_PASS"] | _networkSetting.AP_PASS;
	_networkSetting.STA_SSID = doc["network"]["STA_SSID"] | _networkSetting.STA_SSID;
	_networkSetting.STA_PASS = doc["network"]["STA_PASS"] | _networkSetting.STA_PASS;
	_networkSetting.STA_IP.fromString(doc["network"]["STA_IP"] | _networkSetting.STA_IP.toString());
	_networkSetting.STA_GW.fromString(doc["network"]["STA_GW"] | _networkSetting.STA_GW.toString());
	_networkSetting.STA_SN.fromString(doc["network"]["STA_SN"] | _networkSetting.STA_SN.toString());
	_networkSetting.DHCP = doc["network"]["DHCP"] | _networkSetting.DHCP;
	_networkSetting.max_retry = doc["network"]["max_retry"] | _networkSetting.max_retry;
	_networkSetting.reconnectTimeout_ms =
		doc["network"]["reconnectTimeout_ms"] | _networkSetting.reconnectTimeout_ms;
	_networkSetting.networkTimeout_ms =
		doc["network"]["networkTimeout_ms"] | _networkSetting.networkTimeout_ms;
	_networkSetting.refreshConnectionAfter_ms =
		doc["network"]["refreshConnectionAfter_ms"] | _networkSetting.refreshConnectionAfter_ms;
	_networkSetting.lastsetting_updated =
		doc["network"]["lastsetting_updated"] | _networkSetting.lastsetting_updated;

	_modbusSetting.baudrate = doc["modbus"]["baudrate"] | _modbusSetting.baudrate;
	_modbusSetting.parity = doc["modbus"]["parity"] | _modbusSetting.parity;
	_modbusSetting.databits = doc["modbus"]["databits"] | _modbusSetting.databits;
	_modbusSetting.stopbits = doc["modbus"]["stopbits"] | _modbusSetting.stopbits;
	_modbusSetting.slaveID = doc["modbus"]["slaveID"] | _modbusSetting.slaveID;
	_modbusSetting.lastsetting_updated =
		doc["modbus"]["lastsetting_updated"] | _modbusSetting.lastsetting_updated;

	_reportSetting.enableReport = doc["report"]["enableReport"] | _reportSetting.enableReport;
	_reportSetting.reportFormat = doc["report"]["ReportFormat"] | _reportSetting.reportFormat;
	_reportSetting.clientName = doc["report"]["clientName"] | _reportSetting.clientName;
	_reportSetting.brandName = doc["report"]["brandName"] | _reportSetting.brandName;
	_reportSetting.serialNumber = doc["report"]["serialNumber"] | _reportSetting.serialNumber;
	_reportSetting.sampleNumber = doc["report"]["sampleNumber"] | _reportSetting.sampleNumber;
}

void UPSTesterSetup::notifyAllSettingsApplied()
{
	if(_allSettingCallback)
	{
		_allSettingCallback(true,
							{_spec, _testSetting, _taskSetting, _taskParamsSetting,
							 _hardwareSetting, _networkSetting, _modbusSetting, _reportSetting});
	}
}

// Register Callback Methods
void UPSTesterSetup::registerSpecCallback(OnSetupSpecCallback callback)
{
	_specSetCallback = callback;
}

void UPSTesterSetup::registerTestCallback(OnSetupTestCallback callback)
{
	_testSetCallback = callback;
}
void UPSTesterSetup::registerTaskCallback(OnSetupTaskCallback callback)
{
	_taskSetCallback = callback;
}
void UPSTesterSetup::registerTaskParamsCallback(OnSetupTaskParamsCallback callback)
{
	_taskParamsSetCallback = callback;
}
void UPSTesterSetup::registerNetworkCallback(OnSetupNetworkCallback callback)
{
	_commSetCallback = callback;
}
void UPSTesterSetup::registerModbusCallback(OnSetupModbusCallback callback)
{
	_modbusSetCallback = callback;
}
void UPSTesterSetup::registerHardwareCallback(OnSetupHardwareCallback callback)
{
	_hardwareSetCallback = callback;
}

void UPSTesterSetup::registerReportCallback(OnSetupReportCallback callback)
{
	_reportSetCallback = callback;
}

// void UPSTesterSetup::loadFactorySettings() {
//   // SetupSpec factory settings
//   SetupSpec factorySpec = {
//       1500,      // Rating_va (uint16_t)
//       230,       // RatedVoltage_volt (uint16_t)
//       6,         // RatedCurrent_amp (uint16_t)
//       180,       // MinInputVoltage_volt (uint16_t)
//       260,       // MaxInputVoltage_volt (uint16_t)
//       50UL,      // AvgSwitchTime_ms (unsigned long)
//       300000UL,  // AvgBackupTime_ms (unsigned long)
//       0UL        // lastsetting_updated (unsigned long)
//   };
//   updateSettings(SettingType::SPEC, &factorySpec);

//   // SetupTest factory settings
//   SetupTest factoryTest = {
//       "IEC 62040-3",   // TestStandard (const char*)
//       TestMode::AUTO,  // mode (TestMode enum)
//       1500,            // testVARating (uint16_t)
//       230,             // inputVoltage_volt (uint16_t)
//       3600000UL,       // testDuration_ms (unsigned long)
//       1UL,             // min_valid_switch_time_ms (unsigned long)
//       3000UL,          // max_valid_switch_time_ms (unsigned long)
//       50UL,            // ToleranceSwitchTime_ms (unsigned long)
//       28800000UL,      // maxBackupTime_ms (unsigned long)
//       300000UL,        // ToleranceBackUpTime_ms (unsigned long)
//       3,               // MaxRetest (uint8_t)
//       0UL              // lastsetting_updated (unsigned long)
//   };
//   updateSettings(SettingType::TEST, &factoryTest);

//   // SetupTask factory settings
//   SetupTask factoryTask = {
//       0,                     // mainTest_taskCore (BaseType_t)
//       ARDUINO_RUNNING_CORE,  // mainsISR_taskCore (BaseType_t)
//       ARDUINO_RUNNING_CORE,  // upsISR_taskCore (BaseType_t)
//       2,                     // mainTest_taskIdlePriority (UBaseType_t)
//       1,                     // mainsISR_taskIdlePriority (UBaseType_t)
//       1,                     // upsISR_taskIdlePriority (UBaseType_t)
//       12000,                 // mainTest_taskStack (uint32_t)
//       12000,                 // mainsISR_taskStack (uint32_t)
//       12000,                 // upsISR_taskStack (uint32_t)
//       0UL                    // lastsetting_updated (unsigned long)
//   };
//   updateSettings(SettingType::TASK, &factoryTask);

//   // SetupTaskParams factory settings
//   SetupTaskParams factoryTaskParams = {
//       false,      // flag_mains_power_loss (bool)
//       false,      // flag_ups_power_gain (bool)
//       false,      // flag_ups_power_loss (bool)
//       1500,       // task_TestVARating (uint16_t)
//       3600000UL,  // task_testDuration_ms (unsigned long)
//       0UL         // lastsetting_updated (unsigned long)
//   };
//   updateSettings(SettingType::TASK_PARAMS, &factoryTaskParams);

//   // SetupHardware factory settings
//   SetupHardware factoryHardware = {
//       0,       // pwmchannelNo (uint8_t)
//       8,       // pwmResolusion_bits (uint8_t)
//       0,       // pwmduty_set (uint16_t)
//       25,      // adjust_pwm_25P (uint16_t)
//       50,      // adjust_pwm_50P (uint16_t)
//       75,      // adjust_pwm_75P (uint16_t)
//       100,     // adjust_pwm_100P (uint16_t)
//       3000UL,  // pwm_frequecy (uint32_t)
//       0UL      // lastsetting_updated (unsigned long)
//   };
//   updateSettings(SettingType::HARDWARE, &factoryHardware);

//   // SetupNetwork factory settings
//   SetupNetwork factoryNetwork = {
//       "UPS_Test_AP",                // AP_SSID (const char*)
//       "password",                   // AP_PASS (const char*)
//       "UPS_Test_STA",               // STA_SSID (const char*)
//       "password",                   // STA_PASS (const char*)
//       IPAddress(192, 168, 1, 100),  // STA_IP (IPAddress)
//       IPAddress(192, 168, 1, 1),    // STA_GW (IPAddress)
//       IPAddress(255, 255, 255, 0),  // STA_SN (IPAddress)
//       true,                         // DHCP (bool)
//       3,                            // max_retry (int)
//       5000UL,                       // reconnectTimeout_ms (uint32_t)
//       10000UL,                      // networkTimeout_ms (uint32_t)
//       86400000UL,                   // refreshConnectionAfter_ms (uint32_t)
//       0UL                           // lastsetting_updated (unsigned long)
//   };
//   updateSettings(SettingType::NETWORK, &factoryNetwork);

//   // SetupModbus factory settings
//   SetupModbus factoryModbus = {
//       1,       // slaveID (uint8_t)
//       8,       // databits (uint8_t)
//       1,       // stopbits (uint8_t)
//       0,       // parity (uint8_t)
//       9600UL,  // baudrate (unsigned long)
//       0UL      // lastsetting_updated (unsigned long)
//   };
//   updateSettings(SettingType::MODBUS, &factoryModbus);

//   // SetupReport factory settings
//   SetupReport factoryReport = {
//       true,          // enableReport (bool)
//       "JSON",        // ReportFormat (const char*)
//       "ClientName",  // clientName (const char*)
//       "BrandName",   // brandName (const char*)
//       "SN12345",     // serialNumber (const char*)
//       1,             // sampleNumber (int)
//       0UL            // lastsetting_updated (unsigned long)
//   };
//   updateSettings(SettingType::REPORT, &factoryReport);

//   // Notify that all settings have been applied
//   notifyAllSettingsApplied();
// }

} // namespace Node_Core
