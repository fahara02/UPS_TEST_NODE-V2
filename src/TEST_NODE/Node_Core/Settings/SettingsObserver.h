#ifndef SETTINGS_OBSERVER_H
#define SETTINGS_OBSERVER_H
#include "Settings.h"
namespace Node_Core
{

class SettingsObserver
{
  public:
	virtual void onSettingsUpdate(SettingType type, const void* settings) = 0;
	virtual ~SettingsObserver() = default;
};

} // namespace Node_Core

#endif