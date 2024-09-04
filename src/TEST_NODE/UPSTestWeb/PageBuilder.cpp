#include "PageBuilder.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"

// void PageBuilder::setupPages(TestSync& syncTest)
// {
// 	// Handle the main page
// 	_server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
// 		auto* response = request->beginResponseStream("text/html");
// 		this->sendHtmlHead(response);
// 		this->sendStyle(response);
// 		this->sendHeaderTrailer(response);
// 		this->sendHeader(response);
// 		this->sendNavbar(response);
// 		this->sendSidebar(response);
// 		this->sendUserCommand(response);
// 		this->sendScript(response);
// 		this->sendPageTrailer(response);
// 		request->send(response);
// 	});

// 	_server->on("/dashboard", HTTP_GET, [this](AsyncWebServerRequest* request) {
// 		auto* response = request->beginResponseStream("text/html");
// 		this->sendHtmlHead(response);
// 		this->sendStyle(response);
// 		this->sendHeaderTrailer(response);
// 		this->sendHeader(response);
// 		this->sendNavbar(response);
// 		this->sendSidebar(response);
// 		this->sendUserCommand(response);
// 		this->sendScript(response);
// 		this->sendPageTrailer(response);
// 		request->send(response);
// 	});

// 	_server->on("/settings/ups-specification", HTTP_GET, [this](AsyncWebServerRequest* request) {
// 		auto* response = request->beginResponseStream("text/html");
// 		this->sendHtmlHead(response);
// 		this->sendStyle(response);
// 		this->sendTableStyle(response);
// 		this->sendHeaderTrailer(response);
// 		this->sendHeader(response);
// 		this->sendNavbar(response);
// 		UPSTesterSetup& testerSetup = UPSTesterSetup::getInstance();
// 		this->sendSettingTable(response, testerSetup, "UPS SPECIFICATION", SettingType::SPEC);

// 		this->sendScript(response);
// 		this->sendPageTrailer(response);
// 		request->send(response);
// 	});
// 	_server->on("/settings/test-specification", HTTP_GET, [this](AsyncWebServerRequest* request) {
// 		auto* response = request->beginResponseStream("text/html");
// 		this->sendHtmlHead(response);
// 		this->sendStyle(response);
// 		this->sendTableStyle(response);
// 		this->sendHeaderTrailer(response);
// 		this->sendHeader(response);
// 		this->sendNavbar(response);
// 		UPSTesterSetup& testerSetup = UPSTesterSetup::getInstance();
// 		this->sendSettingTable(response, testerSetup, "TEST SPECIFICATION", SettingType::TEST);

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

// 	// Handle POST request for updating test data
// 	auto* testDataHandler = new AsyncCallbackJsonWebHandler(
// 		"/updateTestData", [](AsyncWebServerRequest* request, JsonVariant& json) {
// 			// Print the received JSON to the serial monitor for debugging
// 			Serial.println("Received JSON Data:");
// 			serializeJsonPretty(json, Serial);

// 			// Check if the JSON data is an array
// 			if(json.is<JsonArray>())
// 			{
// 				JsonArray jsonArray = json.as<JsonArray>();

// 				// Iterate through the array and process each JSON object
// 				for(JsonVariant value: jsonArray)
// 				{
// 					if(value.is<JsonObject>())
// 					{
// 						JsonObject jsonObj = value.as<JsonObject>();

// 						// Validate and process the expected fields
// 						if(jsonObj.containsKey("testName") && jsonObj.containsKey("loadLevel"))
// 						{
// 							Serial.print("testName: ");
// 							Serial.println(jsonObj["testName"].as<String>());
// 							Serial.print("loadLevel: ");
// 							Serial.println(jsonObj["loadLevel"].as<String>());
// 							TestSync& SyncTest = TestSync::getInstance();
// 							SyncTest.parseIncomingJson(jsonObj);
// 						}
// 						else
// 						{
// 							Serial.println("Error: Required fields missing in JSON.");
// 						}
// 					}
// 					else
// 					{
// 						Serial.println("Error: Expected JSON objects in array.");
// 					}
// 				}
// 				request->send(200, "application/json", "{\"status\":\"success\"}");
// 			}
// 			else
// 			{
// 				Serial.println("Error: Invalid JSON format.");
// 				request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
// 			}
// 		});
// 	_server->addHandler(testDataHandler);
// 	// Handle POST request to updateMode
// 	_server->on("/updateMode", HTTP_POST, [](AsyncWebServerRequest* request) {
// 		if(!request->hasParam("body", true))
// 		{
// 			request->send(400, "text/plain", "Invalid request: No body found.");
// 			return;
// 		}

