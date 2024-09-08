#include "TestServer.h"
#include "AsyncJson.h"
#include <map>
#include <Ticker.h>
Ticker pingTimer;

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
}

void TestServer::createPeriodicTask()
{
	// Initialize the member struct with necessary values
	DataHandler& dataHandler = DataHandler::getInstance();
	periodicTaskParams.ws = _ws;

	// Pass the pointer to the member struct
	xTaskCreate(dataHandler.periodicDataSender, "PeriodicDataSender", 4096, &periodicTaskParams, 1,
				nullptr);
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

// void TestServer::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType
// type, 						   void* arg, uint8_t* data, size_t len)
// {
// 	switch(type)
// 	{
// 		case WS_EVT_CONNECT:
// 			Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
// 						  client->remoteIP().toString().c_str());
// 			isClientConnected = true; // Set flag to true when client connects
// 			break;

// 		case WS_EVT_DISCONNECT:
// 			Serial.printf("WebSocket client #%u disconnected\n", client->id());
// 			isClientConnected = false; // Set flag to false when client disconnects
// 			break;

// 		case WS_EVT_DATA:
// 		{
// 			AwsFrameInfo* info = (AwsFrameInfo*)arg;

// 			if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
// 			{
// 				// Check if the data length fits within the buffer
// 				if(len >= WS_BUFFER_SIZE)
// 				{
// 					Serial.println("Received data exceeds buffer size; discarding message.");
// 					client->text("Error: Message too large.");
// 					return;
// 				}

// 				WebSocketMessage wsMsg;
// 				memcpy(wsMsg.data, data, len); // Copy data to the fixed-size buffer
// 				wsMsg.len = len; // Set the message length
// 				wsMsg.info = *info; // Copy the frame info

// 				// Process the message as needed
// 				if(strcmp(reinterpret_cast<char*>(wsMsg.data), "getReadings") == 0)
// 				{
// 					if(!DataHandler::getInstance().wsDeque.empty())
// 					{
// 						const std::array<char, WS_BUFFER_SIZE>& jsonData =
// 							DataHandler::getInstance().wsDeque.front();
// 						DataHandler::getInstance().wsDeque.pop_front();

// 						client->text(jsonData.data(), strlen(jsonData.data()));
// 					}
// 					else
// 					{
// 						client->text("Data not available yet");
// 					}
// 				}
// 				else
// 				{
// 					// Enqueue message for processing
// 					if(xQueueSend(DataHandler::getInstance().WebsocketDataQueue, &wsMsg,
// 								  portMAX_DELAY) != pdPASS)
// 					{
// 						Serial.println("Failed to enqueue WebSocket message.");
// 					}
// 				}
// 			}
// 			else
// 			{
// 				Serial.println("Incomplete or invalid WebSocket frame.");
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
			pingTimer.attach(30, sendPing, client);
			break;

		case WS_EVT_DISCONNECT:
			Serial.printf("WebSocket client #%u disconnected\n", client->id());
			EventHelper::clearBits(wsClientStatus::CONNECTED);
			EventHelper::setBits(wsClientStatus::DISCONNECTED);
			EventHelper::clearBits(wsClientUpdate::GET_READING);
			pingTimer.detach();
			break;

		case WS_EVT_DATA:
		{
			AwsFrameInfo* info = (AwsFrameInfo*)arg;

			if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
			{
				if(len >= WS_BUFFER_SIZE)
				{
					Serial.println("Received data exceeds buffer size; discarding message.");
					// Construct and send error message for oversized data
					StaticJsonDocument<256> errorMsg;
					errorMsg["error"] = "Message too large";
					String errorStr;
					serializeJson(errorMsg, errorStr);
					client->text(errorStr);
					return;
				}

				EventHelper::setBits(wsClientStatus::DATA);

				WebSocketMessage wsMsg;
				memcpy(wsMsg.data, data, len);
				wsMsg.len = len;
				wsMsg.info = *info;

				if(xEventGroupWaitBits(EventHelper::wsClientEventGroup,
									   static_cast<EventBits_t>(wsClientStatus::CONNECTED), pdFALSE,
									   pdFALSE, CLIENT_CONNECT_TIMEOUT_MS))
				{
					wsMsg.client_id = client->id();

					if(strcmp(reinterpret_cast<char*>(wsMsg.data), "getReadings") == 0)
					{
						EventHelper::setBits(wsClientUpdate::GET_READING);
						Serial.printf("Message from client: %s\n", wsMsg.data);
						StaticJsonDocument<256> ackMsg;
						ackMsg["SUCCESS"] = "Received Get Readings Command";
						String ackStr;
						serializeJson(ackMsg, ackStr);
						client->text(ackStr);

						// // if(xSemaphoreTake(DataHandler::getInstance().dataEventMutex,
						// // portMAX_DELAY))
						// // {
						// Serial.println("Checking if deque is empty");

						// if(!DataHandler::getInstance().wsDeque.empty())
						// {
						// 	Serial.println("Sending data...");
						// 	const std::array<char, WS_BUFFER_SIZE>& jsonData =
						// 		DataHandler::getInstance().wsDeque.front();
						// 	DataHandler::getInstance().wsDeque.pop_front();
						// 	// xSemaphoreGive(DataHandler::getInstance().dataEventMutex);
						// 	client->text(jsonData.data(), jsonData.size());

						// 	if(xQueueSend(DataHandler::getInstance().WebsocketDataQueue, &wsMsg,
						// 				  QUEUE_TIMEOUT_MS) != pdPASS)
						// 	{
						// 		Serial.println(
						// 			"Failed to enqueue WebSocket message within timeout.");
						// 	}
						// }
						// else
						// {
						// 	// xSemaphoreGive(DataHandler::getInstance().dataEventMutex);
						// 	//  Construct JSON error message for data not available
						// 	StaticJsonDocument<256> errorMsg;
						// 	errorMsg["warning"] = "Data not available yet";
						// 	String errorStr;
						// 	serializeJson(errorMsg, errorStr);
						// 	client->text(errorStr);
						// }
						// //}
					}
					else
					{
						// Enqueue message for processing
						if(xQueueSend(DataHandler::getInstance().WebsocketDataQueue, &wsMsg,
									  QUEUE_TIMEOUT_MS) != pdPASS)
						{
							Serial.println("Failed to enqueue WebSocket message within timeout.");
						}
					}
				}
				else
				{
					Serial.println("Client not connected in time.");
					// Construct JSON error message for connection timeout
					StaticJsonDocument<256> errorMsg;
					errorMsg["error"] = "Connection timeout";
					String errorStr;
					serializeJson(errorMsg, errorStr);
					client->text(errorStr);
				}
			}
			else
			{
				Serial.println("Incomplete or invalid WebSocket frame.");

				StaticJsonDocument<256> errorMsg;
				errorMsg["error"] = "Invalid WebSocket frame";
				errorMsg["details"] = "The WebSocket frame is incomplete or invalid.";
				String errorStr;
				serializeJson(errorMsg, errorStr);
				client->text(errorStr);
			}
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
