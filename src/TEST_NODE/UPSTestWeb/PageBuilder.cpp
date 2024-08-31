#include "PageBuilder.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"

extern TestSync& SyncTest;

#define ETAG "\"" __DATE__ " " __TIME__ "\""

void PageBuilder::setupPages(TestSync& testSync)
{
	// Handle the main page
	_server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
		auto* response = request->beginResponseStream("text/html");
		this->sendHeader(response);
		this->sendStyle(response);
		this->sendHeaderTrailer(response);

		const char* routes[] = {"/", "/settings", "/network", "/modbus", "/report"};
		this->sendNavbar(response, "UPS TESTING PANEL", routes);
		this->sendSidebar(response);
		this->sendUserCommand(response);
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

	auto* testDataHandler = new AsyncCallbackJsonWebHandler(
		"/updateTestData", [this, &testSync](AsyncWebServerRequest* request, JsonVariant& json) {
			logger.log(LogLevel::SUCCESS, "Received Json HTTP_POST");

			// Serialize and log the received JSON data
			String jsonString;
			serializeJsonPretty(json, jsonString);
			logger.log(LogLevel::SUCCESS, jsonString.c_str());

			// Check if the JSON data is an array
			if(json.is<JsonArray>())
			{
				JsonArray jsonArray = json.as<JsonArray>();

				// Process each JSON object in the array
				for(JsonVariant value: jsonArray)
				{
					if(value.is<JsonObject>())
					{
						JsonObject jsonObj = value.as<JsonObject>();

						// Validate required fields
						if(jsonObj.containsKey("testName") && jsonObj.containsKey("loadLevel"))
						{
							const char* testName = jsonObj["testName"];
							const char* loadLevel = jsonObj["loadLevel"];

							logger.log(LogLevel::SUCCESS, "Test Name: ", testName);
							logger.log(LogLevel::SUCCESS, "Load Level: ", loadLevel);
							logger.log(LogLevel::SUCCESS, "Initial parsing success");

							// Process the JSON object
							SyncTest.parseIncomingJson(jsonObj);
						}
						else
						{
							logger.log(LogLevel::ERROR,
									   "Error: Required fields missing in JSON object.");
						}
					}
					else
					{
						logger.log(LogLevel::ERROR, "Error: Expected JSON object in array.");
					}
				}
				request->send(200, "application/json", "{\"status\":\"success\"}");
			}
			else
			{
				logger.log(LogLevel::ERROR, "Error: Invalid JSON format, expected an array.");

				request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
			}
		});
	_server->addHandler(testDataHandler);

	// Handle POST request to update status
	_server->on("/updateStatus", HTTP_POST, [](AsyncWebServerRequest* request) {
		if(!request->hasParam("body", true))
		{
			request->send(400, "text/plain", "Invalid request: No body found.");
			return;
		}

		const char* status = request->getParam("body", true)->value().c_str();
		String response = "Status received: ";
		response += status;

		request->send(200, "text/plain", response);
	});

	// Handle 404 errors
	_server->onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "text/plain", "404 Not Found");
	});
}

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

void PageBuilder::sendHeader(AsyncResponseStream* response)
{
	char buffer[HEADER_HTML_LENGTH];
	const char* headerHtml = copyFromPROGMEM(HEADER_HTML, buffer);
	response->printf(HEADER_HTML);
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
void PageBuilder::sendNavbar(AsyncResponseStream* response, const char* title, const char* routes[],
							 const char* btn1class, const char* btn2class, const char* btn3class)
{
	char buffer[NAVBAR_HTML_LENGTH];
	const char* navbarHtml = copyFromPROGMEM(NAVBAR_HTML, buffer);

	response->printf(navbarHtml,
					 title, // title
					 routes[0], // Dashboard route
					 routes[1], // Settings route
					 routes[2], // Network route
					 routes[3], // Modbus route
					 routes[4], // Test Report route

					 btn1class, // Action for Shutdown button
					 btn2class, // CSS class for Shutdown button
					 btn3class // Action for Restart button
	);
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

void PageBuilder::sendPageTrailer(AsyncResponseStream* response)
{
	char buffer[LAST_TRAILER_HTML_LENGTH];
	const char* pageTrailerHtml = copyFromPROGMEM(LAST_TRAILER_HTML, buffer);
	response->print(pageTrailerHtml);
}

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
