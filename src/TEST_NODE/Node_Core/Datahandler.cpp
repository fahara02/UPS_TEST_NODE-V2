#include "DataHandler.h"

using namespace Node_Core;
extern Logger& logger;
namespace Node_Core
{
DataHandler& DataHandler::getInstance()
{
	static DataHandler instance;
	return instance;
}

DataHandler::DataHandler() :
	_isReadingsRequested(false), _periodicFillRequest(false), _result(ProcessingResult::PENDING)
{
	WebsocketDataQueue = xQueueCreate(10, sizeof(WebSocketMessage));
	dataEventMutex = xSemaphoreCreateMutex();
	periodicMutex = xSemaphoreCreateMutex();
}

void DataHandler::init()
{
	fillDequeWithData();
	xTaskCreatePinnedToCore(wsDataProcessor, "ProcessWsData", 8192, this, 4, &dataTaskHandler, 0);
}

// void DataHandler::wsDataProcessor(void* pVparamter)
// {
// 	DataHandler* instance = static_cast<DataHandler*>(pVparamter);
// 	WebSocketMessage wsMsg;

// 	while(true)
// 	{
// 		int result = xEventGroupWaitBits(EventHelper::wsClientEventGroup,
// 										 static_cast<EventBits_t>(wsClientStatus::DATA), pdFALSE,
// 										 pdFALSE, portMAX_DELAY);
// 		if((result & static_cast<EventBits_t>(wsClientStatus::DATA)) != 0)
// 		{
// 			if(instance->_isReadingsRequested)
// 			{
// 				instance->fillDequeWithData();
// 				instance->_isReadingsRequested = false;
// 			}

// 			if(xQueueReceive(instance->WebsocketDataQueue, &wsMsg, QUEUE_TIMEOUT_MS))
// 			{
// 				logger.log(LogLevel::INFO, "Processing WebSocket data in DataHandler task");
// 				instance->processWsMessage(wsMsg);
// 			}

// 			EventHelper::clearBits(wsClientStatus::DATA);
// 		}

// 		vTaskDelay(200 / portTICK_PERIOD_MS); // Add a delay to allow other tasks to run
// 	}

// 	vTaskDelete(NULL);
// }

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

	if(cmd != wsIncomingCommands::INVALID_COMMAND && cmd != wsIncomingCommands::GET_READINGS)
	{
		handleWsIncomingCommands(cmd);
	}
	else if(cmd == wsIncomingCommands::GET_READINGS)
	{
		_periodicFillRequest = true;
	}
}

// void DataHandler::fillDequeWithData()
// {
// 	StaticJsonDocument<WS_BUFFER_SIZE> wsData = prepData(wsOutGoingDataType::POWER_READINGS);

// 	if(xSemaphoreTake(dequeMutex, portMAX_DELAY))
// 	{
// 		if(wsDeque.size() < 10)
// 		{
// 			std::array<char, WS_BUFFER_SIZE> jsonBuffer;
// 			size_t len = serializeJson(wsData, jsonBuffer.data(), jsonBuffer.size());

// 			if(len > 0 && len < jsonBuffer.size())
// 			{
// 				wsDeque.push_back(jsonBuffer);

// 				logger.log(LogLevel::INFO, "Added random data to wsDeque: ");
// 			}
// 		}
// 		xSemaphoreGive(dequeMutex);
// 	}

// 	wsData.clear();
// }

