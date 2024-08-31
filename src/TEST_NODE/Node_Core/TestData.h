#ifndef TEST_DATA_H
#define TEST_DATA_H

#include <stdint.h>
#include <algorithm>
#include <cctype>
#include <string>
#include "Arduino.h"

static int caseInsensitiveCompare(const char* str1, const char* str2)
{
	while(*str1 && *str2)
	{
		if(tolower(*str1) != tolower(*str2))
		{
			return *str1 - *str2;
		}
		str1++;
		str2++;
	}
	return *str1 - *str2;
}

enum TestResult
{
	TEST_FAILED = 0,
	TEST_PENDING = 1,
	TEST_SUCCESSFUL = 2
};

enum class TestType
{
	SwitchTest = 1 << 0, // 1
	BackupTest = 1 << 1, // 2
	EfficiencyTest = 1 << 2, // 3
	InputVoltageTest = 1 << 3, // 4
	WaveformTest = 1 << 4, // 5
	TunePWMTest = 1 << 5 // 6
};
enum LoadPercentage
{
	LOAD_0P = 0,
	LOAD_25P = 25,
	LOAD_50P = 50,
	LOAD_75P = 75,
	LOAD_100P = 100
};

enum class TestManagerStatus
{
	NOT_IN_QUEUE = 1 << 0,
	PENDING = 1 << 1,
	RETEST = 1 << 2,
	DONE = 1 << 3
};

//
enum class TestOperatorStatus
{
	NOT_STARTED = 1 << 4,
	RUNNING = 1 << 5,
	SUCCESS = 1 << 6,
	FAILED = 1 << 7
};

struct TestStatus
{
	TestManagerStatus managerStatus;
	TestOperatorStatus operatorStatus;
	TestStatus() :
		managerStatus(TestManagerStatus::PENDING), operatorStatus(TestOperatorStatus::NOT_STARTED)
	{
	}
};
// for outside use of  the class
struct RequiredTest
{
	int testId; // Unique test number
	TestType testType;
	LoadPercentage loadLevel;
	bool isActive = false;
	RequiredTest() :
		testId(0), testType(TestType::SwitchTest), loadLevel(LoadPercentage::LOAD_25P),
		isActive(true)
	{
	}
	RequiredTest(int Id, TestType type, LoadPercentage level, bool active) :
		testId(Id), testType(type), loadLevel(level), isActive(active)
	{
	}
};

struct UPSTestRun
{
	RequiredTest testRequired;
	TestStatus testStatus;
	UPSTestRun() :
		testRequired(), // Calls the default constructor of RequiredTest
		testStatus() // Calls the default constructor of TestStatus
	{
	}
};

struct TestData
{
	uint8_t testNo;
	unsigned long testTimestamp;
	unsigned long starttime;
	unsigned long endtime;
	LoadPercentage load_percentage : 7;
	bool valid_data : 1;

	// Default constructor for TestData
	TestData() :
		testNo(0), testTimestamp(0), starttime(0), endtime(0),
		load_percentage(LoadPercentage::LOAD_0P), valid_data(false)
	{
	}
};

// Derived struct for SwitchTestData
struct SwitchTestData
{
	struct SingleTest : public TestData
	{
		unsigned long switchtime;

		SingleTest() : TestData(), switchtime(0)
		{
		}
	} switchTest[5];

	// Default constructor for SwitchTestData
	SwitchTestData()
	{
		// Initialize the array of SingleTest
		for(int i = 0; i < 5; ++i)
		{
			switchTest[i] = SingleTest();
		}
	}
};

struct BackupTestData
{
	struct SingleTest : public TestData
	{
		unsigned long backuptime;

		SingleTest() :
			TestData(), // Explicitly initialize the base class
			backuptime(0)
		{
		}
	} backupTest[5];

	// Default constructor for BackupTestData
	BackupTestData()
	{
		// Initialize the array of SingleTest
		for(int i = 0; i < 5; ++i)
		{
			backupTest[i] = SingleTest();
		}
	}
};

static const char* testTypeToString(TestType type)
{
	switch(type)
	{
		case TestType::SwitchTest:
			return "SwitchTest";
		case TestType::BackupTest:
			return "BackupTest";
		case TestType::EfficiencyTest:
			return "EfficiencyTest";
		case TestType::InputVoltageTest:
			return "InputVoltageTest";
		case TestType::WaveformTest:
			return "WaveformTest";
		case TestType::TunePWMTest:
			return "TunePWMTest";
		default:
			return "UnknownTest";
	}
}

static const char* loadPercentageToString(LoadPercentage load)
{
	switch(load)
	{
		case LOAD_0P:
			return "0%";
		case LOAD_25P:
			return "25%";
		case LOAD_50P:
			return "50%";
		case LOAD_75P:
			return "75%";
		case LOAD_100P:
			return "100%";
		default:
			return "Unknown";
	}
}

static TestType getTestTypeFromString(const String& testName)
{
	std::string tName = "";
	tName = static_cast<std::string>(testName.c_str());
	tName.erase(std::remove_if(tName.begin(), tName.end(), ::isspace), tName.end());

	const char* trimmedTestNameCStr = tName.c_str();

	if(caseInsensitiveCompare(trimmedTestNameCStr, "SwitchTest") == 0)
		return TestType::SwitchTest;
	else if(caseInsensitiveCompare(trimmedTestNameCStr, "BackupTest") == 0)
		return TestType::BackupTest;
	else if(caseInsensitiveCompare(trimmedTestNameCStr, "EfficiencyTest") == 0)
		return TestType::EfficiencyTest;
	else if(caseInsensitiveCompare(trimmedTestNameCStr, "InputVoltageTest") == 0)
		return TestType::InputVoltageTest;
	else if(caseInsensitiveCompare(trimmedTestNameCStr, "WaveformTest") == 0)
		return TestType::WaveformTest;
	else if(caseInsensitiveCompare(trimmedTestNameCStr, "TunePWMTest") == 0)
		return TestType::TunePWMTest;

	return static_cast<TestType>(0);
}

static LoadPercentage getLoadLevelFromString(const String& loadLevel)

{
	std::string tload = "";
	tload = static_cast<std::string>(loadLevel.c_str());
	tload.erase(std::remove_if(tload.begin(), tload.end(), ::isspace), tload.end());
	const char* load = tload.c_str();

	if(caseInsensitiveCompare(load, "0%") == 0)
		return LoadPercentage::LOAD_0P;
	else if(caseInsensitiveCompare(load, "25%") == 0)
		return LoadPercentage::LOAD_25P;
	else if(caseInsensitiveCompare(load, "50%") == 0)
		return LoadPercentage::LOAD_50P;
	else if(caseInsensitiveCompare(load, "75%") == 0)
		return LoadPercentage::LOAD_75P;
	else if(caseInsensitiveCompare(load, "100%") == 0)
		return LoadPercentage::LOAD_100P;

	return static_cast<LoadPercentage>(-1); // -1 indicates an invalid load level
}

#endif