#include "TestServer.h"
#include "AsyncJson.h"

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

	_server->on(
		"/settings/ups-specification", HTTP_GET, [this, &_setup](AsyncWebServerRequest* request) {
			this->handleSettingRequest(request, _setup, "UPS_SPECIFICATION", SettingType::SPEC);
		});

	_server->on(
		"/settings/test-specification", HTTP_GET, [this, &_setup](AsyncWebServerRequest* request) {
			this->handleSettingRequest(request, _setup, "TEST_SPECIFICATION", SettingType::TEST);
		});

	_server->on("/log", HTTP_GET, [this](AsyncWebServerRequest* request) {
		this->handleLogRequest(request);
	});

	// Handle individual POST requests
	_server->on("/updateMode", HTTP_POST, [this, &_setup, &_sync](AsyncWebServerRequest* request) {
		this->handleUpdateModeRequest(request, _setup, _sync);
	});

	_server->on("/updateCommand", HTTP_POST,
				[this, &_setup, &_sync](AsyncWebServerRequest* request) {
					this->handleUserCommandRequest(request, _setup, _sync);
				});

	_server->on("/updateSettings/spec", HTTP_POST, [this, &_setup](AsyncWebServerRequest* request) {
		this->handleUpdateSettingRequest(request, _setup, SettingType::SPEC);
	});
	_server->on("/updateSettings/test", HTTP_POST, [this, &_setup](AsyncWebServerRequest* request) {
		this->handleUpdateSettingRequest(request, _setup, SettingType::TEST);
	});

	auto* testDataHandler = new AsyncCallbackJsonWebHandler(
		"/updateTestData", [this, &_sync](AsyncWebServerRequest* request, JsonVariant& json) {
			this->handleTestDataRequest(request, json, _sync);
		});
	_server->addHandler(testDataHandler);

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
									  const char* setting_name, SettingType type)
{
	auto* response = request->beginResponseStream("text/html");
	this->webPage->sendHtmlHead(response);
	this->webPage->sendStyle(response);
	this->webPage->sendTableStyle(response);
	this->webPage->sendHeaderTrailer(response);
	this->webPage->sendHeader(response);
	this->webPage->sendNavbar(response);

	this->webPage->sendSettingTable(response, _setup, setting_name, type);
	this->webPage->sendScript(response);
	this->webPage->sendPageTrailer(response);
	request->send(response);
}

void TestServer::handleUpdateSettingRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
											SettingType type)
{
	String responseMessage;

	if(type == SettingType::SPEC)
	{
		SetupSpec& spec = _setup.specSetup();

		if(request->hasParam("Rating_va", true))
		{
			String ratingVa = request->getParam("Rating_va", true)->value();
			spec.setField(SetupSpec::Field::RatingVa, ratingVa.toDouble());
			responseMessage += "Spec Settings rated VA updated successfully.<br>";
		}

		if(request->hasParam("RatedVoltage_volt", true))
		{
			String ratedVolt = request->getParam("RatedVoltage_volt", true)->value();
			spec.setField(SetupSpec::Field::RatedVoltage, ratedVolt.toDouble());
			responseMessage += "Spec Settings rated voltage updated successfully.<br>";
		}
	}

	if(type == SettingType::TEST)
	{
		SetupTest& test = _setup.testSetup();

		// Update TestStandard field (const char*)
		if(request->hasParam("TestStandard", true))
		{
			String testStandardStr = request->getParam("TestStandard", true)->value();
			test.TestStandard = testStandardStr.c_str(); // Assign string to const char*
			responseMessage += "Test Settings standard updated successfully.<br>";
		}

		// Update TestMode field (enum TestMode)
		if(request->hasParam("TestMode", true))
		{
			String modeStr = request->getParam("TestMode", true)->value();
			TestMode mode;

			// Assuming your TestMode enum has values like AUTO, MANUAL, etc.
			if(modeStr == "AUTO")
			{
				mode = TestMode::AUTO;
			}
			else if(modeStr == "MANUAL")
			{
				mode = TestMode::MANUAL;
			}
			else
			{
				request->send(400, "text/html", "Invalid Test Mode.");
				return; // Early exit if invalid mode
			}

			test.mode = mode;
			responseMessage += "Test Settings mode updated successfully.<br>";
		}
	}

	if(responseMessage.isEmpty())
	{
		request->send(400, "text/html", "No valid settings updated.");
	}
	else
	{
		request->send(200, "text/html", responseMessage);
	}
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
