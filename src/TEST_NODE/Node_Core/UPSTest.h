#ifndef UPS_TEST_H
#define UPS_TEST_H

#include "Arduino.h"
#include "HardwareConfig.h"
#include "Logger.h"
#include "TestData.h"

#include "UPSTesterSetup.h"

using namespace Node_Core;
extern Logger& logger;

extern UPSTesterSetup* TesterSetup;

template<class T>
class UPSTest
{
  public:
	static T* getInstance();
	static void deleteInstance();
	// Pure virtual function to be implemented by derived classes
	virtual void init() = 0;
	virtual TestResult run(uint16_t testVARating, unsigned long testDuration) = 0;
	virtual void startTestCapture() = 0;
	virtual void stopTestCapture() = 0;
	virtual bool processTestImpl() = 0;

  protected:
	UPSTest(); // Protected constructor
	virtual ~UPSTest() = default;

	// Utility functions
	void setLoad(uint16_t testVARating);
	void selectLoadBank(uint16_t bankNumbers);
	void simulatePowerCut();
	void simulatePowerRestore();
	void sendEndSignal();
	void processTest(T& test);

	// Pure virtual functions to be implemented by derived classes

  private:
	static T* instance;

	// Prevent copying
	UPSTest(const UPSTest&) = delete;
	UPSTest& operator=(const UPSTest&) = delete;
};

template<typename T>
T* UPSTest<T>::instance = nullptr;

// Protected Constructor
template<typename T>
UPSTest<T>::UPSTest()
{
	// Constructor logic if needed
}

template<typename T>
T* UPSTest<T>::getInstance()
{
	if(instance == nullptr)
	{
		instance = new T();
	}
	return instance;
}

template<typename T>
void UPSTest<T>::deleteInstance()
{
	if(instance != nullptr)
	{
		delete instance;
		instance = nullptr;
	}
}

template<typename T>
void UPSTest<T>::setLoad(uint16_t testVARating)
{
	if(!TesterSetup)
		return; // Ensure TesterSetup is valid

	const uint16_t maxVARating = TesterSetup->specSetup().Rating_va;
	uint16_t singlebankVA = maxVARating / 4;
	uint16_t dualbankVA = (maxVARating / 4) * 2;
	uint16_t triplebankVA = (maxVARating / 4) * 3;
	uint16_t reqbankNumbers = 0;
	uint16_t duty = 0;
	uint16_t pwmValue = 0;
	uint16_t adjustpwm = 0;

	if(testVARating <= singlebankVA)
	{
		reqbankNumbers = 1;
		duty = (testVARating * 100) / singlebankVA;
		pwmValue = map(testVARating, 0, singlebankVA, 0, 255);
		adjustpwm = TesterSetup->tuningSetup().adjust_pwm_25P;
	}
	else if(testVARating > singlebankVA && testVARating <= dualbankVA)
	{
		reqbankNumbers = 2;
		duty = (testVARating * 100) / dualbankVA;
		pwmValue = map(testVARating, 0, dualbankVA, 0, 255);
		adjustpwm = TesterSetup->tuningSetup().adjust_pwm_50P;
	}
	else if(testVARating > dualbankVA && testVARating <= triplebankVA)
	{
		reqbankNumbers = 3;
		duty = (testVARating * 100) / triplebankVA;
		pwmValue = map(testVARating, 0, triplebankVA, 0, 255);
		adjustpwm = TesterSetup->tuningSetup().adjust_pwm_75P;
	}
	else if(testVARating > triplebankVA && testVARating <= maxVARating)
	{
		reqbankNumbers = 4;
		duty = (testVARating * 100) / maxVARating;
		pwmValue = map(testVARating, 0, maxVARating, 0, 255);
		adjustpwm = TesterSetup->tuningSetup().adjust_pwm_100P;
	}

	uint16_t set_pwmValue = pwmValue + adjustpwm;
	ledcWrite(TesterSetup->hardwareSetup().pwmchannelNo, set_pwmValue);

	selectLoadBank(reqbankNumbers);
}

template<typename T>
void UPSTest<T>::selectLoadBank(uint16_t bankNumbers)
{
	digitalWrite(LOAD25P_ON_PIN, bankNumbers >= 1 ? HIGH : LOW);
	digitalWrite(LOAD50P_ON_PIN, bankNumbers >= 2 ? HIGH : LOW);
	digitalWrite(LOAD75P_ON_PIN, bankNumbers >= 3 ? HIGH : LOW);
	digitalWrite(LOAD_FULL_ON_PIN, bankNumbers >= 4 ? HIGH : LOW);
}

template<typename T>
void UPSTest<T>::sendEndSignal()
{
	digitalWrite(TEST_END_INT_PIN, HIGH);
	vTaskDelay(pdMS_TO_TICKS(10));
	digitalWrite(TEST_END_INT_PIN, LOW);
}

template<typename T>
void UPSTest<T>::simulatePowerCut()
{
	digitalWrite(UPS_POWER_CUT_PIN, HIGH); // Simulate power cut
}

template<typename T>
void UPSTest<T>::simulatePowerRestore()
{
	digitalWrite(UPS_POWER_CUT_PIN, LOW); // Simulate mains power restore
}

template<typename T>
void UPSTest<T>::processTest(T& test)
{
	// Placeholder for processing the test; type-specific implementations should override
	// processTestImpl
	processTestImpl();
}

#endif // UPS_TEST_H
