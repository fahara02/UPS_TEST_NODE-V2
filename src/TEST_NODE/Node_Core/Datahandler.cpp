#include "DataHandler.h"
#include "HPTSettings.h"
#include "PZEM_Modbus.hpp"
#include "NodeUtility.hpp"

using namespace Node_Core;
using namespace Node_Utility;
extern Logger& logger;
extern Node_Utility::ModbusManager& MBManager;
extern TaskHandle_t PeriodicDataHandle;
namespace Node_Core
{
DataHandler& DataHandler::getInstance()
{
	static DataHandler instance;
	return instance;
}

DataHandler::DataHandler() :
	_updateLedStatus(false), _blinkBlue(false), _blinkGreen(false), _blinkRed(false),
	_periodicSendRequest(false), _result(ProcessingResult::PENDING),
	_currentState(State::DEVICE_ON), _deviceMode(TestMode::MANUAL), _newClietId(0)
{
	WebsocketDataQueue = xQueueCreate(WS_QUEUE_SIZE, sizeof(WebSocketMessage));
	websocketMutex = xSemaphoreCreateMutex();
	clientListMutex = xSemaphoreCreateMutex();
}
void DataHandler::updateState(State state)
{
	_currentState.store(state);
}
void DataHandler::updateMode(TestMode mode)
{
	_deviceMode.store(mode);
}
void DataHandler::updateNewClientId(int Id)
{
	_newClietId.store(Id);
}
void DataHandler::wsDataHandler(void* pvParameter)
{
	WsDataHandlerTaskParams* params = static_cast<WsDataHandlerTaskParams*>(pvParameter);
	DataHandler& instance = DataHandler::getInstance();
	AsyncWebSocket* websocket = params->ws;
	TickType_t lastWakeTime = xTaskGetTickCount();
	WebSocketMessage wsMsg;
	uint32_t ulNotificationValue;

	while(true)
	{
		// Process WebSocket Data
		if(xEventGroupWaitBits(EventHelper::wsClientEventGroup,
							   static_cast<EventBits_t>(wsClientStatus::DATA), pdFALSE, pdFALSE, 0))
		{
			logger.log(LogLevel::INTR, "DATA Bit is set,,,processing WebSocket data");

			while(uxQueueMessagesWaiting(instance.WebsocketDataQueue) > 0)
			{
				if(xQueueReceive(instance.WebsocketDataQueue, &wsMsg, QUEUE_TIMEOUT_MS))
				{
					logger.log(LogLevel::INFO, "Processing WebSocket data in combined task");
					instance.processWsMessage(wsMsg);
				}
				else
				{
					logger.log(LogLevel::ERROR, "Queue timeout while processing WebSocket data!");
				}
			}
		}

		// Handle Periodic Data Sending
		if(xEventGroupWaitBits(EventHelper::wsClientEventGroup,
							   static_cast<EventBits_t>(wsClientUpdate::GET_READING), pdFALSE,
							   pdFALSE, 0))
		{
			std::set<int> clientsToCheck = instance.connectedClients;
			for(int clientId: clientsToCheck)
			{
				AsyncWebSocketClient* client = websocket->client(clientId);
				if(client != nullptr && client->status() == WS_CONNECTED)
				{
					instance.sendData(websocket, clientId);
				}
				else
				{
					logger.log(LogLevel::ERROR,
							   "Client object for ID %d is nullptr or disconnected", clientId);
					instance.updateClientList(clientId, false);
				}
			}
		}

		// Handle Task Notifications
		if(xTaskNotifyWait(0x00, 0xFFFFFFFF, &ulNotificationValue, 0) == pdTRUE)
		{
			switch(static_cast<UserUpdateEvent>(ulNotificationValue))
			{
				case UserUpdateEvent::NEW_TEST:
					logger.log(LogLevel::SUCCESS, "Sending LED STATUS from combined task");
					for(int clientId: instance.connectedClients)
					{
						instance.sendData(websocket, clientId, wsOutGoingDataType::LED_STATUS);
					}
					break;
				case UserUpdateEvent::DELETE_TEST:
					// Add logic for DELETE_TEST event
					break;
				default:
					logger.log(LogLevel::INFO, "Unhandled notification event: %d",
							   ulNotificationValue);
					break;
			}
		}

		// Maintain periodic delay
		vTaskDelayUntil(&lastWakeTime, 1000 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
}

void DataHandler::init()
{
	// xTaskCreatePinnedToCore(wsDataProcessor, "ProcessWsData", wsDataProcessor_Stack, this,
	// 						wsDataProcessor_Priority, &dataTaskHandler, wsDataProcessor_CORE);
}

void DataHandler::updateClientList(int clientId, bool connected)
{
	if(xSemaphoreTake(clientListMutex, portMAX_DELAY) == pdTRUE)
	{
		if(connected)
		{
			connectedClients.insert(clientId);
		}
		else
		{
			connectedClients.erase(clientId);
		}
		xSemaphoreGive(clientListMutex);
	}
}

void DataHandler::processWsMessage(WebSocketMessage& wsMsg)
{
	char message[WS_BUFFER_SIZE];

	if(wsMsg.len >= sizeof(message))
	{
		Serial.println("Received data exceeds MAX BUFFER SIZE");
		return;
	}

	memcpy(message, wsMsg.data, wsMsg.len);
	message[wsMsg.len] = '\0';

	wsIncomingCommands cmd = getWebSocketCommand(message);
	logger.log(LogLevel::INFO, "processing Message: %s", message);

	if(cmd != wsIncomingCommands::INVALID_COMMAND && cmd != wsIncomingCommands::GET_READINGS)
	{
		handleWsIncomingCommands(cmd);

		AsyncWebSocketClient* client = wsMsg.client;
		if(client == nullptr || (client->status() != AwsClientStatus::WS_CONNECTED))
		{
			logger.log(LogLevel::ERROR,
					   " not connected or is null, aborting from processMsg function.");
		}
		else
		{
			sendData(client);
		}
	}
	else if(cmd == wsIncomingCommands::GET_READINGS)
	{
		logger.log(LogLevel::INTR, "GET_READINGS command received, enabling periodic sending.");
	}
}

void DataHandler::sendData(AsyncWebSocket* websocket, int clientId, wsOutGoingDataType type)
{
	JsonDocument doc;

	// Prepare JSON data
	if(type == wsOutGoingDataType::POWER_READINGS)
	{
		doc = prepData(wsOutGoingDataType::POWER_READINGS);
	}
	else if(type == wsOutGoingDataType::LED_STATUS)
	{
		logger.log(LogLevel::INTR, "CURRENT  STATE FOR LED %s ",
				   Node_Utility::ToString::state(_currentState));

		if(_currentState == State::READY_TO_PROCEED)
		{
			_blinkGreen = true;
		}

		doc["type"] = "LED_STATUS";
		doc["blinkBlue"] = _blinkBlue;
		doc["blinkGreen"] = _blinkGreen;
		doc["blinkRed"] = _blinkRed;
	}
	else
	{
		logger.log(LogLevel::ERROR, "Not implemented yet");
		return;
	}

	std::array<char, WS_BUFFER_SIZE> jsonBuffer;
	size_t len = serializeJson(doc, jsonBuffer.data(), jsonBuffer.size());

	if(len == 0)
	{
		logger.log(LogLevel::ERROR, "Serialization failed: Empty JSON.");
		return;
	}
	else if(len >= jsonBuffer.size())
	{
		logger.log(LogLevel::ERROR, "Serialization failed: Buffer overflow.");
		return;
	}
	else if(!isValidUTF8(jsonBuffer.data(), len))
	{
		logger.log(LogLevel::ERROR, "Invalid UTF-8 data detected.");
		return;
	}

	if(xSemaphoreTake(websocketMutex, portMAX_DELAY) == pdTRUE)
	{
		AsyncWebSocketClient* client = websocket->client(clientId);
		if(client == nullptr || (client->status() != AwsClientStatus::WS_CONNECTED))
		{
			logger.log(LogLevel::ERROR, "Client is not connected or is null, aborting send.");
		}
		else if(client->status() == WS_CONNECTED)
		{
			websocket->text(clientId, jsonBuffer.data());
			logger.log(LogLevel::SUCCESS, "Data sent successfully to client %d.", clientId);
		}
		else
		{
			logger.log(LogLevel::ERROR, "Client %d is no longer connected.", clientId);
		}
		xSemaphoreGive(websocketMutex); // Ensure the mutex is released
	}
	else
	{
		logger.log(LogLevel::ERROR, "Failed to take WebSocket mutex.");
	}
}

void DataHandler::sendData(AsyncWebSocketClient* client, wsOutGoingDataType type)
{
	JsonDocument doc;

	if(type == wsOutGoingDataType::POWER_READINGS)
	{
		doc = prepData(wsOutGoingDataType::POWER_READINGS);
	}
	else if(type == wsOutGoingDataType::LED_STATUS)
	{ // start with all blink false

		logger.log(LogLevel::INTR, "FOR LED STATUS STATE:%s ",
				   Node_Utility::ToString::state(_currentState));
		if(_currentState == State::READY_TO_PROCEED)
		{
			_blinkBlue = true;
		}
		doc["type"] = "LED_STATUS";
		doc["blinkBlue"] = _blinkBlue;
		doc["blinkGreen"] = _blinkGreen;
		doc["blinkRed"] = _blinkRed;
	}
	else
	{
		logger.log(LogLevel::ERROR, "not implemented yet");
		return;
	}

	std::array<char, WS_BUFFER_SIZE> jsonBuffer;
	size_t len = serializeJson(doc, jsonBuffer.data(), jsonBuffer.size());

	if(len == 0)
	{
		logger.log(LogLevel::ERROR, "Serialization failed: Empty JSON.");
	}
	else if(len >= jsonBuffer.size())
	{
		logger.log(LogLevel::ERROR, "Serialization failed: Buffer overflow.");
	}
	else
	{
		logger.log(LogLevel::INFO, "Sending data--> ");

		if(isValidUTF8(jsonBuffer.data(), len))
		{
			if(xSemaphoreTake(websocketMutex, portMAX_DELAY) == pdTRUE)
			{
				if(client == nullptr || (client->status() != AwsClientStatus::WS_CONNECTED))
				{
					logger.log(LogLevel::ERROR,
							   "Client is not connected or is null, aborting send.");
				}
				else if(client->status() == AwsClientStatus::WS_CONNECTED)
				{
					client->text(jsonBuffer.data(), len);
					logger.log(LogLevel ::INTR, "SEND LED STATUS ");
					serializeJsonPretty(doc, Serial);
				}
				else
				{
					logger.log(LogLevel::ERROR, "recent Client  is no longer connected.");
				}
				xSemaphoreGive(websocketMutex);
			}
		}
		else
		{
			logger.log(LogLevel::ERROR, "Invalid UTF-8 data detected.");
		}
	}
}

void DataHandler::handleWsIncomingCommands(wsIncomingCommands cmd)
{
	if(cmd == wsIncomingCommands::TEST_START)
	{
		_blinkRed = true;
		logger.log(LogLevel::SUCCESS, "handling TEST START EVENT");
		handleUserCommand(UserCommandEvent::START);
	}
	else if(cmd == wsIncomingCommands::TEST_STOP)
	{
		_blinkRed = false;
		logger.log(LogLevel::SUCCESS, "handling TEST STOP EVENT");
		handleUserCommand(UserCommandEvent::STOP);
	}
	else if(cmd == wsIncomingCommands::TEST_PAUSE)
	{
		_blinkRed = false;
		logger.log(LogLevel::SUCCESS, "handling TEST PAUSE EVENT");
		handleUserCommand(UserCommandEvent::PAUSE);
	}
	else if(cmd == wsIncomingCommands::AUTO_MODE)
	{
		_blinkBlue = true;
		handleUserCommand(UserCommandEvent::AUTO);
		logger.log(LogLevel::SUCCESS, "handling command set to AUTO ");
	}
	else if(cmd == wsIncomingCommands::MANUAL_MODE)
	{
		_blinkBlue = false;
		handleUserCommand(UserCommandEvent::MANUAL);
		logger.log(LogLevel::SUCCESS, "handling command set to MANUAL ");
	}
	else
	{
		logger.log(LogLevel::ERROR, "invalid command ");
	}
}

void DataHandler::handleUserCommand(UserCommandEvent command)
{
	EventBits_t commandBits = static_cast<EventBits_t>(command);
	switch(command)
	{
		case UserCommandEvent ::PAUSE:
			EventHelper::clearBits(UserCommandEvent::RESUME);
			break;
		case UserCommandEvent ::RESUME:
			EventHelper::clearBits(UserCommandEvent::PAUSE);
			break;
		case UserCommandEvent ::AUTO:
			EventHelper::clearBits(UserCommandEvent::MANUAL);

			break;
		case UserCommandEvent ::MANUAL:
			EventHelper::clearBits(UserCommandEvent::AUTO);

			break;
		case UserCommandEvent ::START:
			EventHelper::clearBits(UserCommandEvent::STOP);
			break;
		case UserCommandEvent ::STOP:
			EventHelper::clearBits(UserCommandEvent::START);
			break;
		default:
			break;
	}
	xEventGroupSetBits(EventHelper::userCommandEventGroup, commandBits);
}

JsonDocument DataHandler::prepData(wsOutGoingDataType type)
{
	JsonDocument doc;
	if(type == wsOutGoingDataType::POWER_READINGS)
	{
		// Populate the JSON document with these random values
		doc["inputCurrent"] = MBManager.getInputPower().current;
		doc["outputCurrent"] = MBManager.getoutputPower().current;
		doc["inputVoltage"] = MBManager.getInputPower().voltage;
		doc["outputVoltage"] = MBManager.getoutputPower().voltage;
		;
		doc["inputPowerFactor"] = MBManager.getInputPower().powerfactor;
		doc["outputPowerFactor"] = MBManager.getoutputPower().powerfactor;
		doc["inputWattage"] = MBManager.getInputPower().power;
		doc["outputWattage"] = MBManager.getoutputPower().power;
	}
	else
	{
		logger.log(LogLevel::ERROR, "not implemented the logic yet");
	}

	return doc;
}

wsIncomingCommands DataHandler::getWebSocketCommand(const char* incomingCommand)
{
	if(strcmp(incomingCommand, "start") == 0)

		return wsIncomingCommands::TEST_START;
	if(strcmp(incomingCommand, "stop") == 0)
		return wsIncomingCommands::TEST_STOP;
	if(strcmp(incomingCommand, "pause") == 0)
		return wsIncomingCommands::TEST_PAUSE;
	if(strcmp(incomingCommand, "AUTO") == 0)
		return wsIncomingCommands::AUTO_MODE;
	if(strcmp(incomingCommand, "MANUAL") == 0)
		return wsIncomingCommands::MANUAL_MODE;
	if(strcmp(incomingCommand, "Load On") == 0)
		return wsIncomingCommands::LOAD_ON;
	if(strcmp(incomingCommand, "Load Off") == 0)
		return wsIncomingCommands::LOAD_OFF;
	if(strcmp(incomingCommand, "Mains On") == 0)
		return wsIncomingCommands::MAINS_ON;
	if(strcmp(incomingCommand, "Mains Off") == 0)
		return wsIncomingCommands::MAINS_OFF;
	if(strcmp(incomingCommand, "getReadings") == 0)
		return wsIncomingCommands::GET_READINGS;
	return wsIncomingCommands::INVALID_COMMAND;
}
bool DataHandler::isValidUTF8(const char* data, size_t len)
{
	for(size_t i = 0; i < len; i++)
	{
		unsigned char c = static_cast<unsigned char>(data[i]);
		if(c <= 0x7F)
			continue; // ASCII
		if((c >> 5) == 0x6)
			i++; // 2-byte sequence
		else if((c >> 4) == 0xE)
			i += 2; // 3-byte sequence
		else if((c >> 3) == 0x1E)
			i += 3; // 4-byte sequence
		else
			return false; // Invalid UTF-8
	}
	return true;
}

// void DataHandler::fillPeriodicDeque()
// {
// 	StaticJsonDocument<WS_BUFFER_SIZE> wsData = prepData(wsOutGoingDataType::POWER_READINGS);

// 	if(xSemaphoreTake(periodicMutex, portMAX_DELAY))
// 	{
// 		// logger.log(LogLevel::INFO, "MUTEX taken to fill...");

// 		if(wsDequePeriodic.size() < 10)
// 		{
// 			// Buffer to hold the serialized JSON data
// 			std::array<char, WS_BUFFER_SIZE> jsonBuffer;
// 			size_t len = serializeJson(wsData, jsonBuffer.data(), jsonBuffer.size());

// 			// Check if the data was serialized successfully
// 			if(len == 0)
// 			{
// 				logger.log(LogLevel::ERROR, "Serialization failed: Empty JSON.");
// 			}
// 			else if(len >= jsonBuffer.size())
// 			{
// 				logger.log(LogLevel::ERROR, "Serialization failed: Buffer overflow.");
// 			}
// 			else
// 			{
// 				// Log the serialized JSON data for debugging
// 				logger.log(LogLevel::INFO, "Serialized JSON:%s ", jsonBuffer.data());

// 				// Check if the serialized data is valid UTF-8
// 				if(isValidUTF8(jsonBuffer.data(), len))
// 				{
// 					// Push the serialized data into the deque
// 					wsDequePeriodic.push_back(jsonBuffer);
// 					logger.log(LogLevel::INFO, "Added data to wsDequePeriodic.");
// 				}
// 				else
// 				{
// 					logger.log(LogLevel::ERROR, "Invalid UTF-8 data detected.");
// 				}
// 			}
// 		}
// 		else
// 		{
// 			logger.log(LogLevel::ERROR, "Deque is full, cannot add more data.");
// 		}

// 		// Release the mutex
// 		logger.log(LogLevel::INFO, "MUTEX releasing from fill...");
// 		xSemaphoreGive(periodicMutex);
// 	}
// 	else
// 	{
// 		logger.log(LogLevel::WARNING, "Failed to acquire MUTEX for filling deque.");
// 	}

// 	// Clear the JSON document
// 	wsData.clear();
// }

} // namespace Node_Core