void DataHandler::handleWsIncomingCommands(wsIncomingCommands cmd)
{
	TestSync& SyncTest = TestSync::getInstance();
	if(cmd == wsIncomingCommands::TEST_START)
	{
		logger.log(LogLevel::SUCCESS, "handling TEST START EVENT");
		SyncTest.handleUserCommand(UserCommandEvent::START);
	}
	else if(cmd == wsIncomingCommands::TEST_STOP)
	{
		logger.log(LogLevel::SUCCESS, "handling TEST STOP EVENT");
		SyncTest.handleUserCommand(UserCommandEvent::STOP);
	}
	else if(cmd == wsIncomingCommands::TEST_PAUSE)
	{
		logger.log(LogLevel::SUCCESS, "handling TEST PAUSE EVENT");
		SyncTest.handleUserCommand(UserCommandEvent::PAUSE);
	}
	else if(cmd == wsIncomingCommands::AUTO_MODE)
	{
		logger.log(LogLevel::SUCCESS, "handling command set to AUTO ");
		SyncTest.handleUserCommand(UserCommandEvent::AUTO);
	}
	else if(cmd == wsIncomingCommands::MANUAL_MODE)
	{
		logger.log(LogLevel::SUCCESS, "handling command set to MANUAL ");
		SyncTest.handleUserCommand(UserCommandEvent::MANUAL);
	}
	else
	{
		logger.log(LogLevel::ERROR, "invalid command ");
	}
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

StaticJsonDocument<WS_BUFFER_SIZE> DataHandler::prepData(wsOutGoingDataType type)
{
	StaticJsonDocument<WS_BUFFER_SIZE> doc;
	if(type == wsOutGoingDataType::POWER_READINGS)
	{
		// Generate random values for different metrics
		int randomCurrentInput = random(20, 100);
		int randomCurrentOutput = random(20, 100);
		int randomVoltageInput = random(200, 240);
		int randomVoltageOutput = random(200, 240);
		float randomPowerFactorInput = random(80, 100) / 100.0;
		float randomPowerFactorOutput = random(80, 100) / 100.0;
		int randomWattageInput = random(1000, 5000);
		int randomWattageOutput = random(1000, 5000);

		// Populate the JSON document with these random values
		doc["inputCurrent"] = randomCurrentInput;
		doc["outputCurrent"] = randomCurrentOutput;
		doc["inputVoltage"] = randomVoltageInput;
		doc["outputVoltage"] = randomVoltageOutput;
		doc["inputPowerFactor"] = randomPowerFactorInput;
		doc["outputPowerFactor"] = randomPowerFactorOutput;
		doc["inputWattage"] = randomWattageInput;
		doc["outputWattage"] = randomWattageOutput;
	}
	else
	{
		logger.log(LogLevel::ERROR, "not implemented the logic yet");
	}

	return doc;
}

// void DataHandler::sendData(AsyncWebSocket* websocket, int clientId)
// {
// 	StaticJsonDocument<WS_BUFFER_SIZE> wsData = prepData(wsOutGoingDataType::POWER_READINGS);

// 	if(xSemaphoreTake(DataHandler::getInstance().dequeMutex, DEQUE_MUTEX_TIMEOUT_MS))
// 	{
// 		if(!DataHandler::getInstance().wsDeque.empty())
// 		{
// 			const std::array<char, WS_BUFFER_SIZE>& jsonData =
// 				DataHandler::getInstance().wsDeque.front();
// 			DataHandler::getInstance().wsDeque.pop_front();

// 			Serial.println("Sending Periodic data...->");
// 			websocket->text(clientId, jsonData.data());

// 			xSemaphoreGive(DataHandler::getInstance().dequeMutex);
// 			_periodicFillRequest = true;
// 		}
// 		else
// 		{
// 			_periodicFillRequest = true;
// 			xSemaphoreGive(DataHandler::getInstance().dequeMutex);
// 		}
// 	}
// }

// void DataHandler::periodicDataSender(void* pvParameter)
// {
// 	PeriodicTaskParams* params = static_cast<PeriodicTaskParams*>(pvParameter);
// 	DataHandler& instance = DataHandler::getInstance();
// 	AsyncWebSocket* websocket = params->ws; // WebSocket instance from TestServer

// 	while(true)
// 	{
// 		int result = xEventGroupWaitBits(EventHelper::wsClientEventGroup,
// 										 static_cast<EventBits_t>(wsClientStatus::CONNECTED),
// 										 pdFALSE, pdFALSE, portMAX_DELAY);
// 		// Send data to all connected clients periodically
// 		for(auto& client: websocket->getClients())
// 		{
// 			if(client.status() == WS_CONNECTED)
// 			{
// 				instance.sendData(websocket, client.id());
// 			}
// 		}

// 		if(instance._periodicFillRequest)
// 		{
// 			instance.fillDequeWithData();
// 			instance._periodicFillRequest = false;
// 		}

// 		vTaskDelay(3000 / portTICK_PERIOD_MS); // Send data every 3 seconds
// 	}
// }

void DataHandler::fillDequeWithData()
{
	StaticJsonDocument<WS_BUFFER_SIZE> wsData = prepData(wsOutGoingDataType::POWER_READINGS);
	if(xSemaphoreTake(dataEventMutex, portMAX_DELAY))
	{
		if(wsDeque.size() < 10)
		{
			std::array<char, WS_BUFFER_SIZE> jsonBuffer;
			size_t len = serializeJson(wsData, jsonBuffer.data(), jsonBuffer.size());

			if(len > 0 && len < jsonBuffer.size())
			{
				wsDeque.push_back(jsonBuffer);
				logger.log(LogLevel::INFO, "Added random data to wsDeque: ");

				xSemaphoreGive(dataEventMutex);
			}
		}

		else
		{
			logger.log(LogLevel::WARNING, "deque is filled");
		}
	}
	wsData.clear();
}

void DataHandler::fillPeriodicDeque()
{
	StaticJsonDocument<WS_BUFFER_SIZE> wsData = prepData(wsOutGoingDataType::POWER_READINGS);
	if(xSemaphoreTake(periodicMutex, portMAX_DELAY))
	{
		if(wsDequePeriodic.size() < 10)
		{
			std::array<char, WS_BUFFER_SIZE> jsonBuffer;
			size_t len = serializeJson(wsData, jsonBuffer.data(), jsonBuffer.size());

			if(len > 0 && len < jsonBuffer.size())
			{
				wsDequePeriodic.push_back(jsonBuffer);
				logger.log(LogLevel::INFO, "Added random data to wsDequePeriodic: ");

				xSemaphoreGive(periodicMutex);
			}
		}

		else
		{
			logger.log(LogLevel::WARNING, "wsDequePeriodic is filled");
		}
	}
	wsData.clear();
}

void DataHandler::periodicDataSender(void* pvParameter)
{
	PeriodicTaskParams* params = static_cast<PeriodicTaskParams*>(pvParameter);
	DataHandler& instance = DataHandler::getInstance();
	AsyncWebSocket* websocket = params->ws; // WebSocket instance from TestServer

	while(true)
	{
		// Wait for connection status
		xEventGroupWaitBits(EventHelper::wsClientEventGroup,
							static_cast<EventBits_t>(wsClientStatus::CONNECTED), pdFALSE, pdFALSE,
							portMAX_DELAY);

		if(instance._periodicFillRequest)
		{
			instance._periodicFillRequest = false; // Reset the request flag
			instance.fillPeriodicDeque();
		}

		xEventGroupWaitBits(EventHelper::wsClientEventGroup,
							static_cast<EventBits_t>(wsClientUpdate::GET_READING), pdFALSE, pdFALSE,
							portMAX_DELAY);

		for(auto& client: websocket->getClients())
		{
			if(client.status() == WS_CONNECTED)
			{
				instance.sendData(websocket, client.id());
			}
		}

		vTaskDelay(5000 / portTICK_PERIOD_MS); // Send data every 3 seconds
	}
}

void DataHandler::wsDataProcessor(void* pVparamter)
{
	DataHandler* instance = static_cast<DataHandler*>(pVparamter);
	WebSocketMessage wsMsg;

	while(true)
	{
		int result = xEventGroupWaitBits(EventHelper::wsClientEventGroup,
										 static_cast<EventBits_t>(wsClientStatus::DATA), pdFALSE,
										 pdFALSE, portMAX_DELAY);
		if((result & static_cast<EventBits_t>(wsClientStatus::DATA)) != 0)
		{
			if(xQueueReceive(instance->WebsocketDataQueue, &wsMsg, QUEUE_TIMEOUT_MS))
			{
				logger.log(LogLevel::INFO, "Processing WebSocket data in DataHandler task");
				instance->processWsMessage(wsMsg);
			}

			EventHelper::clearBits(wsClientStatus::DATA);
		}

		vTaskDelay(200 / portTICK_PERIOD_MS); // Add a delay to allow other tasks to run
	}

	vTaskDelete(NULL);
}

void DataHandler::sendData(AsyncWebSocket* websocket, int clientId)
{
	if(xSemaphoreTake(periodicMutex, portMAX_DELAY))
	{
		if(!wsDeque.empty())
		{
			const std::array<char, WS_BUFFER_SIZE>& jsonData = wsDeque.front();
			wsDeque.pop_front();
			_periodicFillRequest = true;
			xSemaphoreGive(periodicMutex);

			// Send data to the client
			websocket->text(clientId, jsonData.data());
		}
		else
		{
			xSemaphoreGive(periodicMutex);
		}
	}
	else
	{
		logger.log(LogLevel::WARNING, "Failed to acquire consumer mutex; cannot send data.");
	}
}

} // namespace Node_Core