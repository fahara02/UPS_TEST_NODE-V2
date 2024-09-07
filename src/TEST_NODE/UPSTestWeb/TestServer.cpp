#include "TestServer.h"
#include "AsyncJson.h"
#include <map>

// Other includes as necessary

#define ETAG "\"" __DATE__ " " __TIME__ "\""

void TestServer::servePages(UPSTesterSetup& _setup, TestSync& _sync)
{
	// Handle individual GET requests
	_server->on("/", HTTP_GET, [this, &_setup](AsyncWebServerRequest* request) {
		this->handleRootRequest(request);
	});

	_server->on("/dashboard", HTTP_GET, [this, &_setup](AsyncWebServerRequest* request) {
		this->handleDashboardRequest(request);
	});
	_server->on("/log", HTTP_GET, [this](AsyncWebServerRequest* request) {
		this->handleLogRequest(request);
	});

	_server->on("/settings/ups-specification", HTTP_GET,
				[this, &_setup](AsyncWebServerRequest* request) {
					this->handleSettingRequest(request, _setup, "UPS_SPECIFICATION",
											   SettingType::SPEC, "/settings/ups-specification");
				});

	_server->on("/settings/test-specification", HTTP_GET,
				[this, &_setup](AsyncWebServerRequest* request) {
					this->handleSettingRequest(request, _setup, "TEST_SPECIFICATION",
											   SettingType::TEST, "/settings/test-specification");
				});

	_server->on("/settings/ups-specification", HTTP_POST,
				[this, &_setup](AsyncWebServerRequest* request) {
					this->handleUpdateSettingRequest(request, _setup, SettingType::SPEC);
				});
	_server->on("/settings/test-specification", HTTP_POST,
				[this, &_setup](AsyncWebServerRequest* request) {
					this->handleUpdateSettingRequest(request, _setup, SettingType::TEST);
				});

	// Handle individual POST requests
	_server->on("/updateMode", HTTP_POST, [this, &_setup, &_sync](AsyncWebServerRequest* request) {
		this->handleUpdateModeRequest(request, _setup, _sync);
	});

	_server->on("/updateCommand", HTTP_POST,
				[this, &_setup, &_sync](AsyncWebServerRequest* request) {
					this->handleUserCommandRequest(request, _setup, _sync);
				});

	auto* testDataHandler = new AsyncCallbackJsonWebHandler(
		"/updateTestData", [this, &_sync](AsyncWebServerRequest* request, JsonVariant& json) {
			this->handleTestDataRequest(request, json, _sync);
		});
	_server->addHandler(testDataHandler);
	_server->addHandler(_ws);

	// Handle 404 errors
	_server->onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "text/plain", "404 Not Found");
	});
}

void TestServer::handleRootRequest(AsyncWebServerRequest* request)
{
	auto* response = request->beginResponseStream("text/html");
	this->webPage->sendHtmlHead(response);
	this->webPage->sendHeadTrailer(response);
	this->webPage->sendHeader(response);
	this->webPage->sendNavbar(response);
	this->webPage->sendSidebar(response);
	this->webPage->sendUserCommand(response);
	this->webPage->sendPowerMonitor(response);

	this->webPage->sendPageTrailer(response);

	request->send(response);
}

void TestServer::handleDashboardRequest(AsyncWebServerRequest* request)
{
	this->handleRootRequest(request);
}
void TestServer::handleLogRequest(AsyncWebServerRequest* request)
{
	String logs = logger.getBufferedLogs();
	request->send(200, "text/plain", logs.length() > 0 ? logs : "No logs available.");
}
void TestServer::handleSettingRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
									  const char* caption, SettingType type,
									  const char* redirect_uri)
{
	auto* response = request->beginResponseStream("text/html");
	this->webPage->sendHtmlHead(response);
	this->webPage->sendHeadTrailer(response);
	this->webPage->sendHeader(response);
	this->webPage->sendNavbar(response);
	this->webPage->sendSettingTable(response, _setup, caption, type, redirect_uri);
	this->webPage->sendPageTrailer(response);
	request->send(response);
}

void TestServer::handleUpdateModeRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
										 TestSync& _sync)
{
	if(!request->hasParam("body", true))
	{
		request->send(400, "text/plain", "Invalid request: No body found.");
		return;
	}

	String value = request->getParam("body", true)->value();
	request->send(200, "text/plain", "Mode received: " + value);
}

