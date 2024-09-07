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
	_isDataProcessing(false), _isProcessingDone(false), _result(ProcessingResult::PENDING)
{
	WebsocketDataQueue = xQueueCreate(10, sizeof(WebSocketMessage));
}

void DataHandler::init()
{
	const char* current = "43";
	_blankDoc["InputCurrent"] = current;
	xTaskCreatePinnedToCore(wsDataProcessor, "ProcessWsData", 16000, this, 1, &dataTaskHandler, 0);
}

void DataHandler::wsDataProcessor(void* pVparamter)
{
	DataHandler* instance = static_cast<DataHandler*>(pVparamter);
	WebSocketMessage wsMsg;

	while(true)
	{
		// instance->fillDequeWithData();

		if(xQueueReceive(instance->WebsocketDataQueue, &wsMsg, 100 / portTICK_PERIOD_MS))
		{
			logger.log(LogLevel::INFO, "Processing WebSocket data in DataHandler task");

			instance->processWsMessage(wsMsg);
		}

		// Add a delay to allow other tasks to run
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
}

void DataHandler::processWsMessage(WebSocketMessage& wsMsg)
{
	char* message = new char[wsMsg.len + 1];
	memcpy(message, wsMsg.data, wsMsg.len);
	message[wsMsg.len] = '\0';

	wsIncomingCommands cmd = getWebSocketCommand(message);
	if(cmd != wsIncomingCommands::INVALID_COMMAND && cmd != wsIncomingCommands::GET_READINGS)
	{
		handleWsIncomingCommands(cmd); // Process known commands
	}
	else if(cmd == wsIncomingCommands::GET_READINGS)
	{
		fillDequeWithData(); // Default handler
	}

	delete[] message; // Clean up
}

void DataHandler::fillDequeWithData()
{
	int randomCurrent = random(20, 100);
	char currentBuffer[4];
	snprintf(currentBuffer, sizeof(currentBuffer), "%d", randomCurrent);

	_blankDoc["InputCurrent"] = currentBuffer;

	if(wsDeque.size() < 10)
	{
		std::array<char, WS_BUFFER_SIZE> jsonBuffer;
		size_t len = serializeJson(_blankDoc, jsonBuffer.data(), jsonBuffer.size());

		if(len > 0 && len < jsonBuffer.size())
		{
			// Add serialized data to the deque
			wsDeque.push_back(jsonBuffer);

			logger.log(LogLevel::INFO, "Added random current data to wsDeque: ");
			logger.log(LogLevel::INFO, currentBuffer);
		}
	}
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