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

			break;

		case SettingType::TEST:
		{
			const SetupTest* newTest = static_cast<const SetupTest*>(newSetting);

			_testSetting = *newTest;
		}
		break;

		case SettingType::TASK_PARAMS:
			_taskParamsSetting = *static_cast<const SetupTaskParams*>(newSetting);

			break;

		case SettingType::HARDWARE:
			_hardwareSetting = *static_cast<const SetupHardware*>(newSetting);

			break;

		case SettingType::NETWORK:
			_networkSetting = *static_cast<const SetupNetwork*>(newSetting);

			break;

		case SettingType::MODBUS:
			_modbusSetting = *static_cast<const SetupModbus*>(newSetting);

			break;

		case SettingType::REPORT:
			_reportSetting = *static_cast<const SetupReport*>(newSetting);

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
		return true;
	}
	return false;
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

	doc["test"]["inputVoltage_volt"] = _testSetting.inputVoltage_volt;
	doc["test"]["switch_TestDuration"] = _testSetting.switch_testDuration_ms;
	doc["test"]["backup_TestDuration"] = _testSetting.backup_testDuration_ms;
	doc["test"]["min_valid_switch_time_ms"] = _testSetting.min_valid_switch_time_ms;
	doc["test"]["max_valid_switch_time_ms"] = _testSetting.max_valid_switch_time_ms;
	doc["test"]["ToleranceSwitchTime_ms"] = _testSetting.ToleranceSwitchTime_ms;
	doc["test"]["ToleranceBackUpTime_ms"] = _testSetting.ToleranceBackUpTime_ms;
	doc["test"]["MaxRetest"] = _testSetting.MaxRetest;

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

	_testSetting.inputVoltage_volt =
		doc["test"]["inputVoltage_volt"] | _testSetting.inputVoltage_volt;
	_testSetting.switch_testDuration_ms =
		doc["test"]["switch_TestDuration"] | _testSetting.switch_testDuration_ms;
	_testSetting.backup_testDuration_ms =
		doc["test"]["backup_TestDuration"] | _testSetting.backup_testDuration_ms;

	_testSetting.min_valid_switch_time_ms =
		doc["test"]["min_valid_switch_time_ms"] | _testSetting.min_valid_switch_time_ms;
	_testSetting.max_valid_switch_time_ms =
		doc["test"]["max_valid_switch_time_ms"] | _testSetting.max_valid_switch_time_ms;
	_testSetting.ToleranceSwitchTime_ms =
		doc["test"]["ToleranceSwitchTime_ms"] | _testSetting.ToleranceSwitchTime_ms;
	_testSetting.ToleranceBackUpTime_ms =
		doc["test"]["ToleranceBackUpTime_ms"] | _testSetting.ToleranceBackUpTime_ms;
	_testSetting.MaxRetest = doc["test"]["MaxRetest"] | _testSetting.MaxRetest;

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