void TestServer::handleUserCommandRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
										  TestSync& _sync)
{
	if(!request->hasParam("body", true))
	{
		request->send(400, "text/plain", "Invalid request: No body found.");
		return;
	}

	String value = request->getParam("body", true)->value();
	request->send(200, "text/plain", "Command received: " + value);
}

void TestServer::handleTestDataRequest(AsyncWebServerRequest* request, JsonVariant& json,
									   TestSync& _sync)
{
	Serial.println("Received JSON Data:");
	serializeJsonPretty(json, Serial);

	if(json.is<JsonArray>())
	{
		JsonArray jsonArray = json.as<JsonArray>();

		for(JsonVariant value: jsonArray)
		{
			if(value.is<JsonObject>())
			{
				JsonObject jsonObj = value.as<JsonObject>();

				if(jsonObj.containsKey("testName") && jsonObj.containsKey("loadLevel"))
				{
					Serial.print("testName: ");
					Serial.println(jsonObj["testName"].as<String>());
					Serial.print("loadLevel: ");
					Serial.println(jsonObj["loadLevel"].as<String>());
					_sync.parseIncomingJson(jsonObj);
				}
				else
				{
					Serial.println("Error: Required fields missing in JSON.");
				}
			}
			else
			{
				Serial.println("Error: Expected JSON objects in array.");
			}
		}
		request->send(200, "application/json", "{\"status\":\"success\"}");
	}
	else
	{
		Serial.println("Error: Invalid JSON format.");
		request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
	}
}

void TestServer::handleUpdateSettingRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
											SettingType type)
{
	String responseMessage;
	bool success = false;

	if(type == SettingType::SPEC)
	{
		SetupSpec& spec = _setup.specSetup();

		// Array of pairs for SetupSpec fields
		const struct
		{
			const char* fieldName;
			SetupSpec::Field field;
		} specFields[] = {{"Rating_va", SetupSpec::Field::RatingVa},
						  {"RatedVoltage_volt", SetupSpec::Field::RatedVoltage},
						  {"RatedCurrent_amp", SetupSpec::Field::RatedCurrent},
						  {"MinInputVoltage_volt", SetupSpec::Field::MinInputVoltage},
						  {"MaxInputVoltage_volt", SetupSpec::Field::MaxInputVoltage},
						  {"AvgSwitchTime_ms", SetupSpec::Field::AvgSwitchTime},
						  {"AvgBackupTime_ms", SetupSpec::Field::AvgBackupTime}};

		for(const auto& field: specFields)
		{
			if(request->hasParam(field.fieldName, true))
			{
				String paramValue = request->getParam(field.fieldName, true)->value();
				success = spec.setField(field.field, paramValue.toDouble());
				if(success)
				{
					responseMessage +=
						"Spec Settings " + String(field.fieldName) + " updated successfully.<br>";
				}
				else
				{
					responseMessage +=
						"Error: " + String(field.fieldName) + " set field rejected the param.<br>";
				}
			}
		}
	}
	else if(type == SettingType::TEST)
	{
		SetupTest& test = _setup.testSetup();

		// Array of pairs for SetupTest fields
		const struct
		{
			const char* fieldName;
			SetupTest::Field field;
		} testFields[] = {{"TestStandard", SetupTest::Field::TestStandard},
						  {"TestMode", SetupTest::Field::Mode},
						  {"TestVARating", SetupTest::Field::TestVARating},
						  {"InputVoltage_volt", SetupTest::Field::InputVoltage},
						  {"TestDuration_ms", SetupTest::Field::TestDuration},
						  {"MinValidSwitchTime", SetupTest::Field::MinValidSwitchTime},
						  {"MaxValidSwitchTime", SetupTest::Field::MaxValidSwitchTime},
						  {"MinValidBackupTime", SetupTest::Field::MinValidBackupTime},
						  {"MaxValidBackupTime", SetupTest::Field::MaxValidBackupTime},
						  {"ToleranceSwitchTime", SetupTest::Field::ToleranceSwitchTime},
						  {"MaxBackupTime", SetupTest::Field::MaxBackupTime},
						  {"ToleranceBackupTime", SetupTest::Field::ToleranceBackupTime},
						  {"MaxRetest", SetupTest::Field::MaxRetest}};

		for(const auto& field: testFields)
		{
			if(request->hasParam(field.fieldName, true))
			{
				String paramValue = request->getParam(field.fieldName, true)->value();
				success = test.setField(field.field, paramValue.toDouble());
				if(success)
				{
					responseMessage +=
						"Test Settings " + String(field.fieldName) + " updated successfully.<br>";
				}
				else
				{
					responseMessage +=
						"Error: " + String(field.fieldName) + " set field rejected the param.<br>";
				}
			}
		}
	}

	// Send response based on the outcome
	if(responseMessage.isEmpty())
	{
		request->send(400, "text/html", "No valid settings updated.");
	}
	else
	{
		request->send(200, "text/html", responseMessage);
	}
}

