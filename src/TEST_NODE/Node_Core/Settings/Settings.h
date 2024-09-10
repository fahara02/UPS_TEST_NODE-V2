#ifndef SETTINGS_H
#define SETTINGS_H
#include "Arduino.h"
#include <IPAddress.h>
#include <stdint.h>
#include <time.h>
#include "ctime"
#include <cstring>
#include "NodeConstants.h"
#include "SetupDefines.h"

namespace Node_Core
{

// struct SetupSpec
// {
// 	uint16_t Rating_va;
// 	uint16_t RatedVoltage_volt;
// 	uint16_t RatedCurrent_amp;
// 	uint16_t MinInputVoltage_volt;
// 	uint16_t MaxInputVoltage_volt;
// 	unsigned long AvgSwitchTime_ms;
// 	unsigned long AvgBackupTime_ms;

// 	enum class Field
// 	{
// 		RatingVa,
// 		RatedVoltage,
// 		RatedCurrent,
// 		MinInputVoltage,
// 		MaxInputVoltage,
// 		AvgSwitchTime,
// 		AvgBackupTime
// 	};

// 	struct SpecField
// 	{
// 		const char* fieldName;
// 		Field field;
// 	};

// 	static const SpecField specFields[7];

// 	SetupSpec() :
// 		Rating_va(2000), RatedVoltage_volt(230), RatedCurrent_amp(6), MinInputVoltage_volt(180),
// 		MaxInputVoltage_volt(230), AvgSwitchTime_ms(50UL), AvgBackupTime_ms(300000UL)
// 	{
// 	}

// 	bool setField(Field field, uint32_t value)
// 	{
// 		bool updated = false;

// 		switch(field)
// 		{
// 			case Field::RatingVa:
// 				Rating_va = static_cast<uint16_t>(value);
// 				updated = true;
// 				break;
// 			case Field::RatedVoltage:
// 				RatedVoltage_volt = static_cast<uint16_t>(value);
// 				updated = true;
// 				break;
// 			case Field::RatedCurrent:
// 				RatedCurrent_amp = static_cast<uint16_t>(value);
// 				updated = true;
// 				break;
// 			case Field::MinInputVoltage:
// 				MinInputVoltage_volt = static_cast<uint16_t>(value);
// 				updated = true;
// 				break;
// 			case Field::MaxInputVoltage:
// 				MaxInputVoltage_volt = static_cast<uint16_t>(value);
// 				updated = true;
// 				break;
// 			case Field::AvgSwitchTime:
// 				AvgSwitchTime_ms = value;
// 				updated = true;
// 				break;
// 			case Field::AvgBackupTime:
// 				AvgBackupTime_ms = value;
// 				updated = true;
// 				break;
// 		}

// 		if(updated)
// 		{
// 			lastsetting_updated = millis(); // Update the milliseconds timestamp
// 			lastSettingtime = time(nullptr); // Update the real-time timestamp
// 		}

// 		return updated;
// 	}

// 	bool updateFieldByName(const char* fieldName, uint32_t value)
// 	{
// 		for(const auto& specField: specFields)
// 		{
// 			if(strcmp(specField.fieldName, fieldName) == 0)
// 			{
// 				return setField(specField.field, value);
// 			}
// 		}
// 		return false; // Field name not found
// 	}

// 	unsigned lastUpdate() const
// 	{
// 		return lastsetting_updated; // Return the milliseconds timestamp
// 	}

// 	const char* lastUpdateTime() const
// 	{
// 		struct tm* timeinfo = localtime(&lastSettingtime);
// 		if(!timeinfo)
// 		{
// 			std::strncpy(last_update_str, "Invalid Time", sizeof(last_update_str));
// 			return last_update_str;
// 		}
// 		std::strftime(last_update_str, sizeof(last_update_str), "%b %d %Y %H:%M:%S", timeinfo);
// 		return last_update_str;
// 	}

//   private:
// 	SettingType typeofSetting = SettingType::SPEC;
// 	unsigned long lastsetting_updated = 0UL; // Store the last update time in milliseconds
// 	time_t lastSettingtime = 0; // Store the real-time timestamp
// 	mutable char last_update_str[20];
// };

// // Define the specFields array outside the struct
// static const SetupSpec::SpecField SetupSpec::specFields[] = {
// 	{"Rating_va", SetupSpec::Field::RatingVa},
// 	{"RatedVoltage_volt", SetupSpec::Field::RatedVoltage},
// 	{"RatedCurrent_amp", SetupSpec::Field::RatedCurrent},
// 	{"MinInputVoltage_volt", SetupSpec::Field::MinInputVoltage},
// 	{"MaxInputVoltage_volt", SetupSpec::Field::MaxInputVoltage},
// 	{"AvgSwitchTime_ms", SetupSpec::Field::AvgSwitchTime},
// 	{"AvgBackupTime_ms", SetupSpec::Field::AvgBackupTime}};

struct SetupSpec
{
	uint16_t Rating_va = 2000;
	uint16_t RatedVoltage_volt = 230;
	uint16_t RatedCurrent_amp = 6;
	uint16_t MinInputVoltage_volt = 180;
	uint16_t MaxInputVoltage_volt = 230;
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