// 		String mode = request->getParam("body", true)->value();
// 		request->send(200, "text/plain", "Mode received: " + mode);
// 	});

// 	// Handle POST request to updateCommand
// 	_server->on("/updateCommand", HTTP_POST, [](AsyncWebServerRequest* request) {
// 		if(!request->hasParam("body", true))
// 		{
// 			request->send(400, "text/plain", "Invalid request: No body found.");
// 			return;
// 		}

// 		String command = request->getParam("body", true)->value();
// 		request->send(200, "text/plain", "command received: " + command);
// 	});

// 	// Handle 404 errors
// 	_server->onNotFound([](AsyncWebServerRequest* request) {
// 		request->send(404, "text/plain", "404 Not Found");
// 	});
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
	response->print("<div class=\"container\" >");

	// Left side: User commands
	response->print("<div class=\"user-command\">");
	response->print("<h2>User Commands</h2>");
	response->print("<div class=\"command-content\">");
	response->print("<pre id=\"testCommand\"></pre>");

	response->print("</div>");
	response->print("</div>");

	// Right side: Log output
	String logs = logger.getBufferedLogs();
	response->print("<div class=\"log-output\">");
	response->print("<h2>Log Output</h2>");
	response->print("<div class=\"log-content\">");
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
	response->print("</div>"); // Closing log-content div
	response->print("</div>"); // Closing log-output div

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
}

void PageBuilder::sendPageTrailer(AsyncResponseStream* response)
{
	char buffer[LAST_TRAILER_HTML_LENGTH];
	const char* pageTrailerHtml = copyFromPROGMEM(LAST_TRAILER_HTML, buffer);
	response->print(pageTrailerHtml);
}

void PageBuilder::sendSettingTable(AsyncResponseStream* response, UPSTesterSetup& testerSetup,
								   const char* caption, SettingType type, const char* redirect_uri)
{
	response->print("<div id='custom-settings-page'>");
	response->print("<div class='settings-container' style='display: flex; flex-direction: row;'>");

	// Left side: Settings Table
	response->print("<div class='settings-table' style='flex: 2;'>");

	// response->printf("<form id='settingsForm' method='post' action='/%s' onsubmit='return "
	// 				 "handleSubmit(event);'>",
	// 				 redirect_uri);  this dont works due to printf formatting issue
	response->print("<form id='settingsForm' method='post' action='");
	response->print(redirect_uri);
	response->print("' onsubmit='return handleSubmit(event);'>");

	// Begin table
	response->print("<table border='1'>");

	SetupSpec& spec = testerSetup.specSetup();
	SetupTest& test = testerSetup.testSetup();
	sendTableCaption(response, caption);

	if(type == SettingType::SPEC)
	{
		sendSpecTable(response, spec);
	}
	else if(type == SettingType::TEST)
	{
		sendTestTable(response, test);
	}

	sendTableStyle(response);
	response->print("</table>");
	response->print("<button type='submit'>Save Settings</button>");
	response->print("</form>");
	response->print("</div>");

	// Right side: Update Message
	response->print("<div class='update-message' style='flex: 1; padding-left: 20px;'>");
	response->print("<h2 style='margin-top: 0;'>Update Status</h2>");
	response->print("<div id='updateStatus' style='padding: 10px; border: 1px solid #ddd; "
					"border-radius: 4px; background-color: #f9f9f9;'>No updates yet.</div>");
	response->print("</div>");

	response->print("</div>");
	response->print("</div>");

	// JavaScript for handling form submission and updating status message
	response->print(R"(
        <script>
            function handleSubmit(event) {
                event.preventDefault(); // Prevent default form submission

                var form = event.target;
                var formData = new FormData(form);

                var xhr = new XMLHttpRequest();
                xhr.open('POST', form.action, true);
                xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest'); // Indicate AJAX request
                xhr.onload = function() {
                    var statusDiv = document.getElementById('updateStatus');
                    if (xhr.status === 200) {
                        statusDiv.innerHTML = '<p style="color: green;">' + xhr.responseText + '</p>';
                    } else {
                        statusDiv.innerHTML = '<p style="color: red;">Failed to save settings. Please try again.</p>';
                    }
                };
                xhr.send(formData); // Send form data
                return false; // Prevent the default form submission
            }
        </script>)");
}

