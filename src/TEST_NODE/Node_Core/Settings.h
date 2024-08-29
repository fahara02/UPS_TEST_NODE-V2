#ifndef SETTINGS_H
#define SETTINGS_H
#include "Arduino.h"
#include <IPAddress.h>
#include <stdint.h>
#include <ctime>
#include <cstring>
#include "NodeConstants.h"

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

struct SetupSpec
{
    uint16_t Rating_va = 2000;
    uint16_t RatedVoltage_volt = 230;
    uint16_t RatedCurrent_amp = 6;
    uint16_t MinInputVoltage_volt = 180;
    uint16_t MaxInputVoltage_volt = 260;
    unsigned long AvgSwitchTime_ms = 50UL;
    unsigned long AvgBackupTime_ms = 300000UL;

    enum class Field
    {
        RatingVa,
        RatedVoltage,
        RatedCurrent,
        MinInputVoltage,
        MaxInputVoltage,
        AvgSwitchTime,
        AvgBackupTime
    };

    bool setField(Field field, uint32_t value) {
        switch(field) {
            case Field::RatingVa:
			  if ( value >=UPS_MIN_VA && value <=UPS_MAX_VA){
				Rating_va = static_cast<uint16_t>(value);
				return true;
			  }
			  return false;
			  break;
			case Field::RatedVoltage:
			if ( value >=UPS_MIN_INPUT_VOLT && value <=UPS_MAX_INPUT_VOLT){
				 RatedVoltage_volt = static_cast<uint16_t>(value);
				return true;
			  }
			   return false;
               
                break;
            case Field::RatedCurrent:
			if ( value >=UPS_MIN_OUTPUT_mAMP&& value <=UPS_MIN_OUTPUT_mAMP){
				  RatedCurrent_amp  = static_cast<uint16_t>(value)/1000;
				return true;
			  }
                return false;
                break;
            case Field::MinInputVoltage:
			if ( value >=UPS_MIN_INPUT_VOLT && value < UPS_MAX_INPUT_VOLT){
				 MinInputVoltage_volt = static_cast<uint16_t>(value);
				return true;
			  }
                 return false;
                break;
            case Field::MaxInputVoltage:
			if ( value > UPS_MIN_INPUT_VOLT && value <= UPS_MAX_INPUT_VOLT){
				  MaxInputVoltage_volt = static_cast<uint16_t>(value);
				return true;
			  }
                return false;
                break;
            case Field::AvgSwitchTime:
			if ( value <= UPS_MAX_SWITCHING_TIME_MS ){
				  AvgSwitchTime_ms = value;
				return true;
			  }
                 return false;
                break;
            case Field::AvgBackupTime:
			if ( value >= UPS_MIN_BACKUP_TIME_MS ){
				 AvgBackupTime_ms = value;
				return true;
			  }
                return false;
                break;
        }
        lastsetting_updated = millis(); // Update the timestamp to the current time.
		lastupdate_time= std::time(nullptr); 
    }

