#ifndef UPS_TESTER_SETUP_H
#define UPS_TESTER_SETUP_H

#include "Settings.h"
#include <Arduino.h>
#include <IPAddress.h>
#include <functional>
#include <stdint.h>
const unsigned long ONE_DAY_MS = 86400000UL;  // 1 day in milliseconds

namespace Node_Core {
using OnTestModeCallback = std::function<void(TestMode mode)>;
using OnSetupSpecCallback
    = std::function<void(bool spec_updated, SetupSpec spec)>;
using OnSetupTestCallback
    = std::function<void(bool test_updated, SetupTest setting)>;
using OnSetupTaskCallback
    = std::function<void(bool task_updated, SetupTask setting)>;
using OnSetupTaskParamsCallback
    = std::function<void(bool taskParams_updated, SetupTaskParams setting)>;
using OnSetupHardwareCallback
    = std::function<void(bool hardware_updated, SetupHardware setting)>;
using OnSetupNetworkCallback
    = std::function<void(bool network_updated, SetupNetwork setting)>;
using OnSetupModbusCallback
    = std::function<void(bool modbus_updated, SetupModbus setting)>;
using OnSetupReportCallback
    = std::function<void(bool report_updated, SetupReport setting)>;
using OnAllSettingCallback
    = std::function<void(bool allsettings_updated, SetupUPSTest settings)>;

class UPSTesterSetup {
public:
  static UPSTesterSetup* getInstance();
  static void deleteInstance();
  SetupSpec specSetup() { return _spec; };
  SetupTest testSetup() { return _testSetting; };
  SetupTask taskSetup() { return _taskSetting; };
  SetupTaskParams paramSetup() { return _taskParamsSetting; };
  SetupHardware hardwareSetup() { return _hardwareSetting; };
  SetupNetwork networkSetup() { return _networkSetting; };
  SetupModbus modbusSetup() { return _modbusSetting; };
  SetupReport reportSetup() { return _reportSetting; };

  void updateSettings(SettingType settingType, const void* newSetting);
  void loadSettings(SettingType settingType, const void* newSetting);
  void loadFactorySettings();

  void serializeSettings(const char* filename);
  void deserializeSettings(const char* filename);

  void registerTestModeCallback(OnTestModeCallback callback) {
    _testModeCallback = callback;
  }

  void registerSpecCallback(OnSetupSpecCallback callback);
  void registerTestCallback(OnSetupTestCallback callback);
  void registerTaskCallback(OnSetupTaskCallback callback);
  void registerTaskParamsCallback(OnSetupTaskParamsCallback callback);
  void registerHardwareCallback(OnSetupHardwareCallback callback);
  void registerNetworkCallback(OnSetupNetworkCallback callback);
  void registerModbusCallback(OnSetupModbusCallback callback);
  void registerReportCallback(OnSetupReportCallback callback);
  void registerAllSettingCallback(OnAllSettingCallback callback);

private:
  UPSTesterSetup();
  ~UPSTesterSetup();
  static UPSTesterSetup* instance;
  bool _user_update_call = false;

  SetupSpec _spec;
  SetupTest _testSetting;
  SetupTask _taskSetting;
  SetupTaskParams _taskParamsSetting;
  SetupHardware _hardwareSetting;
  SetupTuning _TuningSetting;
  SetupNetwork _networkSetting;
  SetupModbus _modbusSetting;
  SetupReport _reportSetting;
  SetupUPSTest _allSetting;

  OnTestModeCallback _testModeCallback;
  OnSetupSpecCallback _specSetCallback;
  OnSetupTestCallback _testSetCallback;
  OnSetupTaskCallback _taskSetCallback;
  OnSetupTaskParamsCallback _taskParamsSetCallback;
  OnSetupHardwareCallback _hardwareSetCallback;
  OnSetupNetworkCallback _commSetCallback;
  OnSetupModbusCallback _modbusSetCallback;
  OnSetupReportCallback _reportSetCallback;
  OnAllSettingCallback _allSettingCallback;

  void notifyAllSettingsApplied();

  UPSTesterSetup(const UPSTesterSetup&) = delete;
  UPSTesterSetup& operator=(const UPSTesterSetup&) = delete;
};

}  // namespace Node_Core

#endif
