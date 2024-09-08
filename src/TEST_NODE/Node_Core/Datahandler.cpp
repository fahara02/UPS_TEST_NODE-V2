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
	_updateLedStatus(false), _periodicSendRequest(false), _blinkBlue(false), _blinkGreen(false),
	_blinkRed(false), _result(ProcessingResult::PENDING)
{
	WebsocketDataQueue = xQueueCreate(10, sizeof(WebSocketMessage));
}

void DataHandler::init()
{
	xTaskCreatePinnedToCore(wsDataProcessor, "ProcessWsData", 8192, this, 4, &dataTaskHandler, 0);
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

		logger.log(LogLevel::INTR, "DATA Bit is set,,,high priority task");
		if((result & static_cast<EventBits_t>(wsClientStatus::DATA)) != 0)
		{
			if(xQueueReceive(instance->WebsocketDataQueue, &wsMsg, QUEUE_TIMEOUT_MS))
			{
				logger.log(LogLevel::INFO, "Processing WebSocket data in DataHandler task");
				instance->processWsMessage(wsMsg);

				EventHelper::clearBits(wsClientStatus::DATA);
			}
			else
			{
				logger.log(LogLevel::ERROR, "Queue timeout!!!!!");
			}
		}

		vTaskDelay(200 / portTICK_PERIOD_MS); // Add a delay to allow other tasks to run
	}

	vTaskDelete(NULL);
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
		_updateLedStatus = true;
	}
	else if(cmd == wsIncomingCommands::GET_READINGS)
	{
		_updateLedStatus = true;
		_periodicSendRequest = true;
		logger.log(LogLevel::INTR, "GET_READINGS command received, enabling periodic sending.");
	}
}

void DataHandler::periodicDataSender(void* pvParameter)
{
	PeriodicTaskParams* params = static_cast<PeriodicTaskParams*>(pvParameter);
	DataHandler& instance = DataHandler::getInstance();
	AsyncWebSocket* websocket = params->ws; // WebSocket instance from TestServer

	while(true)
	{
		if(instance._updateLedStatus)
		{
			EventBits_t eventBits =
				xEventGroupWaitBits(EventHelper::wsClientEventGroup,
									static_cast<EventBits_t>(wsClientStatus::CONNECTED), pdFALSE,
									pdFALSE, portMAX_DELAY);
			for(auto& client: websocket->getClients())
			{
				if(client.status() == WS_CONNECTED)
				{
					logger.log(LogLevel::INTR, "updating LED Status!!");
					instance.sendData(websocket, client.id(), wsOutGoingDataType::LED_STATUS);
					instance._updateLedStatus = false;
				}
			}
		}

		// Wait for the GET_READING bit, but don't clear it after sending
		EventBits_t eventBits = xEventGroupWaitBits(
			EventHelper::wsClientEventGroup, static_cast<EventBits_t>(wsClientUpdate::GET_READING),
			pdFALSE, pdFALSE, portMAX_DELAY);

		if(instance._periodicSendRequest)
		{
			for(auto& client: websocket->getClients())
			{
				if(client.status() == WS_CONNECTED)
				{
					logger.log(LogLevel::INFO, "delegating to send periodic data..");
					instance.sendData(websocket, client.id());
				}
			}

			// Continue sending every 3 seconds or as needed
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
		else
		{
			// Brief delay to avoid busy-waiting if periodic sending is disabled
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
	}
	vTaskDelete(NULL);
}
void DataHandler::sendData(AsyncWebSocket* websocket, int clientId, wsOutGoingDataType type)
{
	EventBits_t wsBits = xEventGroupGetBits(EventHelper::wsClientEventGroup);
	// EventBits_t cmdBits = xEventGroupGetBits(EventHelper::userCommandEventGroup);
	// EventBits_t systemInitEventbits = xEventGroupGetBits(EventHelper::systemInitEventGroup);
	TestSync& SyncTest = TestSync::getInstance();
	State state = SyncTest.refreshState();

	StaticJsonDocument<WS_BUFFER_SIZE> doc;

	if(type == wsOutGoingDataType::POWER_READINGS)
	{
		doc = prepData(wsOutGoingDataType::POWER_READINGS);
	}
	else if(type == wsOutGoingDataType::LED_STATUS)
	{ // start with all blink false
		state = SyncTest.refreshState();
		logger.log(LogLevel::INTR, "FOR LED STATUS STATE:%s ", stateToString(state));

		if(state == State::TEST_START)
		{
			_blinkRed = true;
		}
		else if(state != State::TEST_START)
		{
			_blinkRed = false;
		}
		else if(state == State::SYSTEM_PAUSED)
		{
			_blinkRed = false;
		}
		else if(state == State::DEVICE_SETUP)
		{
			_blinkGreen = true;
		}
		_blinkBlue = true;

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
		// logger.log(LogLevel::INFO, "Serialized JSON:%s ", jsonBuffer.data());
		logger.log(LogLevel::INFO, "Sending data--> ");

		if(isValidUTF8(jsonBuffer.data(), len))
		{
			if(wsBits & static_cast<EventBits_t>(wsClientStatus::CONNECTED))
			{
				websocket->text(clientId, jsonBuffer.data());
				logger.log(LogLevel ::SUCCESS, "a new data send...");
				serializeJsonPretty(doc, Serial);
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
	TestSync& SyncTest = TestSync::getInstance();
	DataHandler& instance = DataHandler::getInstance();
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