	bool setField(Field field, uint32_t value)
	{
		bool updated = false;

		switch(field)
		{
			case Field::RatingVa:
				if(value >= UPS_MIN_VA && value <= UPS_MAX_VA)
				{
					Rating_va = static_cast<uint16_t>(value);
					Serial.println("Rating VA updated");
					updated = true;
				}
				break;

			case Field::RatedVoltage:
				if(value >= UPS_MIN_INPUT_VOLT && value <= UPS_MAX_INPUT_VOLT)
				{
					RatedVoltage_volt = static_cast<uint16_t>(value);
					Serial.println("Rated voltage updated");
					updated = true;
				}
				break;

			case Field::RatedCurrent:
				if(value >= UPS_MIN_OUTPUT && value <= UPS_MAX_OUTPUT)
				{
					RatedCurrent_amp = static_cast<uint16_t>(value);
					updated = true;
				}
				break;

			case Field::MinInputVoltage:
				if(value >= UPS_MIN_INPUT_VOLT && value < UPS_MAX_INPUT_VOLT)
				{
					MinInputVoltage_volt = static_cast<uint16_t>(value);
					updated = true;
				}
				break;

			case Field::MaxInputVoltage:
				if(value > UPS_MIN_INPUT_VOLT && value <= UPS_MAX_INPUT_VOLT)
				{
					MaxInputVoltage_volt = static_cast<uint16_t>(value);
					updated = true;
				}
				break;

			case Field::AvgSwitchTime:
				if(value <= UPS_MAX_SWITCHING_TIME_MS)
				{
					AvgSwitchTime_ms = value;
					updated = true;
				}
				break;

			case Field::AvgBackupTime:
				if(value >= UPS_MIN_BACKUP_TIME_MS)
				{
					AvgBackupTime_ms = value;
					updated = true;
				}
				break;
		}

		if(updated)
		{
			lastsetting_updated = millis(); // Update the milliseconds timestamp
			lastSettingtime = time(nullptr); // Update the real-time timestamp
			return true;
		}

		return false;
	}

	unsigned lastUpdate() const
	{
		return lastsetting_updated; // Return the milliseconds timestamp
	}

	const char* lastUpdateTime() const
	{
		struct tm* timeinfo = localtime(&lastSettingtime);
		if(!timeinfo)
		{
			std::strncpy(last_update_str, "Invalid Time", sizeof(last_update_str));
			return last_update_str;
		}
		std::strftime(last_update_str, sizeof(last_update_str), "%b %d %Y %H:%M:%S", timeinfo);
		return last_update_str;
	}

  private:
	SettingType typeofSetting = SettingType::SPEC;
	unsigned long lastsetting_updated = 0UL; // Store the last update time in milliseconds
	time_t lastSettingtime = 0; // Store the real-time timestamp
	mutable char last_update_str[20];
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

	enum class Field
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
		MaxRetest
	};

	bool setField(Field field, uint32_t value)
	{
		switch(field)
		{
			case Field::TestStandard:
				TestStandard = reinterpret_cast<const char*>(value);
				Serial.println("TestStandard is updated");
				break;
			case Field::Mode:
				mode = static_cast<TestMode>(value);
				Serial.println("mode is updated");
				break;
			case Field::TestVARating:
				if(value >= UPS_MIN_VA && value <= UPS_MAX_VA)
				{
					testVARating = static_cast<uint16_t>(value);
				}
				else
				{
					return false;
				}
				break;
			case Field::InputVoltage:
				if(value >= UPS_MIN_INPUT_VOLT && value <= UPS_MAX_INPUT_VOLT)
				{
					inputVoltage_volt = static_cast<uint16_t>(value);
				}
				else
				{
					return false;
				}
				break;
			case Field::TestDuration:
				testDuration_ms = value;
				break;
			case Field::MinValidSwitchTime:
				if(value >= UPS_MIN_SWITCHING_TIME_MS_SANITY_CHECK &&
				   value <= UPS_MAX_SWITCHING_TIME_MS_SANITY_CHECK)
				{
					min_valid_switch_time_ms = value;
				}
				else
				{
					return false;
				}
				break;
			case Field::MaxValidSwitchTime:
				if(value >= UPS_MIN_SWITCHING_TIME_MS_SANITY_CHECK &&
				   value <= UPS_MAX_SWITCHING_TIME_MS_SANITY_CHECK)
				{
					max_valid_switch_time_ms = value;
				}
				else
				{
					return false;
				}
				break;
			case Field::MinValidBackupTime:
				if(value >= UPS_MIN_BACKUP_TIME_MS_SANITY_CHECK)
				{
					min_valid_backup_time_ms = value;
				}
				else
				{
					return false;
				}
				break;
			case Field::MaxValidBackupTime:
				if(value >= UPS_MIN_BACKUP_TIME_MS_SANITY_CHECK)
				{
					max_valid_backup_time_ms = value;
				}
				else
				{
					return false;
				}
				break;
			case Field::ToleranceSwitchTime:
				ToleranceSwitchTime_ms = value;
				break;
			case Field::MaxBackupTime:
				maxBackupTime_min = value;
				break;
			case Field::ToleranceBackupTime:
				ToleranceBackUpTime_ms = value;
				break;
			case Field::MaxRetest:
				MaxRetest = static_cast<int>(value);
				break;
		}
		lastsetting_updated = millis(); // Update the timestamp to the current time.
		return true;
	}

