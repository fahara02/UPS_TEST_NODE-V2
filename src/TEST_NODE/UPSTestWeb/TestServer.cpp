#include "TestServer.h"
#include "AsyncJson.h"

// Other includes as necessary

#define ETAG "\"" __DATE__ " " __TIME__ "\""

void TestServer::servePages(UPSTesterSetup& _setup, TestSync& _sync)
{
	// Handle GET requests
	std::vector<String> getUris = {"/", "/dashboard", "/settings/ups-specification",
								   "/settings/test-specification", "/log"};
	for(const auto& uri: getUris)
	{
		_server->on(uri.c_str(), HTTP_GET, [this, uri, &_setup](AsyncWebServerRequest* request) {
			this->handleGETRequest(uri, request, _setup);
		});
	}

	// Handle POST requests
	std::vector<String> postUris = {"/updateMode", "/updateCommand"};
	for(const auto& uri: postUris)
	{
		_server->on(uri.c_str(), HTTP_POST,
					[this, uri, &_setup, &_sync](AsyncWebServerRequest* request) {
						this->handlePOSTRequest(uri, request, _setup, _sync);
					});
	}

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

void TestServer::handleGETRequest(const String& uri, AsyncWebServerRequest* request,
								  UPSTesterSetup& _setup)
{
	auto* response = request->beginResponseStream("text/html");

	this->webPage->sendHtmlHead(response);
	this->webPage->sendStyle(response);
	this->webPage->sendHeaderTrailer(response);
	this->webPage->sendHeader(response);
	this->webPage->sendNavbar(response);
	this->webPage->sendSidebar(response);

	if(uri == "/settings/ups-specification")
	{
		this->webPage->sendTableStyle(response);
		// UPSTesterSetup& testerSetup = UPSTesterSetup::getInstance();
		this->webPage->sendSettingTable(response, _setup, "UPS SPECIFICATION", SettingType::SPEC);
	}
	else if(uri == "/settings/test-specification")
	{
		this->webPage->sendTableStyle(response);
		// UPSTesterSetup& testerSetup = UPSTesterSetup::getInstance();
		this->webPage->sendSettingTable(response, _setup, "TEST SPECIFICATION", SettingType::TEST);
	}
	else if(uri == "/dashboard" || uri == "/")
	{
		this->webPage->sendUserCommand(response);
	}
	else if(uri == "/log")
	{
		String logs = logger.getBufferedLogs();
		request->send(200, "text/plain", logs.length() > 0 ? logs : "No logs available.");
		return;
	}
	else
	{
		// Handle other GET URIs or send a default page.
	}

	this->webPage->sendScript(response);
	this->webPage->sendPageTrailer(response);
	request->send(response);
}

void TestServer::handlePOSTRequest(const String& uri, AsyncWebServerRequest* request,
								   UPSTesterSetup& _setup, TestSync& _sync)
{
	if(uri == "/updateMode" || uri == "/updateCommand")
	{
		if(!request->hasParam("body", true))
		{
			request->send(400, "text/plain", "Invalid request: No body found.");
			return;
		}

		String value = request->getParam("body", true)->value();
		String responseText = (uri == "/updateMode") ? "Mode received: " : "Command received: ";
		request->send(200, "text/plain", responseText + value);
	}
	else if(uri == "/updateTestData")
	{
		// Handle the JSON data
	}
	else
	{
		// Handle other POST URIs
	}
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
					// TestSync& SyncTest = TestSync::getInstance();
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
