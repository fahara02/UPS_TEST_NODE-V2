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
	this->webPage->sendStyle(response);
	this->webPage->sendHeaderTrailer(response);
	this->webPage->sendHeader(response);
	this->webPage->sendNavbar(response);
	this->webPage->sendSidebar(response);
	this->webPage->sendUserCommand(response);
	this->webPage->sendPowerMonitor(response);

	this->webPage->sendScript(response);
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
	this->webPage->sendStyle(response);
	// this->webPage->sendTableStyle(response);
	this->webPage->sendHeaderTrailer(response);
	this->webPage->sendHeader(response);
	this->webPage->sendNavbar(response);

	this->webPage->sendSettingTable(response, _setup, caption, type, redirect_uri);

	this->webPage->sendScript(response);
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

void TestServer::createWSCleanUpTask()
{
	xTaskCreate(wsClientCleanup, "WSCleanupTask", 1024, _ws, 1, NULL);
}
void TestServer::wsClientCleanup(void* pvParameters)
{
	AsyncWebSocket* ws = static_cast<AsyncWebSocket*>(pvParameters);
	while(true)
	{
		ws->cleanupClients();
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}
void TestServer::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
						   void* arg, uint8_t* data, size_t len)
{
	switch(type)
	{
		case WS_EVT_CONNECT:
			Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
						  client->remoteIP().toString().c_str());
			break;
		case WS_EVT_DISCONNECT:
			Serial.printf("WebSocket client #%u disconnected\n", client->id());
			break;
		case WS_EVT_DATA:
			handleWebSocketMessage(arg, data, len);
			break;
		case WS_EVT_PONG:
		case WS_EVT_ERROR:
			break;
	}
}

void TestServer::handleWebSocketMessage(void* arg, uint8_t* data, size_t len)
{
	AwsFrameInfo* info = (AwsFrameInfo*)arg;
	if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
	{
		data[len] = 0; // Null-terminate the string
		wsIncomingCommands cmd = getWebSocketCommand((char*)data);
		if(cmd != wsIncomingCommands::INVALID_COMMAND)
		{
			handleWsIncomingCommands(cmd);
			notifyClients();
		}
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
		//_sync.ReportEvent(Event::TEST_START);
	}
	else if(cmd == wsIncomingCommands::TEST_STOP)
	{
		logger.log(LogLevel::SUCCESS, "handling TEST STOP EVENT");
		//_sync.ReportEvent(Event::TEST_STOP);
	}
	else if(cmd == wsIncomingCommands::TEST_PAUSE)
	{
		logger.log(LogLevel::SUCCESS, "handling TEST PAUSE EVENT");
		//_sync.ReportEvent(Event::TEST_PAUSE);
	}
	else
	{
		// Handle unknown command or do nothing
	}
}

void TestServer::sendwsData(wsOutGoingDataType type, const char* data)
{
	// Convert the wsOutGoingDataType to a string identifier to send to the frontend
	String message;
	switch(type)
	{
		case wsOutGoingDataType::INPUT_POWER:
			message = String("{\"inputWattage\":\"") + data + "\"}";
			break;
		case wsOutGoingDataType::INPUT_VOLT:
			message = String("{\"inputVoltage\":\"") + data + " V\"}";
			break;
		case wsOutGoingDataType::INPUT_CURRENT:
			message = String("{\"inputCurrent\":\"") + data + " A\"}";
			break;
		case wsOutGoingDataType::INPUT_PF:
			message = String("{\"inputPowerFactor\":\"") + data + "\"}";
			break;
		case wsOutGoingDataType::OUTPUT_POWER:
			message = String("{\"outputWattage\":\"") + data + " W\"}";
			break;
		case wsOutGoingDataType::OUTPUT_VOLT:
			message = String("{\"outputVoltage\":\"") + data + " V\"}";
			break;
		case wsOutGoingDataType::OUTPUT_CURRENT:
			message = String("{\"outputCurrent\":\"") + data + " A\"}";
			break;
		case wsOutGoingDataType::OUTPUT_PF:
			message = String("{\"outputPowerFactor\":\"") + data + "\"}";
			break;
		default:
			message = "{\"error\":\"Invalid data type\"}";
			break;
	}

	// Send the message to all connected WebSocket clients
	_ws->textAll(message);
}

void TestServer::sendRandomTestData()
{
	sendwsData(wsOutGoingDataType::INPUT_POWER, String(random(1000, 2000)).c_str());
	sendwsData(wsOutGoingDataType::INPUT_VOLT, String(random(200, 240)).c_str());
	sendwsData(wsOutGoingDataType::INPUT_CURRENT, String(random(5, 10)).c_str());
	sendwsData(wsOutGoingDataType::INPUT_PF, String(random(95, 100) / 100.0, 2).c_str());
	sendwsData(wsOutGoingDataType::OUTPUT_POWER, String(random(900, 1800)).c_str());
	sendwsData(wsOutGoingDataType::OUTPUT_VOLT, String(random(210, 230)).c_str());
	sendwsData(wsOutGoingDataType::OUTPUT_CURRENT, String(random(4, 8)).c_str());
	sendwsData(wsOutGoingDataType::OUTPUT_PF, String(random(90, 100) / 100.0, 2).c_str());
}

void TestServer::notifyClients()
{
	_ws->textAll("Command Executed");
}