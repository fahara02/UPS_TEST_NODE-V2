#ifndef SETUP_BASE_H
#define SETUP_BASE_H
#include "Arduino.h"
#include <IPAddress.h>
#include <stdint.h>
#include <time.h>
#include "ctime"
#include <cstring>
#include "NodeConstants.h"

template<typename T, typename FieldEnum>
struct BaseSetup
{
	struct SpecField
	{
		const char* fieldName;
		FieldEnum field;
	};

	static const SpecField specFields[];

	BaseSetup()
	{
		initializeDefaults();
	}

	virtual ~BaseSetup() = default;

	bool updateFieldByName(const char* fieldName, uint32_t value)
	{
		for(const auto& specField: specFields)
		{
			if(strcmp(specField.fieldName, fieldName) == 0)
			{
				return setField(specField.field, value);
				lastsetting_updated = millis();
			}
		}
		return false; // Field name not found
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

  protected:
	virtual void initializeDefaults() = 0; // Pure virtual method for default initialization

	virtual bool setField(FieldEnum field, uint32_t value) = 0; // Pure virtual function

	unsigned long lastsetting_updated = 0UL; // Store the last update time in milliseconds
	time_t lastSettingtime = 0; // Store the real-time timestamp
	mutable char last_update_str[20];
};

#endif