#include "PageBuilder.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"

#define ETAG "\"" __DATE__ " " __TIME__ "\""

void PageBuilder::setupPages(TestSync& syncTest)
{
	// Handle the main page
	_server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
		auto* response = request->beginResponseStream("text/html");
		this->sendHtmlHead(response);
		this->sendStyle(response);
		this->sendHeaderTrailer(response);
		this->sendHeader(response);
		this->sendNavbar(response);
		this->sendSidebar(response);
		this->sendUserCommand(response);
		this->sendScript(response);
		this->sendPageTrailer(response);
		request->send(response);
	});

	_server->on("/dashboard", HTTP_GET, [this](AsyncWebServerRequest* request) {
		auto* response = request->beginResponseStream("text/html");
		this->sendHtmlHead(response);
		this->sendStyle(response);
		this->sendHeaderTrailer(response);
		this->sendHeader(response);
		this->sendNavbar(response);
		this->sendSidebar(response);
		this->sendUserCommand(response);
		this->sendScript(response);
		this->sendPageTrailer(response);
		request->send(response);
	});

	_server->on("/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
		auto* response = request->beginResponseStream("text/html");
		this->sendHtmlHead(response);
		this->sendStyle(response);
		this->sendHeaderTrailer(response);
		this->sendHeader(response);
		this->sendNavbar(response);
		UPSTesterSetup& testerSetup = UPSTesterSetup::getInstance();
		this->sendSettingTable(response, testerSetup);
		this->sendScript(response);
		this->sendPageTrailer(response);
		request->send(response);
	});

	// Handle GET request for logs
	_server->on("/log", HTTP_GET, [](AsyncWebServerRequest* request) {
		String logs = logger.getBufferedLogs();
		if(logs.length() > 0)
		{
			request->send(200, "text/plain", logs);
		}
		else
		{
			request->send(200, "text/plain", "No logs available.");
		}
	});

	// Handle POST request for updating test data
	auto* testDataHandler = new AsyncCallbackJsonWebHandler(
		"/updateTestData", [](AsyncWebServerRequest* request, JsonVariant& json) {
			// Print the received JSON to the serial monitor for debugging
			Serial.println("Received JSON Data:");
			serializeJsonPretty(json, Serial);

			// Check if the JSON data is an array
			if(json.is<JsonArray>())
			{
				JsonArray jsonArray = json.as<JsonArray>();

				// Iterate through the array and process each JSON object
				for(JsonVariant value: jsonArray)
				{
					if(value.is<JsonObject>())
					{
						JsonObject jsonObj = value.as<JsonObject>();

						// Validate and process the expected fields
						if(jsonObj.containsKey("testName") && jsonObj.containsKey("loadLevel"))
						{
							Serial.print("testName: ");
							Serial.println(jsonObj["testName"].as<String>());
							Serial.print("loadLevel: ");
							Serial.println(jsonObj["loadLevel"].as<String>());
							TestSync& SyncTest = TestSync::getInstance();
							SyncTest.parseIncomingJson(jsonObj);
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
		});
	_server->addHandler(testDataHandler);
	// Handle POST request to updateMode
	_server->on("/updateMode", HTTP_POST, [](AsyncWebServerRequest* request) {
		if(!request->hasParam("body", true))
		{
			request->send(400, "text/plain", "Invalid request: No body found.");
			return;
		}

		String mode = request->getParam("body", true)->value();
		request->send(200, "text/plain", "Mode received: " + mode);
	});

	// Handle POST request to updateCommand
	_server->on("/updateCommand", HTTP_POST, [](AsyncWebServerRequest* request) {
		if(!request->hasParam("body", true))
		{
			request->send(400, "text/plain", "Invalid request: No body found.");
			return;
		}

		String command = request->getParam("body", true)->value();
		request->send(200, "text/plain", "command received: " + command);
	});

	// Handle 404 errors
	_server->onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "text/plain", "404 Not Found");
	});
}

// void PageBuilder::setupPages(TestSync& testSync)
// {
// 	// Handle the main page
// 	_server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
// 		auto* response = request->beginResponseStream("text/html");
// 		this->sendHeader(response);
// 		this->sendStyle(response);
// 		this->sendHeaderTrailer(response);

// 		const char* routes[] = {"/", "/settings", "/network", "/modbus", "/report"};
// 		this->sendNavbar(response, "UPS TESTING PANEL", routes);
// 		this->sendSidebar(response);
// 		this->sendUserCommand(response);
// 		this->sendScript(response);
// 		this->sendPageTrailer(response);
// 		request->send(response);
// 	});

