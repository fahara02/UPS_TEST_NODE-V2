#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H
#include "ArduinoJson.h"
#include <ESPAsyncWebServer.h>
#include "TestData.h"
#include "Settings.h"
#include "Logger.h"
#include "TestSync.h"
#include <deque>
#include <array>

enum class wsIncomingCommands
{
	TEST_START,
	TEST_STOP,
	TEST_PAUSE,
	AUTO_MODE,
	MANUAL_MODE,
	LOAD_ON,
	LOAD_OFF,
	MAINS_ON,
	MAINS_OFF,
	GET_READINGS,

	INVALID_COMMAND // Handle invalid cases
};

enum class wsOutgoingCommands
{
	BLINK_SETUP,
	BLINK_READY,
	BLINK_RUNNING,
	INVALID_COMMAND // Handle invalid cases
};
enum class wsOutGoingDataType
{
	INPUT_POWER,
	INPUT_VOLT,
	INPUT_CURRENT,
	INPUT_PF,
	OUTPUT_POWER,
	OUTPUT_VOLT,
	OUTPUT_CURRENT,
	OUTPUT_PF,
	INVALID_DATA
};

static const char* outgoingCommandTable[] = {
	"blinkBlue", // BLINK_SETUP
	"blinkGreen", // BLINK_READY
	"blinkRed" // BLINK_RUNNING
};

static const char* wsDataTypeToString(wsOutGoingDataType type)
{
	switch(type)
	{
		case wsOutGoingDataType::INPUT_POWER:
			return "InputPower";
		case wsOutGoingDataType::INPUT_VOLT:
			return "InputVoltage";
		case wsOutGoingDataType::INPUT_CURRENT:
			return "InputCurrent";
		case wsOutGoingDataType::INPUT_PF:
			return "InputPowerFactor";
		case wsOutGoingDataType::OUTPUT_POWER:
			return "OutputPower";
		case wsOutGoingDataType::OUTPUT_VOLT:
			return "OutputVoltage";
		case wsOutGoingDataType::OUTPUT_CURRENT:
			return "OutputCurrent";
		case wsOutGoingDataType::OUTPUT_PF:
			return "OutputPowerFactor";
		default:
			return "Invalid Data";
	}
}

namespace Node_Core
{

static constexpr size_t WS_BUFFER_SIZE = 256;
struct WebSocketMessage
{
	uint8_t* data; // Pointer to raw data
	size_t len; // Length of data
	AwsFrameInfo info; // Frame info (could copy from arg)
	WebSocketMessage() : data(nullptr), len(0), info()
	{
	}
};

enum class ProcessingResult
{
	SUCCESS,
	FAILED,
	ONGOING,
	PENDING,
	INVALID_DATA
};

class DataHandler
{
  public:
	static DataHandler& getInstance();
	void init();
	QueueHandle_t WebsocketDataQueue = NULL;
	std::deque<std::array<char, 256>> wsDeque;

  private:
	DataHandler();
	bool _isDataProcessing;
	bool _isProcessingDone;
	ProcessingResult _result;

	// Fixed-size array in deque
	StaticJsonDocument<WS_BUFFER_SIZE> _blankDoc; // JSON document to store data

	static void wsDataProcessor(void* pVparamter);
	TaskHandle_t dataTaskHandler = nullptr;

	// data handling functions

	void processWsMessage(WebSocketMessage& wsMsg);
	void handleWsIncomingCommands(wsIncomingCommands cmd);
	void fillDequeWithData();
	wsIncomingCommands getWebSocketCommand(const char* incomingCommand);
	void prepWebSocketData(wsOutGoingDataType type, const char* data);
	void sendRandomTestData();

	DataHandler(const DataHandler&) = delete;
	DataHandler& operator=(const DataHandler&) = delete;
};

} // namespace Node_Core
#endif