void PageBuilder::sendSpecTable(AsyncResponseStream* response, SetupSpec& spec)
{
	sendTableRow(response, "Rating (VA)", static_cast<double>(spec.Rating_va));
	sendInputField(response, "Rating_va", spec.Rating_va, UPS_MIN_VA, UPS_MAX_VA);
	sendTableRow(response, "Rated Voltage (V)", static_cast<double>(spec.RatedVoltage_volt));
	sendInputField(response, "RatedVoltage_volt", spec.RatedVoltage_volt, UPS_MIN_INPUT_VOLT,
				   UPS_MAX_INPUT_VOLT);
	sendTableRow(response, "Rated Current (A)", static_cast<double>(spec.RatedCurrent_amp));
	sendInputField(response, "RatedCurrent_amp", spec.RatedCurrent_amp);

	sendTableRow(response, "Minimum Input Voltage (V)",
				 static_cast<double>(spec.MinInputVoltage_volt));
	sendInputField(response, "MinInputVoltage_volt", spec.MinInputVoltage_volt, UPS_MIN_INPUT_VOLT,
				   UPS_MIN_INPUT_VOLT_MAXCAP);
	sendTableRow(response, "Maximum Input Voltage (V)",
				 static_cast<double>(spec.MaxInputVoltage_volt));
	sendInputField(response, "MaxInputVoltage_volt", spec.MaxInputVoltage_volt,
				   UPS_MAX_INPUT_VOLT_MINCAP, UPS_MAX_INPUT_VOLT);

	sendTableRow(response, "Avg Switch Time (ms)", static_cast<double>(spec.AvgSwitchTime_ms));
	sendInputField(response, "AvgSwitchTime_ms", spec.AvgSwitchTime_ms);

	sendTableRow(response, "Avg Backup Time (ms)", static_cast<double>(spec.AvgBackupTime_ms));
	sendInputField(response, "AvgBackupTime_ms", spec.AvgBackupTime_ms);
	sendTableRow(response, "Last Update (Spec)", spec.lastUpdateTime());
	sendTableRow(response, "Todays Date Time", __DATE__ " " __TIME__);
}
void PageBuilder::sendTestTable(AsyncResponseStream* response, SetupTest& test)
{
	sendTableRow(response, "Test Standard", test.TestStandard);
	sendInputField(response, "TestStandard", test.TestStandard);

	sendTableRow(response, "Test Mode", static_cast<int>(test.mode));
	sendDropdown(response, "TestMode", {"AUTO", "MANUAL"}, "AUTO");

	sendTableRow(response, "Test VA Rating", static_cast<double>(test.testVARating));
	sendInputField(response, "TestVARating", test.testVARating, UPS_MIN_VA, UPS_MAX_VA);

	sendTableRow(response, "Input Voltage (V)", static_cast<double>(test.inputVoltage_volt));
	sendInputField(response, "InputVoltage_volt", test.inputVoltage_volt, UPS_MIN_INPUT_VOLT,
				   UPS_MAX_INPUT_VOLT);
	sendTableRow(response, "TestDuration (ms)", static_cast<double>(test.testDuration_ms));
	sendInputField(response, "TestDuration_ms", test.testDuration_ms, UPS_MIN_TEST_DURATION,
				   UPS_MAX_TEST_DURATION);
	sendTableRow(response, "MinValidSwitchTime (ms)",
				 static_cast<double>(test.min_valid_switch_time_ms));
	sendInputField(response, "MinValidSwitchTime", test.min_valid_switch_time_ms,
				   UPS_MIN_SWITCHING_TIME_MS_SANITY_CHECK, UPS_MAX_SWITCHING_TIME_MS_SANITY_CHECK);
	sendTableRow(response, "MaxValidSwitchTime (ms)",
				 static_cast<double>(test.max_valid_switch_time_ms));
	sendInputField(response, "MaxValidSwitchTime", test.max_valid_switch_time_ms,
				   UPS_MIN_SWITCHING_TIME_MS_SANITY_CHECK, UPS_MAX_SWITCHING_TIME_MS_SANITY_CHECK);
	sendTableRow(response, "ToleranceSwitchTime (ms)",
				 static_cast<double>(test.ToleranceSwitchTime_ms));
	sendInputField(response, "ToleranceSwitchTime", test.ToleranceSwitchTime_ms,
				   UPS_MIN_SWITCH_TIME_TOLERANCE_MS, UPS_MAX_SWITCH_TIME_TOLERANCE_MS);
	sendTableRow(response, "MinValidBackupTime (ms)",
				 static_cast<double>(test.min_valid_backup_time_ms));
	sendInputField(response, "MinValidBackupTime", test.min_valid_backup_time_ms,
				   UPS_MIN_BACKUP_TIME_MS_SANITY_CHECK, UPS_MAX_BACKUP_TIME_MS_SANITY_CHECK);

	sendTableRow(response, "MaxValidBackupTime (ms)",
				 static_cast<double>(test.max_valid_backup_time_ms));
	sendInputField(response, "MaxValidBackupTime", test.max_valid_backup_time_ms,
				   UPS_MIN_BACKUP_TIME_MS_SANITY_CHECK, UPS_MAX_BACKUP_TIME_MS_SANITY_CHECK);

	sendTableRow(response, "MaxBackupTime (min)", static_cast<double>(test.maxBackupTime_min));
	sendInputField(response, "MaxBackupTime", test.maxBackupTime_min,
				   UPS_MIN_BACKUP_TIME_MS_SANITY_CHECK, UPS_MAX_BACKUP_TIME_MINUTE_SANITY_CHECK);

	sendTableRow(response, "ToleranceBackupTime (ms)",
				 static_cast<double>(test.ToleranceBackUpTime_ms));
	sendInputField(response, "ToleranceBackupTime", test.ToleranceBackUpTime_ms);

	sendTableRow(response, "MaxRetest", static_cast<double>(test.MaxRetest));
	sendInputField(response, "MaxRetest", test.MaxRetest);

	sendTableRow(response, "Last Update (Test)", test.lastUpdateTime());
	sendTableRow(response, "Todays Date Time", __DATE__ " " __TIME__);
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

void PageBuilder::sendInputField(AsyncResponseStream* response, const char* name, uint32_t value,
								 uint32_t minValue, uint32_t maxValue)
{
	response->printf("<tr><td>%s:</td>", name);
	response->printf("<td><input type='number' name='%s' value='%d' min='%d' max='%d'></td></tr>",
					 name, value, minValue, maxValue);
}
void PageBuilder::sendInputField(AsyncResponseStream* response, const char* name, const char* value,
								 const std::vector<const char*>& options)
{
	response->printf("<tr><td>%s:</td>", name);
	response->printf("<td><select name='%s'>", name);
	for(const auto& option: options)
	{
		response->printf("<option value='%s'%s>%s</option>", option,
						 strcmp(option, value) == 0 ? " selected" : "", option);
	}
	response->printf("</select></td></tr>");
}
void PageBuilder::sendDropdown(AsyncResponseStream* response, const char* name,
							   const std::vector<const char*>& options, const char* selected)

{
	response->print("<tr><td>");
	response->printf("<label>%s</label>", name);
	response->print("</td><td><select name='");
	response->print(name);
	response->print("'>");

	for(const auto& option: options)
	{
		response->printf("<option value='%s' %s>%s</option>", option,
						 (strcmp(option, selected) == 0) ? "selected" : "", option);
	}

	response->print("</select></td></tr>");
}

// -----TABLE RELATED ----------------------------------------//

void PageBuilder::sendTableCaption(AsyncResponseStream* response, const char* caption,
								   const char* style)
{
	response->printf("<caption style=%s>%s</caption>", style, caption);
}

void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, int value,
							   const char* cssClass)
{
	if(cssClass)
	{
		response->printf("<tr class='%s'><td>%s:</td><td>%d</td></tr>", cssClass, name, value);
	}
	else
	{
		response->printf("<tr><td>%s:</td><td>%d</td></tr>", name, value);
	}
}
void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, double value,
							   const char* cssClass)
{
	if(cssClass)
	{
		response->printf("<tr class='%s'><td>%s:</td><td>%.2f</td></tr>", cssClass, name, value);
	}
	else
	{
		response->printf("<tr><td>%s:</td><td>%.2f</td></tr>", name, value);
	}
}
void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, uint32_t value,
							   const char* cssClass)
{
	// Handle the class attribute separately
	if(cssClass)
	{
		response->printf("<tr class='%s'><td>%s:</td><td>%d</td></tr>", cssClass, name, value);
	}
	else
	{
		response->printf("<tr><td>%s:</td><td>%d</td></tr>", name, value);
	}
}

