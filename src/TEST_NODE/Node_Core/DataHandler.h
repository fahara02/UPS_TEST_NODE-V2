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

namespace Node_Core
{
static constexpr size_t WS_BUFFER_SIZE = 256;
static constexpr size_t WS_QUEUE_SIZE = 20;
static constexpr TickType_t QUEUE_TIMEOUT_MS = pdMS_TO_TICKS(300);
static constexpr TickType_t DATABIT_TIMEOUT_MS = pdMS_TO_TICKS(200);
static constexpr TickType_t CLIENT_CONNECT_TIMEOUT_MS = pdMS_TO_TICKS(1000);
static constexpr TickType_t READ_TIMEOUT_MS = pdMS_TO_TICKS(2000);

struct WsDataHandlerTaskParams
{
	AsyncWebSocket* ws;
};

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

class DataHandler
{
  public:
	// Singleton Instance Access
	static DataHandler& getInstance();
	// Initialization
	void init();

	// Task Functions
	static void wsDataHandler(void* pvParameter);

	// State and Mode Management
	void updateState(State state);
	void updateMode(TestMode mode);
	void updateNewClientId(int Id);

	// Client Management
	void updateClientList(int clientId, bool connected);

	// Data Handling
	void sendData(AsyncWebSocket* websocket, int clientId,
				  wsOutGoingDataType type = wsOutGoingDataType::POWER_READINGS);
	void sendData(AsyncWebSocketClient* client,
				  wsOutGoingDataType type = wsOutGoingDataType::LED_STATUS);
	void processWsMessage(WebSocketMessage& wsMsg);
	void handleWsIncomingCommands(wsIncomingCommands cmd);
	void handleUserCommand(UserCommandEvent command);
	// Task Handles
	TaskHandle_t dataTaskHandler = NULL;
	// Queues
	QueueHandle_t WebsocketDataQueue = NULL;

  private:
	// Constructor and Deleted Copy
	DataHandler();

	DataHandler(const DataHandler&) = delete;
	DataHandler& operator=(const DataHandler&) = delete;

	// WebSocket Utilities
	wsIncomingCommands getWebSocketCommand(const char* incomingCommand);
	JsonDocument prepData(wsOutGoingDataType type);
	void cleanUpClients(AsyncWebSocket* websocket);
	bool isValidUTF8(const char* data, size_t len);

	// Synchronization Primitives
	SemaphoreHandle_t websocketMutex;
	SemaphoreHandle_t clientListMutex;

	// Client Management
	std::set<int> connectedClients;

	// Flags
	bool _updateLedStatus = false;
	bool _blinkBlue = false;
	bool _blinkGreen = false;
	bool _blinkRed = false;
	bool _periodicSendRequest = false;

	// Data Processing
	ProcessingResult _result;
	// State Variables
	std::atomic<State> _currentState{State::DEVICE_ON};
	std::atomic<TestMode> _deviceMode{TestMode::MANUAL};
	std::atomic<int> _newClietId{0};
};

} // namespace Node_Core
#endif