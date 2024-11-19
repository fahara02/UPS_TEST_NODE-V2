#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H
#include "ArduinoJson.h"
#include <ESPAsyncWebServer.h>
#include "TestData.h"
#include "Settings.h"
#include "Logger.h"
#include "TestSync.h"

#include <array>
#include <set>
#include "wsDefines.hpp"



// static const char* outgoingLedTable[] = {
// 	"blinkBlue", // BLINK_SETUP
// 	"blinkGreen", // BLINK_READY
// 	"blinkRed" // BLINK_RUNNING
// };

struct PeriodicTaskParams
{
	AsyncWebSocket* ws;
};



namespace Node_Core
{
static constexpr size_t WS_BUFFER_SIZE = 256;

static constexpr TickType_t QUEUE_TIMEOUT_MS = pdMS_TO_TICKS(10);
static constexpr TickType_t DATABIT_TIMEOUT_MS = pdMS_TO_TICKS(200);

static constexpr TickType_t CLIENT_CONNECT_TIMEOUT_MS = pdMS_TO_TICKS(1000);
static constexpr TickType_t READ_TIMEOUT_MS = pdMS_TO_TICKS(2000);
struct WebSocketMessage
{
	uint8_t data[WS_BUFFER_SIZE];
	size_t len;
	AwsFrameInfo info;
	int client_id;
	AsyncWebSocketClient* client;

	WebSocketMessage() : len(0), info(), client_id(0), client(nullptr)
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

	static void periodicDataSender(void* pvParameter);
	void updateState(State state)
	{
		_currentState.store(state);
	}
	void updateMode(TestMode mode)
	{
		_deviceMode.store(mode);
	}

	TaskHandle_t dataTaskHandler = NULL;
	TaskHandle_t PeriodicDataHandle = NULL;
	void updateNewClientId(int Id)
	{
		_newClietId.store(Id);
	}

	void updateClientList(int clientId, bool connected);

  private:
	DataHandler();
	bool _updateLedStatus;
	bool _periodicSendRequest;
	bool _blinkBlue;
	bool _blinkGreen;
	bool _blinkRed;
	std::atomic<int> _newClietId{0};
	std::set<int> connectedClients;
	ProcessingResult _result;
	JsonDocument _blankDoc;

	std::atomic<State> _currentState{State::DEVICE_ON};
	std::atomic<TestMode> _deviceMode{TestMode::MANUAL};

	static void wsDataProcessor(void* pVparamter);

	// data handling functions
	void sendData(AsyncWebSocket* websocket, int clientId,
				  wsOutGoingDataType type = wsOutGoingDataType::POWER_READINGS);
	void sendData(AsyncWebSocketClient* client,
				  wsOutGoingDataType type = wsOutGoingDataType::LED_STATUS);
	void processWsMessage(WebSocketMessage& wsMsg);
	void handleWsIncomingCommands(wsIncomingCommands cmd);
	void handleUserCommand(UserCommandEvent command);

	SemaphoreHandle_t websocketMutex;
	SemaphoreHandle_t clientListMutex;

	wsIncomingCommands getWebSocketCommand(const char* incomingCommand);
	JsonDocument prepData(wsOutGoingDataType type);
	void cleanUpClients(AsyncWebSocket* websocket);
	bool isValidUTF8(const char* data, size_t len);

	DataHandler(const DataHandler&) = delete;
	DataHandler& operator=(const DataHandler&) = delete;
};

} // namespace Node_Core
#endif