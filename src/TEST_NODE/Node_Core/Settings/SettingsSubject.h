#ifndef SETTINGS_SUBJECT_H
#define SETTINGS_SUBJECT_H

#include <vector>
#include <algorithm>
#include "SettingsObserver.h"
namespace Node_Core
{
class SettingsSubject
{
  public:
	void addObserver(SettingsObserver* observer)
	{
		observers.push_back(observer);
	}

	void removeObserver(SettingsObserver* observer)
	{
		observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
	}

  protected:
	void notifyObservers(SettingType type, const void* settings)
	{
		for(SettingsObserver* observer: observers)
		{
			observer->onSettingsUpdate(type, settings);
		}
	}

  private:
	std::vector<SettingsObserver*> observers;
};

} // namespace Node_Core

#endif