// 	// Handle GET request for logs
// 	_server->on("/log", HTTP_GET, [](AsyncWebServerRequest* request) {
// 		String logs = logger.getBufferedLogs();
// 		if(logs.length() > 0)
// 		{
// 			request->send(200, "text/plain", logs);
// 		}
// 		else
// 		{
// 			request->send(200, "text/plain", "No logs available.");
// 		}
// 	});

// 	auto* testDataHandler = new AsyncCallbackJsonWebHandler(
// 		"/updateTestData", [this, &testSync](AsyncWebServerRequest* request, JsonVariant& json) {
// 			logger.log(LogLevel::SUCCESS, "Received Json HTTP_POST");

// 			// Serialize and log the received JSON data
// 			String jsonString;
// 			serializeJsonPretty(json, jsonString);
// 			logger.log(LogLevel::SUCCESS, jsonString.c_str());

// 			// Create a StaticJsonDocument for deserialization
// 			StaticJsonDocument<200> doc;
// 			DeserializationError error = deserializeJson(doc, jsonString);

// 			if(error)
// 			{
// 				logger.log(LogLevel::ERROR, "Deserialization failed: ", error.c_str());
// 				request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
// 				return;
// 			}

// 			// Extract values from the deserialized JSON
// 			String testName = doc["testName"].as<String>();
// 			String loadLevel = doc["loadLevel"].as<String>();

// 			// Check and log values
// 			if(testName.isEmpty())
// 			{
// 				logger.log(LogLevel::ERROR, "testName is empty");
// 			}
// 			else
// 			{
// 				logger.log(LogLevel::SUCCESS, "Test Name: ", testName.c_str());
// 				Serial.print("Direct serial");
// 				Serial.println(testName.c_str());
// 			}

// 			if(loadLevel.isEmpty())
// 			{
// 				logger.log(LogLevel::ERROR, "loadLevel is empty");
// 			}
// 			else
// 			{
// 				logger.log(LogLevel::SUCCESS, "Load Level: ", loadLevel.c_str());
// 				Serial.print("Direct serial");
// 				Serial.println(loadLevel.c_str());
// 			}

// 			logger.log(LogLevel::SUCCESS, "Initial parsing success");

// 			// Process the JSON object
// 			testSync.parseIncomingJson(doc.as<JsonObject>());
// 			request->send(200, "application/json", "{\"status\":\"success\"}");
// 		});
// 	_server->addHandler(testDataHandler);

// 	// Handle POST request to update status
// 	_server->on("/updateStatus", HTTP_POST, [](AsyncWebServerRequest* request) {
// 		if(!request->hasParam("body", true))
// 		{
// 			request->send(400, "text/plain", "Invalid request: No body found.");
// 			return;
// 		}

// 		const char* status = request->getParam("body", true)->value().c_str();
// 		String response = "Status received: ";
// 		response += status;

// 		request->send(200, "text/plain", response);
// 	});

// 	// Handle 404 errors
// 	_server->onNotFound([](AsyncWebServerRequest* request) {
// 		request->send(404, "text/plain", "404 Not Found");
// 	});
// }

// void PageBuilder::handleJsonPost(AsyncWebServerRequest* request, JsonVariant& json)
// {
// 	if(!json.is<JsonObject>())
// 	{
// 		request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
// 		return;
// 	}

// 	JsonObject jsonObj = json.as<JsonObject>();

// 	// Clear the existing _testList data before updating with new JSON content
// 	_testList.clear();
// 	for(JsonPair kv: jsonObj)
// 	{
// 		// Add the new key-value pairs to _testList
// 		_testList[kv.key()] = kv.value();
// 	}

// 	request->send(200, "application/json", "{\"status\":\"success\"}");
// }

// void PageBuilder::sendJsonResponse(AsyncWebServerRequest* request)
// {
// 	String jsonResponse;
// 	serializeJson(_testList, jsonResponse); // Serialize the _testList JsonObject

// 	AsyncResponseStream* response = request->beginResponseStream("application/json");
// 	response->print(jsonResponse); // Send the serialized JSON as the response
// 	request->send(response);
// }

