#ifndef BASE_SETUP_H
#define BASE_SETUP_H
#include "Arduino.h"
#include <IPAddress.h>
#include <stdint.h>
#include <time.h>
#include "ctime"
#include <cstring>
#include "NodeConstants.h"
namespace Node_Core
{
struct SetupBus; // Forward declaration
}

namespace Node_Core
{

template<typename T, typename FieldEnum>
struct BaseSetup
{
	struct SpecField
	{
		const char* fieldName;
		FieldEnum field;
	};

	bool updateFieldByName(const char* fieldName, uint32_t value)
	{
		T* derived = static_cast<T*>(this);
		for(const auto& specField: derived->specFields)
		{
			if(strcmp(specField.fieldName, fieldName) == 0)
			{
				return derived->setField(specField.field, value);
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
	// Pure virtual method for default initialization

	unsigned long lastsetting_updated = 0UL; // Store the last update time in milliseconds
	time_t lastSettingtime = 0; // Store the real-time timestamp
	mutable char last_update_str[20];
};
// template<typename T, typename FieldEnum>
// const typename BaseSetup<T, FieldEnum>::SpecField BaseSetup<T, FieldEnum>::specFields[] = {
// 	// Populate with actual values if needed
// };
// template class BaseSetup<Node_Core::SetupSpec, Node_Core::SetupSpec::Field>;
} // namespace Node_Core

#endif