// void TestServer::initWebSocket()
// {
// 	_ws.onEvent(onWsEvent);
// 	_server.addHandler(&ws);
// }
void TestServer::initWebSocket()
{
	_ws->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
						void* arg, uint8_t* data, size_t len) {
		this->onWsEvent(server, client, type, arg, data, len);
	});
	//_ws.onEvent(this->onWsEvent);
	_server->addHandler(_ws);
}

void TestServer::createServerTask()
{
	xTaskCreatePinnedToCore(wsClientCleanup, "WSCleanupTask", 4096, _ws, 5, NULL, 0);
	// xTaskCreatePinnedToCore(wsDataUpdate, "WSDataUpdate", 16000, this, 1, NULL, 0);
}
void TestServer::wsClientCleanup(void* pvParameters)
{
	AsyncWebSocket* ws = static_cast<AsyncWebSocket*>(pvParameters);
	while(true)
	{
		ws->cleanupClients();
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}
void TestServer::wsDataUpdate(void* pvParameters)
{
	TestServer* server = static_cast<TestServer*>(pvParameters);

	while(true)
	{
		// server->prepWebSocketData(wsOutGoingDataType::INPUT_CURRENT, String(random(5,
		// 10)).c_str());
		//  server->prepWebSocketData(wsOutGoingDataType::INPUT_VOLT, String(random(200,
		//  240)).c_str()); server->prepWebSocketData(wsOutGoingDataType::INPUT_POWER,
		//  						  String(random(1000, 2000)).c_str());
		//  server->prepWebSocketData(wsOutGoingDataType::INPUT_PF,
		//  						  String(random(95, 100) / 100.0, 2).c_str());
		if(server->_sendData)
		{
			if(server->isClientConnected)
			{
				server->_sendData = false;
				logger.log(LogLevel::WARNING, "Attempting sending data");
				const char* current = "43";
				StaticJsonDocument<200> doc;

				// Prepare JSON response
				doc["InputCurrent"] = current; // Add key-value pair
				String jsonData;
				serializeJson(doc, jsonData); // Serialize JSON document

				// For debugging purposes (Serial output)
				serializeJsonPretty(doc, Serial);

				logger.log(LogLevel::WARNING, "Alert: Checking if client still connected...");
				if(server->isClientConnected)
				{
					if(server->_lastClientId >= 0)
					{
						logger.log(LogLevel::WARNING, "Alert: Sending data for client %d",
								   server->_lastClientId);
						// server->_ws->text(server->_lastClientId, jsonData);
					}
				}
				else
				{
					logger.log(LogLevel::ERROR, "client not ready");
				}
			}
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL); // Clean up task when done
}

void TestServer::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
						   void* arg, uint8_t* data, size_t len)
{
	switch(type)
	{
		case WS_EVT_CONNECT:
			Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
						  client->remoteIP().toString().c_str());
			isClientConnected = true; // Set flag to true when client connects
			break;

		case WS_EVT_DISCONNECT:
			Serial.printf("WebSocket client #%u disconnected\n", client->id());
			isClientConnected = false; // Set flag to false when client disconnects
			break;

		case WS_EVT_DATA:
		{
			String msg = String((char*)data).substring(0, len);
			Serial.printf("Message from client: %s\n", msg.c_str());

			// Respond back to the client
			client->text("Received: " + msg);

			// Optionally handle the WebSocket message
			// handleWebSocketMessage(arg, data, len);
			break;
		}

		case WS_EVT_PONG:
			Serial.println("PONG received");
			break;

		case WS_EVT_ERROR:
			Serial.println("WebSocket error occurred");
			break;
	}
}

