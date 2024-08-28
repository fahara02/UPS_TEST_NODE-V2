#ifndef TEST_DATA_H
#define TEST_DATA_H
#include <stdint.h>
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

#endif