	unsigned lastUpdate() const
	{
		return lastsetting_updated;
	}

	const char* lastUpdateTime() const
	{
		std::time_t t = lastsetting_updated;
		std::strftime(last_update_str, sizeof(last_update_str), "%Y-%m-%d %H:%M:%S",
					  std::localtime(&t));
		return last_update_str;
	}

  private:
	SettingType typeofSetting = SettingType::TEST;
	unsigned long lastsetting_updated = 0UL;
	unsigned long lastupdate_time = 0UL;
	mutable char last_update_str[20];
};

struct SetupTask
{
	int mainTest_taskCore = 0;
	int mainsISR_taskCore = ARDUINO_RUNNING_CORE;
	int upsISR_taskCore = ARDUINO_RUNNING_CORE;
	int mainTest_taskIdlePriority = 1;
	int mainsISR_taskIdlePriority = 1;
	int upsISR_taskIdlePriority = 1;
	uint32_t mainTest_taskStack = 4096;
	uint32_t mainsISR_taskStack = 1024;
	uint32_t upsISR_taskStack = 1024;
	unsigned long lastsetting_updated = 0UL;
	enum class Field
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
	enum class Field
	{
		FlagMainsPowerLoss,
		FlagUpsPowerGain,
		FlagUpsPowerLoss,
		TestNo,
		TaskTestVARating,
		TaskTestDuration,
		LastSettingUpdated
	};
};

struct SetupHardware
{
	uint8_t pwmchannelNo = 0;
	uint8_t pwmResolusion_bits = 8;
	uint16_t pwmduty_set = 0;
	uint32_t pwm_frequency = 3000UL;
	unsigned long lastsetting_updated = 0UL;
	enum class Field
	{
		PwmChannelNo,
		PwmResolutionBits,
		PwmDutySet,
		PwmFrequency,
		LastSettingUpdated
	};
};

struct SetupTuning
{
	uint16_t adjust_pwm_25P = 0;
	uint16_t adjust_pwm_50P = 0;
	uint16_t adjust_pwm_75P = 0;
	uint16_t adjust_pwm_100P = 0;
	enum class Field
	{
		AdjustPwm25P,
		AdjustPwm50P,
		AdjustPwm75P,
		AdjustPwm100P
	};
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
	enum class Field
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
};

struct SetupModbus
{
	uint8_t slaveID = 1;
	uint8_t databits = 8;
	uint8_t stopbits = 1;
	uint8_t parity = 0;
	unsigned long baudrate = 9600;
	unsigned long lastsetting_updated = 0UL;
	enum class Field
	{
		SlaveID,
		DataBits,
		StopBits,
		Parity,
		BaudRate,
		LastSettingUpdated
	};
};

struct SetupReport
{
	bool enableReport = true;
	const char* reportFormat = "PDF";
	const char* clientName = "Default";
	const char* brandName = "Default";
	const char* serialNumber = "SN12345";
	int sampleNumber = 1;

	enum class Field
	{
		EnableReport,
		ReportFormat,
		ClientName,
		BrandName,
		SerialNumber,
		SampleNumber,
		LastSettingUpdated
	};

	bool setField(Field field, const char* value)
	{
		switch(field)
		{
			case Field::ReportFormat:
				reportFormat = value;
				return true;
			case Field::ClientName:
				clientName = value;
				return true;
			case Field::BrandName:
				brandName = value;
				return true;
			case Field::SerialNumber:
				serialNumber = value;
				return true;
			default:
				return false;
		}
	}

	bool setField(Field field, int value)
	{
		switch(field)
		{
			case Field::SampleNumber:
				sampleNumber = value;
				return true;
			case Field::EnableReport:
				enableReport = value;
				return true;
			default:
				return false;
		}
	}

	unsigned long lastUpdate() const
	{
		return lastsetting_updated;
	}

	const char* lastUpdateTime() const
	{
		std::time_t t = lastsetting_updated;
		std::strftime(last_update_str, sizeof(last_update_str), "%Y-%m-%d %H:%M:%S",
					  std::localtime(&t));
		return last_update_str;
	}

  private:
	unsigned long lastsetting_updated = 0UL;
	mutable char last_update_str[20];
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