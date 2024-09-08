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

DataHandler::DataHandler() : _isReadingsRequested(false), _result(ProcessingResult::PENDING)
{
	WebsocketDataQueue = xQueueCreate(10, sizeof(WebSocketMessage));
	dequeMutex = xSemaphoreCreateMutex();
}

void DataHandler::init()
{
	fillDequeWithData();
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
		if((result & static_cast<EventBits_t>(wsClientStatus::DATA)) != 0)
		{
			if(instance->_isReadingsRequested)
			{
				instance->fillDequeWithData();
				instance->_isReadingsRequested = false;
			}

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
		_isReadingsRequested = true;
	}
}

void DataHandler::fillDequeWithData()
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
	_blankDoc["inputCurrent"] = randomCurrentInput;
	_blankDoc["outputCurrent"] = randomCurrentOutput;
	_blankDoc["inputVoltage"] = randomVoltageInput;
	_blankDoc["outputVoltage"] = randomVoltageOutput;
	_blankDoc["inputPowerFactor"] = randomPowerFactorInput;
	_blankDoc["outputPowerFactor"] = randomPowerFactorOutput;
	_blankDoc["inputWattage"] = randomWattageInput;
	_blankDoc["outputWattage"] = randomWattageOutput;

	if(xSemaphoreTake(dequeMutex, portMAX_DELAY))
	{
		if(wsDeque.size() < 10)
		{
			std::array<char, WS_BUFFER_SIZE> jsonBuffer;
			size_t len = serializeJson(_blankDoc, jsonBuffer.data(), jsonBuffer.size());

			if(len > 0 && len < jsonBuffer.size())
			{
				wsDeque.push_back(jsonBuffer);

				logger.log(LogLevel::INFO, "Added random data to wsDeque: ");
			}
		}
		xSemaphoreGive(dequeMutex);
	}

	_blankDoc.clear();
}

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

void DataHandler::prepWebSocketData(wsOutGoingDataType type, const char* data)
{
	logger.log(LogLevel::INFO, "currently doing nothing");
}

void DataHandler::sendRandomTestData()
{
	prepWebSocketData(wsOutGoingDataType::INPUT_POWER, String(random(1000, 2000)).c_str());
	prepWebSocketData(wsOutGoingDataType::INPUT_VOLT, String(random(200, 240)).c_str());
	prepWebSocketData(wsOutGoingDataType::INPUT_CURRENT, String(random(5, 10)).c_str());
	prepWebSocketData(wsOutGoingDataType::INPUT_PF, String(random(95, 100) / 100.0, 2).c_str());
	prepWebSocketData(wsOutGoingDataType::OUTPUT_POWER, String(random(900, 1800)).c_str());
	prepWebSocketData(wsOutGoingDataType::OUTPUT_VOLT, String(random(210, 230)).c_str());
	prepWebSocketData(wsOutGoingDataType::OUTPUT_CURRENT, String(random(4, 8)).c_str());
	prepWebSocketData(wsOutGoingDataType::OUTPUT_PF, String(random(90, 100) / 100.0, 2).c_str());
}

} // namespace Node_Core