void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, String value,
							   const char* cssClass)
{
	// Handle the class attribute separately
	if(cssClass)
	{
		response->printf("<tr class='%s'><td>%s:</td><td>%s</td></tr>", cssClass, name,
						 value.c_str());
	}
	else
	{
		response->printf("<tr><td>%s:</td><td>%s</td></tr>", name, value.c_str());
	}
}

void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, const char* value,
							   const char* cssClass)
{
	if(cssClass)
	{
		response->printf("<tr class='%s'><td>%s:</td><td>%s</td></tr>", cssClass, name, value);
	}
	else
	{
		response->printf("<tr><td>%s:</td><td>%s</td></tr>", name, value);
	}
}

void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, time_t timeValue,
							   const char* cssClass)
{
	// Convert time_t to a human-readable string
	struct tm* timeinfo = localtime(&timeValue);
	char timeStr[20]; // Adjust the size if needed
	std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

	// Handle the class attribute separately
	if(cssClass)
	{
		response->printf("<tr class='%s'><td>%s:</td><td>%s</td></tr>", cssClass, name, timeStr);
	}
	else
	{
		response->printf("<tr><td>%s:</td><td>%s</td></tr>", name, timeStr);
	}
}

template<typename... Args>
void PageBuilder::sendColorGroup(AsyncResponseStream* response, int span,
								 const char* background_color, Args&&... args)
{
	static_assert(sizeof...(args) == (span - 1), "Each span must have a color group");
	response->print("<colgroup>");
	response->printf("<col span=\"%d\" style=\"background-color:%s\">", span, background_color);
	(response->printf("<col style=\"background-color:%s\">", args), ...);
	response->print("</colgroup>");
}

void PageBuilder::sendTableStyle(AsyncResponseStream* response)
{
	response->print("<style>");
	response->print("#custom-settings-page table { width: 100%; border-collapse: collapse; margin: "
					"20px 0; font-size: 1em; font-family: sans-serif; min-width: 400px; }");
	response->print("#custom-settings-page th, td { padding: 12px 15px; border: 1px solid #ddd; "
					"text-align: left; }");
	response->print("#custom-settings-page th { background-color: #f4f4f4; font-weight: bold; }");
	response->print("#custom-settings-page tr:nth-child(even) { background-color: #f9f9f9; }");
	response->print("#custom-settings-page tr:hover { background-color: #f1f1f1; }");
	response->print("#custom-settings-page input[type='text'], #custom-settings-page "
					"input[type='number'], #custom-settings-page select { width: 100%; padding: "
					"8px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }");
	response->print("#custom-settings-page button { background-color: #4CAF50; color: white; "
					"padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; }");
	response->print("#custom-settings-page button:hover { background-color: #45a049; }");
	response->print("#custom-settings-page form { padding: 20px; background-color: #fff; "
					"border-radius: 5px; box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.1); }");
	response->print("</style>");
}