void TestServer::handleWebSocketMessage(void* arg, uint8_t* data, size_t len)
{
	AwsFrameInfo* info = (AwsFrameInfo*)arg;

	// Check if this is the final frame, first fragment, and if the size matches
	if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
	{
		// Safely handle message null-termination
		char* message = new char[len + 1];
		memcpy(message, data, len);
		message[len] = '\0'; // Null-terminate

		// Process incoming WebSocket command
		wsIncomingCommands cmd = getWebSocketCommand(message);
		delete[] message; // Free the buffer after use

		if(cmd != wsIncomingCommands::INVALID_COMMAND && cmd != wsIncomingCommands::GET_READINGS)
		{
			handleWsIncomingCommands(cmd);
			logger.log(LogLevel::SUCCESS, "RECEIVED WEBSOCKET DATA!");
		}
		// else if(cmd == wsIncomingCommands::GET_READINGS)
		// {
		// 	if(isClientConnected)
		// 	{
		// 		if(_ws->availableForWriteAll())
		// 		{
		// 			_sendData = true;
		// 		}
		// 		else
		// 		{
		// 			logger.log(LogLevel::ERROR, "WebSocket not ready for writing");
		// 		}
		// 	}
		// 	else
		// 	{
		// 		logger.log(LogLevel::ERROR, "Client is not connected");
		// 	}
		// }
	}
}

wsIncomingCommands TestServer::getWebSocketCommand(const char* incomingCommand)
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

String sendWebsocketCommands(wsOutgoingCommands cmd)
{
	int index = static_cast<int>(cmd);
	size_t size = sizeof(outgoingCommandTable) / sizeof(outgoingCommandTable[0]);

	// Check if the index is within valid range
	if(index >= 0 && index < size)
	{
		return String(outgoingCommandTable[index]);
	}
	else
	{
		return String("INVALID_COMMAND");
	}
}

void TestServer::handleWsIncomingCommands(wsIncomingCommands cmd)
{
	if(cmd == wsIncomingCommands::TEST_START)
	{
		logger.log(LogLevel::SUCCESS, "handling TEST START EVENT");
		_sync.handleUserCommand(UserCommandEvent::START);
	}
	else if(cmd == wsIncomingCommands::TEST_STOP)
	{
		logger.log(LogLevel::SUCCESS, "handling TEST STOP EVENT");
		_sync.handleUserCommand(UserCommandEvent::STOP);
	}
	else if(cmd == wsIncomingCommands::TEST_PAUSE)
	{
		logger.log(LogLevel::SUCCESS, "handling TEST PAUSE EVENT");
		_sync.handleUserCommand(UserCommandEvent::PAUSE);
	}
	else if(cmd == wsIncomingCommands::AUTO_MODE)
	{
		logger.log(LogLevel::SUCCESS, "handling command set to AUTO ");
		_sync.handleUserCommand(UserCommandEvent::AUTO);
	}
	else if(cmd == wsIncomingCommands::MANUAL_MODE)
	{
		logger.log(LogLevel::SUCCESS, "handling command set to MANUAL ");
		_sync.handleUserCommand(UserCommandEvent::MANUAL);
	}
	else
	{
		logger.log(LogLevel::ERROR, "invalid command ");
	}
}

void TestServer::prepWebSocketData(wsOutGoingDataType type, const char* data)
{
	// Use the provided size for StaticJsonDocument
	StaticJsonDocument<64> doc; // Use a size that fits comfortably within the expected data size

	// Fill the JSON document
	doc["type"] = wsDataTypeToString(type);
	doc["message"] = data;

	// Measure the JSON document size
	size_t len = measureJson(doc);

	// Create a buffer to hold the JSON data
	AsyncWebSocketMessageBuffer* buffer = _ws->makeBuffer(len + 1); // +1 for null terminator
	if(!buffer)
	{
		Serial.println("Error: Failed to create message buffer.");
		return;
	}

	// Serialize the JSON document into the buffer
	size_t bytesWritten = serializeJson(doc, buffer->get(), len + 1); // +1 for null terminator
	if(bytesWritten != len)
	{
		Serial.println("Error: Buffer write did not match measured length.");
		return;
	}

	// Push the buffer to the deque
	// sensorDataQueue.push_back(buffer);
}

void TestServer::sendRandomTestData()
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

void TestServer::notifyClients(String data)
{
	_ws->textAll(data);
}