const char* PageBuilder::copyFromPROGMEM(const char copyFrom[], char sendTo[])
{
	// Copy the string from PROGMEM to SRAM
	strcpy_P(sendTo, copyFrom);
	// Return the destination buffer
	return sendTo;
}
void PageBuilder::sendHtmlHead(AsyncResponseStream* response)
{
	char buffer[TOP_HTML_LENGTH];
	const char* headerHtml = copyFromPROGMEM(TOP_HTML, buffer);
	response->printf(TOP_HTML);
}
void PageBuilder::sendStyle(AsyncResponseStream* response)
{
	char buffer[CSS_LENGTH];
	const char* CSS = copyFromPROGMEM(STYLE_BLOCK_CSS, buffer);
	response->print(CSS);
}
void PageBuilder::sendHeaderTrailer(AsyncResponseStream* response)
{
	char buffer[HEADER_TRAILER_HTML_LENGTH];
	const char* headerTrailerHtml = copyFromPROGMEM(HEADER_TRAILER_HTML, buffer);
	response->print(headerTrailerHtml);
}
void PageBuilder::sendHeader(AsyncResponseStream* response)
{
	char buffer[HEADER_HTML_LENGTH];
	const char* headerHtml = copyFromPROGMEM(HEADER_HTML, buffer);
	response->printf(HEADER_HTML);
}

void PageBuilder::sendNavbar(AsyncResponseStream* response)
{
	char buffer[NAVBAR_HTML_LENGTH];
	const char* navbarHtml = copyFromPROGMEM(NAVBAR_HTML, buffer);

	response->printf(navbarHtml);
}

void PageBuilder::sendSidebar(AsyncResponseStream* response, const char* content)
{
	char buffer[SIDEBAR_HTML_LENGTH];
	const char* sidebarHtml = copyFromPROGMEM(SIDEBAR_HTML, buffer);
	response->printf(sidebarHtml, content);
}

void PageBuilder::sendScript(AsyncResponseStream* response)
{
	char buffer[JSS_LENGTH];
	const char* JSS = copyFromPROGMEM(MAIN_SCRIPT_JSS, buffer);
	response->print(JSS);
}

void PageBuilder::sendUserCommand(AsyncResponseStream* response, const char* content)
{
	response->print("<div class=\"container\" id=\"content\">");

	// Left side: User commands
	response->print("<div class=\"user-command\">");
	response->print("<h2>User Commands</h2>");
	response->print("<pre id=\"testCommand\"></pre>");
	response->print("</div>");

	// Right side: Log output
	String logs = logger.getBufferedLogs();
	response->print("<div class=\"log-output\">");
	response->print("<h2>Log Output</h2>");
	if(logs.length() > 0)
	{
		response->print("<pre style=\"color:green;\" id=\"logs\">");
		response->printf("%s", logs.c_str());
		response->print("</pre>");
	}
	else
	{
		response->print("<p id=\"logs\">No logs available.</p>");
	}
	response->print("</div>");

	response->print("</div>"); // Closing container div

	// JavaScript for auto-refresh
	response->print(R"(<script>
        function refreshLogs() {
            var xhr = new XMLHttpRequest();
            xhr.open('GET', '/log', true);
            xhr.onload = function() {
                if (xhr.status === 200) {
                    document.getElementById('logs').innerHTML = xhr.responseText;
                }
            };
            xhr.send();
        }
        setInterval(refreshLogs, 2000);
    </script>)");

	char buffer[CONTENT_HTML_LENGTH];
	const char* contentHtml = copyFromPROGMEM(USER_COMMAND_AND_LOG_HTML, buffer);
	response->print(contentHtml);
}

void PageBuilder::sendPageTrailer(AsyncResponseStream* response)
{
	char buffer[LAST_TRAILER_HTML_LENGTH];
	const char* pageTrailerHtml = copyFromPROGMEM(LAST_TRAILER_HTML, buffer);
	response->print(pageTrailerHtml);
}

