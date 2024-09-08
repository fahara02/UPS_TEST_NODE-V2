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
enum class wsPowerDataType
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

enum class wsOutGoingDataType
{
	POWER_READINGS,
	LED_STATUS,
	BUTTONS_STATUS,
	INVALID_DATA
};

static const char* outgoingLedTable[] = {
	"blinkBlue", // BLINK_SETUP
	"blinkGreen", // BLINK_READY
	"blinkRed" // BLINK_RUNNING
};

struct PeriodicTaskParams
{
	AsyncWebSocket* ws;
};

static const char* wsPowerDataTypeToString(wsPowerDataType type)
{
	switch(type)
	{
		case wsPowerDataType::INPUT_POWER:
			return "InputPower";
		case wsPowerDataType::INPUT_VOLT:
			return "InputVoltage";
		case wsPowerDataType::INPUT_CURRENT:
			return "InputCurrent";
		case wsPowerDataType::INPUT_PF:
			return "InputPowerFactor";
		case wsPowerDataType::OUTPUT_POWER:
			return "OutputPower";
		case wsPowerDataType::OUTPUT_VOLT:
			return "OutputVoltage";
		case wsPowerDataType::OUTPUT_CURRENT:
			return "OutputCurrent";
		case wsPowerDataType::OUTPUT_PF:
			return "OutputPowerFactor";
		default:
			return "Invalid Data";
	}
}

namespace Node_Core
{
static constexpr size_t WS_BUFFER_SIZE = 256;

static constexpr TickType_t QUEUE_TIMEOUT_MS = pdMS_TO_TICKS(100);
static constexpr TickType_t DATABIT_TIMEOUT_MS = pdMS_TO_TICKS(200);

static constexpr TickType_t CLIENT_CONNECT_TIMEOUT_MS = pdMS_TO_TICKS(1000);

struct WebSocketMessage
{
	uint8_t data[WS_BUFFER_SIZE];
	size_t len;
	AwsFrameInfo info;
	int client_id;

	WebSocketMessage() : len(0), info(), client_id(0)
	{
		memset(data, 0, sizeof(data));
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

	bool _periodicSendRequest;

	static void periodicDataSender(void* pvParameter);

  private:
	DataHandler();

	ProcessingResult _result;
	StaticJsonDocument<WS_BUFFER_SIZE> _blankDoc;

	static void wsDataProcessor(void* pVparamter);
	TaskHandle_t dataTaskHandler = NULL;

	// data handling functions
	void sendData(AsyncWebSocket* websocket, int clientId,
				  wsOutGoingDataType type = wsOutGoingDataType::POWER_READINGS);
	void processWsMessage(WebSocketMessage& wsMsg);
	void handleWsIncomingCommands(wsIncomingCommands cmd);

	wsIncomingCommands getWebSocketCommand(const char* incomingCommand);
	StaticJsonDocument<WS_BUFFER_SIZE> prepData(wsOutGoingDataType type);
	bool isValidUTF8(const char* data, size_t len);

	DataHandler(const DataHandler&) = delete;
	DataHandler& operator=(const DataHandler&) = delete;
};

} // namespace Node_Core
#endif