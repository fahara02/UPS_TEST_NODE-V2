#include "TestServer.h"
#include "AsyncJson.h"
#include <map>
#include <Ticker.h>
#include "HPTSettings.h"

Ticker pingTimer;
extern TaskHandle_t PeriodicDataHandle;
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
	this->webPage->sendUserCommand(response, true);
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
	this->webPage->sendSidebar(response);
	this->webPage->sendUserCommand(response, false);

	this->webPage->sendSettingTable(response, _setup, caption, type, redirect_uri);
	this->webPage->sendPowerMonitor(response);
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

		if(success)
		{
			_setup.notifySpecUpdated(spec, false);
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
		} testFields[] = {

			{"InputVoltage_volt", SetupTest::Field::InputVoltage},
			{"SwitchTestDuration_ms", SetupTest::Field::SWITCH_TestDuration},
			{"BackupTestDuration_ms", SetupTest::Field::BACKUP_TestDuration},
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

		if(success)
		{
			_setup.notifyTestUpdated(test, false);
			logger.log(LogLevel::INTR, "TEST SETUP SUBMIT SUCCESS!!");
		}
		else
		{
			logger.log(LogLevel::ERROR, "TEST SETUP SUBMIT FAILED!!");
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
	if(responseMessage.length() > 1024)
	{
		request->send(500, "text/html", "Response message too long.");
		return;
	}
}

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
	xTaskCreatePinnedToCore(wsClientCleanup, "WSCleanupTask", WSCleanup_Stack, _ws,
							WSCleanup_Priority, NULL, WSCleanup_CORE);
}

void TestServer::createWsDataHandlerTask()
{
	// Initialize the member struct with necessary values
	DataHandler& dataHandler = DataHandler::getInstance();
	wsHandlerTaskParams.ws = _ws;

	// Pass the pointer to the member struct
	xTaskCreatePinnedToCore(dataHandler.wsDataHandler, "wsDataHandler", wsDataHandler_Stack,
							&wsHandlerTaskParams, wsDataHandler_Priority,
							&DataHandler::getInstance().dataTaskHandler, wsDataHandler_CORE);
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

void TestServer::sendPing(AsyncWebSocketClient* client)
{
	// Ensure the client is still connected by checking the event group or valid flag
	EventBits_t bits = xEventGroupGetBits(EventHelper::wsClientEventGroup);

	if(bits & static_cast<EventBits_t>(wsClientStatus::CONNECTED))
	{
		if(client->status() == WS_CONNECTED)
		{
			client->ping(); // Send ping to the client
			Serial.printf("Ping sent to WebSocket client #%u\n", client->id());
		}
	}
	else
	{
		Serial.println("Ping skipped: Client not connected");
	}
}
void TestServer::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
						   void* arg, uint8_t* data, size_t len)
{
	switch(type)
	{
		case WS_EVT_CONNECT:
			Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
						  client->remoteIP().toString().c_str());
			EventHelper::clearBits(wsClientStatus::DISCONNECTED);
			EventHelper::setBits(wsClientStatus::CONNECTED);
			DataHandler::getInstance().updateClientList(client->id(), true);

			if(!pingTimer.active())
			{
				pingTimer.attach(10, sendPing, client);
				Serial.println("Ping timer attached.");
			}
			break;

		case WS_EVT_DISCONNECT:
			Serial.printf("WebSocket client #%u disconnected\n", client->id());
			DataHandler::getInstance().updateClientList(client->id(), false);
			EventHelper::clearBits(wsClientStatus::CONNECTED);
			EventHelper::setBits(wsClientStatus::DISCONNECTED);
			EventHelper::clearBits(wsClientUpdate::GET_READING);

			if(pingTimer.active())
			{
				pingTimer.detach();
				Serial.println("Ping timer detached.");
			}
			break;

		case WS_EVT_DATA:
		{
			AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);

			if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
			{
				if(len >= WS_BUFFER_SIZE)
				{
					Serial.println("Message too large; discarding.");
					client->text(R"({"error":"Message too large"})");
					return;
				}

				WebSocketMessage wsMsg = {};
				memcpy(wsMsg.data, data, len);
				wsMsg.len = len;
				wsMsg.info = *info;
				wsMsg.client_id = client->id();
				wsMsg.client = client;

				if(xEventGroupWaitBits(EventHelper::wsClientEventGroup,
									   static_cast<EventBits_t>(wsClientStatus::CONNECTED), pdFALSE,
									   pdFALSE, CLIENT_CONNECT_TIMEOUT_MS))
				{
					EventHelper::setBits(wsClientStatus::DATA);

					if(strcmp(reinterpret_cast<char*>(wsMsg.data), "getReadings") == 0)
					{
						if(xQueueSend(DataHandler::getInstance().WebsocketDataQueue, &wsMsg,
									  QUEUE_TIMEOUT_MS) == pdTRUE)
						{
							EventHelper::setBits(wsClientUpdate::GET_READING);
							client->text(R"({"SUCCESS":"Received Get Readings Command"})");
						}
						else
						{
							Serial.println("Queue full. Message dropped.");
						}
					}
					else if(xQueueSend(DataHandler::getInstance().WebsocketDataQueue, &wsMsg,
									   QUEUE_TIMEOUT_MS) != pdTRUE)
					{
						Serial.println("Failed to enqueue message.");
					}
				}
				else
				{
					client->text(R"({"error":"Connection timeout"})");
				}
			}
			else
			{
				client->text(R"({"error":"Invalid WebSocket frame"})");
			}
			break;
		}

		case WS_EVT_PONG:
			Serial.println("PONG received.");
			break;

		case WS_EVT_ERROR:
			Serial.println("WebSocket error occurred.");
			break;
	}
}