void PageBuilder::sendSettingTable(AsyncResponseStream* response, UPSTesterSetup& testerSetup)
{
	// Begin table
	response->print("<form method='post' action='/updateSettings'>");
	response->print("<table border='1'>");

	// Retrieve SetupSpec and SetupTest instances
	SetupSpec& spec = testerSetup.specSetup(); // Retrieve SetupSpec
	SetupTest& test = testerSetup.testSetup(); // Retrieve SetupTest

	// Editable dropdown or input fields for SetupSpec
	sendTableRow(response, "Rating (VA)", spec.Rating_va);
	sendInputField(response, "Rating_va", spec.Rating_va);

	sendTableRow(response, "Rated Voltage (V)", spec.RatedVoltage_volt);
	sendInputField(response, "RatedVoltage_volt", spec.RatedVoltage_volt);

	sendTableRow(response, "Rated Current (A)", spec.RatedCurrent_amp);
	sendInputField(response, "RatedCurrent_amp", spec.RatedCurrent_amp);

	sendTableRow(response, "Min Input Voltage (V)", spec.MinInputVoltage_volt);
	sendInputField(response, "MinInputVoltage_volt", spec.MinInputVoltage_volt);

	sendTableRow(response, "Max Input Voltage (V)", spec.MaxInputVoltage_volt);
	sendInputField(response, "MaxInputVoltage_volt", spec.MaxInputVoltage_volt);

	sendTableRow(response, "Avg Switch Time (ms)", spec.AvgSwitchTime_ms);
	sendInputField(response, "AvgSwitchTime_ms", spec.AvgSwitchTime_ms);

	sendTableRow(response, "Avg Backup Time (ms)", spec.AvgBackupTime_ms);
	sendInputField(response, "AvgBackupTime_ms", spec.AvgBackupTime_ms);

	// Handle the Test settings similarly, providing individual editable fields
	sendTableRow(response, "Test Standard", test.TestStandard);
	sendInputField(response, "TestStandard", test.TestStandard);

	sendTableRow(response, "Test Mode", static_cast<int>(test.mode));
	sendDropdown(response, "TestMode", {"AUTO", "MANUAL"}, "AUTO"); // Example Dropdown

	sendTableRow(response, "Test VA Rating", test.testVARating);
	sendInputField(response, "TestVARating", test.testVARating);

	sendTableRow(response, "Input Voltage (V)", test.inputVoltage_volt);
	sendInputField(response, "InputVoltage_volt", test.inputVoltage_volt);

	// More fields...

	// Add the last update timestamp for both SetupSpec and SetupTest
	sendTableRow(response, "Last Update (Spec)", spec.lastUpdateTime());
	sendTableRow(response, "Last Update (Test)", test.lastUpdateTime());

	// End table and form
	response->print("</table>");
	response->print("<button type='submit'>Save Settings</button>");
	response->print("</form>");
}

//----------------------------------HTML BLOCK FACTORY------------------//
void PageBuilder::sendMargin(AsyncResponseStream* response, int pixel, MarginType marginType)
{
	// Open the <div> tag
	response->print("<div");

	// Determine the CSS property based on the MarginType enum
	const char* cssProperty = "margin-top"; // Default to margin-top

	switch(marginType)
	{
		case MarginType::Top:
			cssProperty = "margin-top";
			break;
		case MarginType::Bottom:
			cssProperty = "margin-bottom";
			break;
		case MarginType::Left:
			cssProperty = "margin-left";
			break;
		case MarginType::Right:
			cssProperty = "margin-right";
			break;
	}

	// Add inline style for the specified margin type dynamically based on the input pixel
	response->printf(" style=\"%s: %dpx;\"", cssProperty, pixel);

	// Close the <div> tag
	response->print("></div>");
}

void PageBuilder::sendButton(AsyncResponseStream* response, const char* title, const char* action,
							 const char* css)
{
	response->printf("<form method=\"get\" action=\"%s\">"
					 "<button class=\"%s\">%s</button>"
					 "</form>"
					 "<p></p>",
					 action, css, title);
}

void PageBuilder::sendToggleButton(AsyncResponseStream* response, const char* name, bool state)
{
	response->print("<div>");
	response->printf("<label>%s</label>", name);
	response->printf("<input type='checkbox' %s>", state ? "checked" : "");
	response->print("</div>");
}

void PageBuilder::sendDropdown(AsyncResponseStream* response, const char* name,
							   const std::vector<const char*>& options, const char* selected)
{
	response->print("<div>");
	response->printf("<label>%s</label>", name);
	response->print("<select>");

	for(const auto& option: options)
	{
		response->printf("<option value='%s' %s>%s</option>", option,
						 (selected && strcmp(option, selected) == 0) ? "selected" : "", option);
	}

	response->print("</select>");
	response->print("</div>");
}
void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, String value)
{
	response->printf("<tr>"
					 "<td>%s:</td>"
					 "<td>%s</td>"
					 "</tr>",
					 name, value.c_str());
}

void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, uint32_t value)
{
	response->printf("<tr>"
					 "<td>%s:</td>"
					 "<td>%d</td>"
					 "</tr>",
					 name, value);
}

void PageBuilder::sendInputField(AsyncResponseStream* response, const char* name, uint32_t value)
{
	response->printf("<tr><td>%s:</td>", name);
	response->printf("<td><input type='number' name='%s' value='%d'></td></tr>", name, value);
}

void PageBuilder::sendInputField(AsyncResponseStream* response, const char* name, const char* value)
{
	response->printf("<tr><td>%s:</td>", name);
	response->printf("<td><input type='text' name='%s' value='%s'></td></tr>", name, value);
}
