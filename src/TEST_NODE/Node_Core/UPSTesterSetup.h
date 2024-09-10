#ifndef UPS_TESTER_SETUP_H
#define UPS_TESTER_SETUP_H

#include "Settings.h"
#include <Arduino.h>
#include <IPAddress.h>
#include <functional>
#include <stdint.h>
#include "SettingsSubject.h"

const unsigned long ONE_DAY_MS = 86400000UL; // 1 day in milliseconds

namespace Node_Core
{

class UPSTesterSetup : public SettingsSubject
{
  public:
	static UPSTesterSetup& getInstance();

	SetupSpec& specSetup()
	{
		return _spec;
	};
	SetupTest& testSetup()
	{
		return _testSetting;
	};

	SetupTaskParams paramSetup()
	{
		return _taskParamsSetting;
	};
	SetupHardware hardwareSetup()
	{
		return _hardwareSetting;
	};
	SetupNetwork networkSetup()
	{
		return _networkSetting;
	};
	SetupModbus modbusSetup()
	{
		return _modbusSetting;
	};
	SetupReport reportSetup()
	{
		return _reportSetting;
	};
	SetupTuning tuningSetup()
	{
		return _TuningSetting;
	};

	template<typename T, typename U, typename V>
	bool setField(T& setup, const U Field, V Fieldvalue);

	void updateSettings(SettingType settingType, const void* newSetting);

	void loadSettings(SettingType settingType, const void* newSetting);
	void loadFactorySettings();

	void serializeSettings(const char* filename);
	void deserializeSettings(const char* filename);

	void notifySpecUpdated(const SetupSpec& newSpec, bool saveSetting)
	{
		_spec = newSpec;
		notifyObservers(SettingType::SPEC, &_spec);
	}

	void notifyTestUpdated(const SetupTest& newTest, bool saveSetting)
	{
		_testSetting = newTest;
		notifyObservers(SettingType::TEST, &_testSetting);
	}

  private:
	UPSTesterSetup();

	// static UPSTesterSetup* instance;
	bool _user_update_call = false;

	SetupSpec _spec;
	SetupTest _testSetting;

	SetupTaskParams _taskParamsSetting;
	SetupHardware _hardwareSetting;
	SetupTuning _TuningSetting;
	SetupNetwork _networkSetting;
	SetupModbus _modbusSetting;
	SetupReport _reportSetting;
	SetupUPSTest _allSetting;

	void notifyAllSettingsApplied();

	UPSTesterSetup(const UPSTesterSetup&) = delete;
	UPSTesterSetup& operator=(const UPSTesterSetup&) = delete;
};

} // namespace Node_Core

#endif