// void TestServer::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType
// type, 						   void* arg, uint8_t* data, size_t len)
// {
// 	switch(type)
// 	{
// 		case WS_EVT_CONNECT:
// 			Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
// 						  client->remoteIP().toString().c_str());

// 			EventHelper::clearBits(wsClientStatus::DISCONNECTED);
// 			EventHelper::setBits(wsClientStatus::CONNECTED);
// 			DataHandler::getInstance().updateClientList(client->id(), true);
// 			if(!pingTimer.active())
// 			{
// 				pingTimer.attach(10, sendPing, client);
// 				Serial.println("Ping timer attached.");
// 			}
// 			break;

// 		case WS_EVT_DISCONNECT:
// 			Serial.printf("WebSocket client #%u disconnected\n", client->id());
// 			// server->client(client->id())->close();
// 			DataHandler::getInstance().updateClientList(client->id(), false);
// 			EventHelper::clearBits(wsClientStatus::CONNECTED);
// 			EventHelper::setBits(wsClientStatus::DISCONNECTED);
// 			EventHelper::clearBits(wsClientUpdate::GET_READING);
// 			if(pingTimer.active())
// 			{
// 				pingTimer.detach();
// 				Serial.println("Ping timer detached.");
// 			}
// 			break;

// 		case WS_EVT_DATA:
// 		{
// 			AwsFrameInfo* info = (AwsFrameInfo*)arg;

// 			if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
// 			{
// 				if(len >= WS_BUFFER_SIZE)
// 				{
// 					Serial.println("Received data exceeds buffer size; discarding message.");
// 					// Construct and send error message for oversized data
// 					JsonDocument  errorMsg;
// 					errorMsg["error"] = "Message too large";
// 					String errorStr;
// 					serializeJson(errorMsg, errorStr);
// 					client->text(errorStr);
// 					return;
// 				}

// 				WebSocketMessage wsMsg;
// 				memcpy(wsMsg.data, data, len);
// 				wsMsg.len = len;
// 				wsMsg.info = *info;

// 				if(client == nullptr)
// 				{
// 					Serial.println("Client object is nullptr.");
// 					return;
// 				}

// 				if(xEventGroupWaitBits(EventHelper::wsClientEventGroup,
// 									   static_cast<EventBits_t>(wsClientStatus::CONNECTED), pdFALSE,
// 									   pdFALSE, CLIENT_CONNECT_TIMEOUT_MS))

// 				{
// 					EventHelper::setBits(wsClientStatus::DATA);
// 					wsMsg.client_id = client->id();
// 					wsMsg.client = client;
// 					if(strcmp(reinterpret_cast<char*>(wsMsg.data), "getReadings") == 0)
// 					{
// 						EventHelper::setBits(wsClientUpdate::GET_READING);

// 						if(xQueueSend(DataHandler::getInstance().WebsocketDataQueue, &wsMsg,
// 									  QUEUE_TIMEOUT_MS) == pdTRUE)
// 						{
// 							Serial.printf("Message from client: %s\n", wsMsg.data);
// 							JsonDocument  ackMsg;
// 							ackMsg["SUCCESS"] = "Received Get Readings Command";
// 							String ackStr;
// 							serializeJson(ackMsg, ackStr);
// 							client->text(ackStr);
// 						}
// 						else
// 						{
// 							Serial.println("Failed to enqueue WebSocket message within timeout.");
// 						}
// 					}
// 					else
// 					{
// 						if(xQueueSend(DataHandler::getInstance().WebsocketDataQueue, &wsMsg,
// 									  QUEUE_TIMEOUT_MS) != pdPASS)
// 						{
// 							Serial.println("Failed to enqueue WebSocket message within timeout.");
// 						}
// 					}
// 				}
// 				else
// 				{
// 					Serial.println("Client not connected in time.");
// 					// Construct JSON error message for connection timeout
// 					JsonDocument errorMsg;
// 					errorMsg["error"] = "Connection timeout";
// 					String errorStr;
// 					serializeJson(errorMsg, errorStr);
// 					client->text(errorStr);
// 				}
// 			}
// 			else
// 			{
// 				Serial.println("Incomplete or invalid WebSocket frame.");

// 				JsonDocument  errorMsg;
// 				errorMsg["error"] = "Invalid WebSocket frame";
// 				errorMsg["details"] = "The WebSocket frame is incomplete or invalid.";
// 				String errorStr;
// 				serializeJson(errorMsg, errorStr);
// 				client->text(errorStr);
// 			}
// 			break;
// 		}

// 		case WS_EVT_PONG:
// 			Serial.println("PONG received");
// 			break;

// 		case WS_EVT_ERROR:
// 			Serial.println("WebSocket error occurred");
// 			break;
// 	}
// }