    unsigned lastUpdate() const {
        return lastsetting_updated;
    }
	 const char* lastUpdateTime() const {
        std::time_t t = lastsetting_updated;
        std::strftime(last_update_str, sizeof(last_update_str), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        return last_update_str;
    }

private:
    unsigned long lastsetting_updated = 0UL;
	unsigned long lastupdate_time= 0UL;
	 mutable char last_update_str[20]; 
};

enum class TestField
{
	TestStandard,
	Mode,
	TestVARating,
	InputVoltage,
	TestDuration,
	MinValidSwitchTime,
	MaxValidSwitchTime,
	MinValidBackupTime,
	MaxValidBackupTime,
	ToleranceSwitchTime,
	MaxBackupTime,
	ToleranceBackupTime,
	MaxRetest,
	LastSettingUpdated
};

struct SetupTest
{
	const char* TestStandard = "IEC 62040-3";
	TestMode mode = TestMode::AUTO;
	uint16_t testVARating = 3000;
	uint16_t inputVoltage_volt = 220;
	unsigned long testDuration_ms = 600000UL;
	unsigned long min_valid_switch_time_ms = 1UL;
	unsigned long max_valid_switch_time_ms = 3000UL;
	unsigned long min_valid_backup_time_ms = 1UL;
	unsigned long max_valid_backup_time_ms = 300000UL;
	unsigned long ToleranceSwitchTime_ms = 50UL;
	unsigned long maxBackupTime_min = 300UL;
	unsigned long ToleranceBackUpTime_ms = 300000UL;
	int MaxRetest = 3;
	unsigned long lastsetting_updated = 0UL;
};
enum class TaskField
{
	MainTestTaskCore,
	MainsISRTaskCore,
	UpsISRTaskCore,
	MainTestTaskIdlePriority,
	MainsISRTaskIdlePriority,
	UpsISRTaskIdlePriority,
	MainTestTaskStack,
	MainsISRTaskStack,
	UpsISRTaskStack,
	LastSettingUpdated
};

struct SetupTask
{
	int mainTest_taskCore = 0;
	int mainsISR_taskCore = ARDUINO_RUNNING_CORE;
	int upsISR_taskCore = ARDUINO_RUNNING_CORE;
	int mainTest_taskIdlePriority = 2;
	int mainsISR_taskIdlePriority = 1;
	int upsISR_taskIdlePriority = 1;
	uint32_t mainTest_taskStack = 12000;
	uint32_t mainsISR_taskStack = 4096;
	uint32_t upsISR_taskStack = 4096;
	unsigned long lastsetting_updated = 0UL;
};
enum class TaskParamsField
{
	FlagMainsPowerLoss,
	FlagUpsPowerGain,
	FlagUpsPowerLoss,
	TestNo,
	TaskTestVARating,
	TaskTestDuration,
	LastSettingUpdated
};

struct SetupTaskParams
{
	bool flag_mains_power_loss = false;
	bool flag_ups_power_gain = false;
	bool flag_ups_power_loss = false;
	uint16_t test_No = 0;
	uint16_t task_TestVARating = 1000;
	unsigned long task_testDuration_ms = 10000UL;
	unsigned long lastsetting_updated = 0UL;
};
enum class HardwareField
{
	PwmChannelNo,
	PwmResolutionBits,
	PwmDutySet,
	PwmFrequency,
	LastSettingUpdated
};

struct SetupHardware
{
	uint8_t pwmchannelNo = 0;
	uint8_t pwmResolusion_bits = 8;
	uint16_t pwmduty_set = 0;
	uint32_t pwm_frequency = 3000UL;
	unsigned long lastsetting_updated = 0UL;
};
enum class TuningField
{
	AdjustPwm25P,
	AdjustPwm50P,
	AdjustPwm75P,
	AdjustPwm100P
};

struct SetupTuning
{
	uint16_t adjust_pwm_25P = 0;
	uint16_t adjust_pwm_50P = 0;
	uint16_t adjust_pwm_75P = 0;
	uint16_t adjust_pwm_100P = 0;
};
enum class NetworkField
{
	ApSSID,
	ApPass,
	StaSSID,
	StaPass,
	StaIP,
	StaGW,
	StaSN,
	DHCP,
	MaxRetry,
	ReconnectTimeout,
	NetworkTimeout,
	RefreshConnectionAfter,
	LastSettingUpdated
};

struct SetupNetwork
{
	const char* AP_SSID = "UPS_Test_AP";
	const char* AP_PASS = "password";
	const char* STA_SSID = "UPS_Test_STA";
	const char* STA_PASS = "password";
	IPAddress STA_IP = IPAddress(192, 168, 1, 100);
	IPAddress STA_GW = IPAddress(192, 168, 1, 1);
	IPAddress STA_SN = IPAddress(255, 255, 255, 0);
	bool DHCP = true;
	int max_retry = 3;
	uint32_t reconnectTimeout_ms = 5000UL;
	uint32_t networkTimeout_ms = 10000UL;
	uint32_t refreshConnectionAfter_ms = 86400000UL;
	unsigned long lastsetting_updated = 0UL;
};
enum class ModbusField
{
	SlaveID,
	DataBits,
	StopBits,
	Parity,
	BaudRate,
	LastSettingUpdated
};

struct SetupModbus
{
	uint8_t slaveID = 1;
	uint8_t databits = 8;
	uint8_t stopbits = 1;
	uint8_t parity = 0;
	unsigned long baudrate = 9600;
	unsigned long lastsetting_updated = 0UL;
};
enum class ReportField
{
	EnableReport,
	ReportFormat,
	ClientName,
	BrandName,
	SerialNumber,
	SampleNumber,
	LastSettingUpdated
};

struct SetupReport
{
	bool enableReport = true;
	const char* ReportFormat = "PDF";
	const char* clientName = "Default";
	const char* brandName = "Default";
	const char* serialNumber = "SN12345";
	int sampleNumber = 1;
	unsigned long lastsetting_updated = 0UL;
};

struct SetupUPSTest
{
	SetupSpec spec;
	SetupTest testSetting;
	SetupTask taskSetting;
	SetupTaskParams paramsSetting;
	SetupHardware hardwareSetting;
	SetupNetwork commSetting;
	SetupModbus modbusSetting;
	SetupReport reportSetting;
	unsigned long lastsetting_updated;

	SetupUPSTest(SetupSpec sp = SetupSpec(), SetupTest ts = SetupTest(), SetupTask tk = SetupTask(),
				 SetupTaskParams ps = SetupTaskParams(), SetupHardware hw = SetupHardware(),
				 SetupNetwork comm = SetupNetwork(), SetupModbus mb = SetupModbus(),
				 SetupReport rp = SetupReport(), unsigned long lsu = 0) :
		spec(sp),
		testSetting(ts), taskSetting(tk), paramsSetting(ps), hardwareSetting(hw), commSetting(comm),
		modbusSetting(mb), reportSetting(rp), lastsetting_updated(lsu)
	{
	}
};

} // namespace Node_Core

#endif